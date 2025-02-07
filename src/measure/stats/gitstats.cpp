#include "gitstats.hpp"

#include "../../logging.hpp"

#include <git2.h>

#include <format>
#include <iostream>
#include <string>

using namespace std::string_literals;

using msr::GitStats;
using msr::Stats;

const char* GitStats::version = "libgit v." LIBGIT2_VERSION;
const std::set<msrMeasure> GitStats::measures{
		MSR_GIT_IS_REPO,		MSR_GIT_HASH,		   MSR_GIT_LAST_COMMIT_HASH,	MSR_GIT_BRANCH,
		MSR_GIT_TAGS,			MSR_GIT_REMOTE_ORIGIN, MSR_GIT_UNCOMMITTED_CHANGES, MSR_GIT_UNPUSHED_CHANGES,
		MSR_GIT_UNCHECKED_FILES
};

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
};

static GitStatusStats getStatusStats(git_repository* repo) {
	GitStatusStats stats = {.numModified = 0, .numNew = 0};
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
		return {{MSR_GIT_IS_REPO, "1"s},
				{MSR_GIT_HASH, "TODO"s},
				{MSR_GIT_LAST_COMMIT_HASH, getLastCommitHash(repo)},
				{MSR_GIT_BRANCH, "TODO"s},
				{MSR_GIT_TAGS, "TODO"s},
				{MSR_GIT_REMOTE_ORIGIN, getRemoteOrigin(repo)},
				{MSR_GIT_UNCOMMITTED_CHANGES, (status.numModified != 0) ? "1"s : "0"s},
				{MSR_GIT_UNPUSHED_CHANGES, "TODO"s},
				{MSR_GIT_UNCHECKED_FILES, (status.numNew != 0) ? "1"s : "0"s}};
	} else {
		return {{MSR_GIT_IS_REPO, "0"s}};
	}
}