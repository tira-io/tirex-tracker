#include "gitstats.hpp"

#include "../../logging.hpp"

#include <git2.h>

#include <iostream>
#include <string>
#include <tuple>

using namespace std::string_literals;

using msr::GitStats;
using msr::Stats;

const char* GitStats::version = "libgit v." LIBGIT2_VERSION;
const std::set<msrMeasure> GitStats::measures{MSR_GIT_IS_REPO,			MSR_GIT_HASH,
											  MSR_GIT_LAST_COMMIT_HASH, MSR_GIT_BRANCH,
											  MSR_GIT_BRANCH_UPSTREAM,	MSR_GIT_TAGS,
											  MSR_GIT_REMOTE_ORIGIN,	MSR_GIT_UNCOMMITTED_CHANGES,
											  MSR_GIT_UNPUSHED_CHANGES, MSR_GIT_UNCHECKED_FILES};

static std::string getLastCommitHash(git_repository* repo) {
	git_oid id;
	if (int err; err = git_reference_name_to_id(&id, repo, "HEAD")) {
		msr::log::error("gitstats", "Failed to lookup HEAD: {}", git_error_last()->message);
		return "";
	}
	char buf[GIT_OID_SHA1_HEXSIZE + 1];
	git_oid_tostr(buf, sizeof(buf), &id);
	return std::string{buf};
}

static std::string getShortname(git_repository* repo) {
	git_reference* head;
	if (int err; err = git_repository_head(&head, repo)) {
		msr::log::error("gitstats", "Failed to fetch repository head: {}", git_error_last()->message);
		return "";
	}
	std::string hash = git_reference_shorthand(head);
	git_reference_free(head);
	return hash;
}

static std::tuple<std::string, std::string> getBranchName(git_repository* repo) {
	git_reference* head;
	if (int err; err = git_repository_head(&head, repo)) {
		msr::log::error("gitstats", "Failed to fetch repository head: {}", git_error_last()->message);
		return {"(failed to fetch)", ""};
	}
	// Local Branch Name
	const char* local;
	if (int err; err = git_branch_name(&local, head)) {
		msr::log::error("gitstats", "Failed to get branch name: {}", git_error_last()->message);
		return {"(failed to fetch)", ""};
	}
	// Upstream Branch Name
	git_buf buf = GIT_BUF_INIT;
	if (int err; (err = git_branch_upstream_name(&buf, repo, git_reference_name(head))) == 0) {
		/** success **/
	} else if (err == GIT_ENOTFOUND) {
		git_buf_dispose(&buf);
		msr::log::warn("gitstats", "Branch is unknown to upstream");
		return {{local}, {""}};
	} else {
		git_buf_dispose(&buf);
		msr::log::error("gitstats", "Failed to get upstream branch name: {}", git_error_last()->message);
		return {{local}, {""}};
	}
	std::string remote = buf.ptr;
	git_buf_dispose(&buf);
	return {{local}, remote};
}

static std::string getRemoteOrigin(git_repository* repo) {
	git_remote* remote;
	if (int err; err = git_remote_lookup(&remote, repo, "origin")) {
		/** No remote called origin is set **/
		msr::log::warn("gitstats", "Failed to lookup remote/origin: {}", git_error_last()->message);
		return "";
	}
	std::string url = git_remote_url(remote);
	git_remote_free(remote);
	return url;
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
		if (int err; err = git_repository_head(&head, repo)) {
			msr::log::error("gitstats", "Failed to fetch repository head: {}", git_error_last()->message);
			return stats;
		}
		if (int err; err = git_branch_upstream(&upstream, head)) {
			msr::log::error("gitstats", "Failed to get upstream branch: {}", git_error_last()->message);
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
		msr::log::error("gitstats", "Failed to open git repository: {}", git_error_last()->message);
}
GitStats::~GitStats() { git_libgit2_shutdown(); }

bool GitStats::isRepository() const noexcept { return repo != nullptr; }

void GitStats::start() { msr::log::info("gitstats", "Is a Git Repository: {}", (isRepository() ? "Yes" : "No")); }
void GitStats::stop() { /* nothing to do */ }
Stats GitStats::getStats() {
	/** \todo: filter by requested metrics */
	if (isRepository()) {
		auto status = getStatusStats(repo);
		msr::log::info(
				"gitstats", "I counted {} tracked files that were changed and {} untracked files", status.numModified,
				status.numNew
		);
		msr::log::info("gitstats", "Local is {} commits ahead and {} behind upstream", status.ahead, status.behind);
		auto [local, remote] = getBranchName(repo);
		return {{MSR_GIT_IS_REPO, "1"s},
				{MSR_GIT_HASH, "TODO"s},
				{MSR_GIT_LAST_COMMIT_HASH, getLastCommitHash(repo)},
				{MSR_GIT_BRANCH, local},
				{MSR_GIT_BRANCH_UPSTREAM, remote},
				{MSR_GIT_TAGS, "TODO"s},
				{MSR_GIT_REMOTE_ORIGIN, getRemoteOrigin(repo)},
				{MSR_GIT_UNCOMMITTED_CHANGES, (status.numModified != 0) ? "1"s : "0"s},
				{MSR_GIT_UNPUSHED_CHANGES, ((status.ahead != 0) || remote.empty()) ? "1"s : "0"s},
				{MSR_GIT_UNCHECKED_FILES, (status.numNew != 0) ? "1"s : "0"s}};
	} else {
		return {{MSR_GIT_IS_REPO, "0"s}};
	}
}