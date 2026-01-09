#include "gitstats.hpp"

#include "../../abort.hpp"
#include "../../logging.hpp"
#include "../utils/rangeutils.hpp"

#include <git2.h>
#include <sha1.h>
#include <zip.h>

#if __cpp_lib_expected
#include <expected>
namespace std23 {
	template <class T, class E>
	using expected = std::expected<T, E>;
	template <class E>
	using unexpected = std::unexpected<E>;
} // namespace std23
#else
#include <tl/expected.hpp>
namespace std23 {
	template <class T, class E>
	using expected = tl::expected<T, E>;
	template <class E>
	using unexpected = tl::unexpected<E>;
} // namespace std23
#endif

#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>

using namespace std::string_literals;

using tirex::GitStats;
using tirex::Stats;

const char* GitStats::version = "libgit v." LIBGIT2_VERSION;
const std::set<tirexMeasure> GitStats::measures{
		TIREX_GIT_IS_REPO,
		TIREX_GIT_HASH,
		TIREX_GIT_LAST_COMMIT_HASH,
		TIREX_GIT_BRANCH,
		TIREX_GIT_BRANCH_UPSTREAM,
		TIREX_GIT_TAGS,
		TIREX_GIT_REMOTE_ORIGIN,
		TIREX_GIT_UNCOMMITTED_CHANGES,
		TIREX_GIT_UNPUSHED_CHANGES,
		TIREX_GIT_UNCHECKED_FILES,
		TIREX_GIT_ROOT,
		TIREX_GIT_ARCHIVE_PATH
};

std::optional<std::filesystem::path> GitStats::getRepoRootDir() {
	git_libgit2_init();
	git_repository* repo;
	auto root = (git_repository_open_ext(&repo, "./", 0, nullptr) == 0)
						? std::make_optional<std::filesystem::path>(git_repository_workdir(repo))
						: std::nullopt;
	git_libgit2_shutdown();
	return root;
}

static std::string formatMemory(size_t size) {
	std::array units{"B", "KB", "MB", "GB", "TB"};
	size_t i = 0;
	while (i < units.size() && (size / 1000) * 1000 == size && size != 0) {
		++i;
		size /= 1000;
	}
	return _fmt::format("{} {}", size, units[i]);
}

static std::string getLastCommitHash(git_repository* repo) {
	git_oid id;
	if (int err; (err = git_reference_name_to_id(&id, repo, "HEAD")) != 0) {
		tirex::log::error("gitstats", "Failed to lookup HEAD: {}", git_error_last()->message);
		return "";
	}
	char buf[GIT_OID_SHA1_HEXSIZE + 1];
	git_oid_tostr(buf, sizeof(buf), &id);
	return std::string{buf};
}

static std::tuple<std::string, std::string> getBranchName(git_repository* repo) {
	git_reference* head;
	if (int err; (err = git_repository_head(&head, repo)) != 0) {
		tirex::log::error("gitstats", "Failed to fetch repository head: {}", git_error_last()->message);
		return {"(failed to fetch)", ""};
	}
	// Local Branch Name
	const char* local;
	if (int err; (err = git_branch_name(&local, head)) != 0) {
		tirex::log::error("gitstats", "Failed to get branch name: {}", git_error_last()->message);
		return {"(failed to fetch)", ""};
	}
	// Upstream Branch Name
	git_buf buf = GIT_BUF_INIT;
	if (int err; (err = git_branch_upstream_name(&buf, repo, git_reference_name(head))) == 0) {
		/** success **/
	} else if (err == GIT_ENOTFOUND) {
		git_buf_dispose(&buf);
		tirex::log::warn("gitstats", "Branch is unknown to upstream");
		return {{local}, {""}};
	} else {
		git_buf_dispose(&buf);
		tirex::log::error("gitstats", "Failed to get upstream branch name: {}", git_error_last()->message);
		return {{local}, {""}};
	}
	std::string remote = buf.ptr;
	git_buf_dispose(&buf);
	return {{local}, remote};
}

static std::string getRemoteOrigin(git_repository* repo) {
	git_remote* remote;
	if (int err; (err = git_remote_lookup(&remote, repo, "origin")) != 0) {
		/** No remote called origin is set **/
		tirex::log::warn("gitstats", "Failed to lookup remote/origin: {}", git_error_last()->message);
		return "";
	}
	std::string url = (git_remote_url(remote) == nullptr) ? "" : git_remote_url(remote);
	git_remote_free(remote);
	return url;
}

template <typename T>
static constexpr int wrap(const char* name, git_oid* oid, void* payload) {
	auto& fn = *static_cast<T*>(payload);
	return fn(name, oid);
}

static std::vector<std::string> getTags(git_repository* repo) {
	std::vector<std::string> tagNames;
	git_oid head;
	if (int err; (err = git_reference_name_to_id(&head, repo, "HEAD")) != 0) {
		tirex::log::error("gitstats", "Failed to lookup HEAD: {}", git_error_last()->message);
		return {};
	}
	auto callback = [head, &repo, &tagNames](const char* name, git_oid* oid) {
		git_object* object = nullptr;
		const git_oid* target = nullptr;
		if (int err; (err = git_object_lookup(&object, repo, oid, GIT_OBJECT_ANY)) != 0) {
			tirex::log::error("gitstats", "Failed to lookup object: {}", git_error_last()->message);
			return 0; // Return 0 to stop processing this tag but don't abort the iteration
		}
		if (git_object_type(object) == GIT_OBJECT_TAG) { // Annotated tag
			git_tag* tag;
			if (int err; (err = git_tag_lookup(&tag, repo, oid)) != 0) {
				tirex::log::error("gitstats", "Failed to lookup tag: {}", git_error_last()->message);
				return 0; // Return 0 to stop processing this tag but don't abort the iteration
			}
			target = git_tag_target_id(tag);
			git_tag_free(tag);
		} else { // Lightweight tag
			target = oid;
		}
		if (git_oid_equal(&head, target)) {
			tagNames.emplace_back(name);
		}
		git_object_free(object);
		return 0;
	};
	git_tag_foreach(repo, wrap<decltype(callback)>, static_cast<void*>(&callback));
	return tagNames;
}

static std::string hashAllFiles(git_repository* repo) {
	Chocobo1::SHA1 hash;
	git_status_list* list;
	git_status_options opts = GIT_STATUS_OPTIONS_INIT;
	opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_INCLUDE_UNMODIFIED;
	git_status_list_new(&list, repo, &opts);
	auto changes = git_status_list_entrycount(list);
	std::filesystem::path root = git_repository_workdir(repo);
	for (size_t i = 0; i < changes; ++i) {
		auto entry = git_status_byindex(list, i);
		if (entry->status == GIT_STATUS_INDEX_DELETED || entry->status == GIT_STATUS_WT_DELETED)
			continue;
		if (!std::filesystem::is_regular_file(root / entry->index_to_workdir->new_file.path)) {
			tirex::log::warn(
					"gitstats", "The folder {} is not checked into the repository nor ignored!",
					entry->index_to_workdir->new_file.path
			);
			tirex::log::warn(
					"gitstats", "I will not include it in the hash. Please add it to the .gitignore if is not part of "
								"your codebase or check it into the repository if it should be."
			);
			tirex::abort(tirexLogLevel::WARN, "Folders that are not checked into the repository are ignored.");
			continue;
		}
		std::ifstream is(root / entry->index_to_workdir->new_file.path, std::ios::binary);
		if (!is) {
			tirex::log::error("gitstats", "Error opening file: {}", entry->index_to_workdir->new_file.path);
			continue;
		}
		for (char buffer[8192]; is; is.read(buffer, sizeof(buffer)))
			hash.addData(buffer, is.gcount());
	}
	git_status_list_free(list);
	return hash.finalize().toString();
}

static std23::expected<void, std::string>
repoToArchive(git_repository* repo, const std::filesystem::path& archive, size_t archivalSizeLimit) noexcept {
	std::filesystem::path root = git_repository_workdir(repo);
	tirex::log::debug("gitstats", "Archiving git repo at root {} to {}", root.string(), archive.string());
	int err;
	auto handle = zip_open(archive.string().c_str(), ZIP_CREATE, &err);
	if (handle == nullptr) {
		zip_error_t error;
		zip_error_init_with_code(&error, err);
		tirex::log::error("gitstats", "Failed to create zip archive: {}", zip_error_strerror(&error));
		zip_error_fini(&error);
		return std23::unexpected<std::string>{"Failed to archive repo"};
	}
	git_status_list* list;
	git_status_options opts = GIT_STATUS_OPTIONS_INIT;
	opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_INCLUDE_UNMODIFIED;
	git_status_list_new(&list, repo, &opts);
	auto changes = git_status_list_entrycount(list);
	for (size_t i = 0; i < changes; ++i) {
		auto entry = git_status_byindex(list, i);
		if (entry->status == GIT_STATUS_INDEX_DELETED || entry->status == GIT_STATUS_WT_DELETED)
			continue;
		auto path = root / entry->index_to_workdir->new_file.path;
		if (!std::filesystem::is_regular_file(path)) {
			tirex::log::warn(
					"gitstats", "The folder {} is not checked into the repository nor ignored!",
					entry->index_to_workdir->new_file.path
			);
			tirex::log::warn(
					"gitstats", "I will not include it in the archive. Please add it to the .gitignore if is not part "
								"of your codebase or check it into the repository if it should be."
			);
			continue;
			/** \todo return unexpected if pedantic **/
			/*return std23::unexpected<std::string>{
					"The repositories contains an unchecked folder. Add it to .gitignore or check it into the repository."
			};*/
		} else if (std::filesystem::file_size(path) >= archivalSizeLimit) {
			tirex::log::warn(
					"gitstats", "The file {} is larger than the configured limit of {} and will be ignored.",
					entry->index_to_workdir->new_file.path, formatMemory(archivalSizeLimit)
			);
			continue;
			/** \todo return unexpected if pedantic **/
			/*return std23::unexpected<std::string>{
					"The repositories contains a large file which will not be added to the git archive."
			};*/
		}
		auto source = zip_source_file(handle, path.string().c_str(), 0, 0);
		if (source == nullptr) {
			tirex::log::error(
					"gitstats", "Error reading file: {}; I will not add it to the archive",
					entry->index_to_workdir->new_file.path
			);
			tirex::abort(tirexLogLevel::ERROR, "Failed to read file to add it to the archive");
			continue;
		}
		if (zip_file_add(handle, entry->index_to_workdir->new_file.path, source, ZIP_FL_OVERWRITE) < 0) {
			tirex::log::error("gitstats", "Error adding file to archive: {}", entry->index_to_workdir->new_file.path);
			tirex::abort(tirexLogLevel::ERROR, "Failed to add file to the archive");
			continue;
		}
	}
	git_status_list_free(list);
	if (zip_close(handle) < 0) {
		tirex::log::error("gitstats", "Failed to write out archive: {}", zip_strerror(handle));
		return std23::unexpected<std::string>{"Failed to write out archive"};
	}
	return {};
}

struct GitStatusStats {
	/** The number of files that were previously added to the repository and have uncommitted changes **/
	size_t numModified;
	/** The number of files that were not yet added to the repository **/
	size_t numNew;
	/** The number of commits ahead of remote origin **/
	size_t ahead;
	/** The number of commits behind of remote origin **/
	size_t behind;
};

static GitStatusStats getStatusStats(git_repository* repo) {
	GitStatusStats stats = {.numModified = 0, .numNew = 0, .ahead = 0, .behind = 0};
	{
		git_status_list* list;
		git_status_list_new(&list, repo, nullptr);
		auto changes = git_status_list_entrycount(list);
		for (size_t i = 0; i < changes; ++i) {
			auto entry = git_status_byindex(list, i);
			if (entry->status & (git_status_t::GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_NEW))
				++stats.numNew;
			else if (entry->status & (git_status_t::GIT_STATUS_INDEX_MODIFIED | git_status_t::GIT_STATUS_WT_MODIFIED))
				++stats.numModified;
		}
		git_status_list_free(list);
	}
	// Ahead / Behind
	{
		git_reference* upstream;
		git_reference* head;
		if (int err; (err = git_repository_head(&head, repo)) != 0) {
			tirex::log::error("gitstats", "Failed to fetch repository head: {}", git_error_last()->message);
			return stats;
		}
		if (int err; (err = git_branch_upstream(&upstream, head)) != 0) {
			tirex::log::error("gitstats", "Failed to get upstream branch: {}", git_error_last()->message);
			return stats;
		}
		auto local = git_reference_target(head);
		auto remote = git_reference_target(upstream);
		git_graph_ahead_behind(&stats.ahead, &stats.behind, repo, local, remote);
	}
	return stats;
}

GitStats::GitStats() : repo(nullptr) {
	git_libgit2_init();
	if (int err; git_repository_open_ext(&repo, "./", 0, nullptr) < 0)
		tirex::log::error("gitstats", "Failed to open git repository: {}", git_error_last()->message);
}
GitStats::~GitStats() { git_libgit2_shutdown(); }

bool GitStats::isRepository() const noexcept { return repo != nullptr; }

std::set<tirexMeasure> GitStats::providedMeasures() noexcept { return measures; }

Stats GitStats::getInfo() {
	if (isRepository()) {
		auto status = getStatusStats(repo);
		tirex::log::info(
				"gitstats", "I counted {} tracked files that were changed and {} untracked files", status.numModified,
				status.numNew
		);
		tirex::log::info("gitstats", "Local is {} commits ahead and {} behind upstream", status.ahead, status.behind);
		/**
		 * clang complains that std::tmpnam is deprecated. [cppreference](https://en.cppreference.com/w/cpp/io/c/tmpnam)
		 * does not seem to know about its deprecation. The C pendant, tmpnam, seems to be deprecated though
		 * (https://www.man7.org/linux/man-pages/man3/tmpnam.3.html). However luckily we are not using C. The suggested
		 * alternatives are mkstemp (POSIX and not standard C) or tmpnam. tmpnam does not do what we need since it
		 * directly opens and manages the file for us but we need a *path* to a temporary file. std::tmpnam is
		 * discouraged since there is a slight chance that someone else could get the same name with std::tmpnam and
		 * create the same temporary file as well (a race condition). One way to make the chance of this happening even
		 * lower could be to directly create the file afterwards but we can't entirely remove the risk and the chances
		 * are quite low anyway.
		 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		std::filesystem::path tmpfile{std::tmpnam(nullptr)};
#pragma clang diagnostic pop
		if (enabled.contains(TIREX_GIT_ARCHIVE_PATH)) {
			repoToArchive(repo, tmpfile, archivalSizeLimit);
		}
		auto [local, remote] = getBranchName(repo);
		return makeFilteredStats(
				enabled, std::pair{TIREX_GIT_IS_REPO, "1"s}, std::pair{TIREX_GIT_HASH, hashAllFiles(repo)},
				std::pair{TIREX_GIT_LAST_COMMIT_HASH, getLastCommitHash(repo)}, std::pair{TIREX_GIT_BRANCH, local},
				std::pair{TIREX_GIT_BRANCH_UPSTREAM, remote},
				std::pair{TIREX_GIT_TAGS, "["s + tirex::utils::join(getTags(repo), ", ") + "]"s},
				std::pair{TIREX_GIT_REMOTE_ORIGIN, getRemoteOrigin(repo)},
				std::pair{TIREX_GIT_UNCOMMITTED_CHANGES, (status.numModified != 0) ? "1"s : "0"s},
				std::pair{TIREX_GIT_UNPUSHED_CHANGES, ((status.ahead != 0) || remote.empty()) ? "1"s : "0"s},
				std::pair{TIREX_GIT_UNCHECKED_FILES, (status.numNew != 0) ? "1"s : "0"s},
				std::pair{TIREX_GIT_ROOT, git_repository_workdir(repo)},
				std::pair{TIREX_GIT_ARCHIVE_PATH, TmpFile{tmpfile}}
		);
	} else {
		return makeFilteredStats(enabled, std::pair{TIREX_GIT_IS_REPO, "0"s});
	}
}
