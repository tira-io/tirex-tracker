#ifndef TIREX_TESTS_TESTUTILS_MOCKFOLDER_HPP
#define TIREX_TESTS_TESTUTILS_MOCKFOLDER_HPP

#include <catch2/catch_test_macros.hpp>
#include <git2.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

struct File {
	std::filesystem::path relpath;
	bool stageInGit;
	bool commitToGit;
	std::string content;
};

/**
 * @brief Creates a new temporary directory for testing purposes. The directory and its contents are automatically
 * deleted upon destruction.
 */
class MockFolder {
private:
	std::filesystem::path _path;
	git_repository* repo;

	MockFolder(const MockFolder&) = delete;
	MockFolder& operator=(const MockFolder&) const = delete;

	void destroy() {
		if (!_path.string().empty()) {
			git_libgit2_shutdown();
			std::filesystem::remove_all(_path);
			std::cout << "Deleted Mock Folder at " << _path << std::endl;
		}
	}

public:
	MockFolder() : _path(""), repo(nullptr) {}
	explicit MockFolder(std::string_view name) : _path(std::filesystem::temp_directory_path() / name), repo(nullptr) {
		std::cout << "Created Mock Folder at " << _path << std::endl;
		std::filesystem::create_directory(_path);
		git_libgit2_init();
		REQUIRE(git_repository_init(&repo, _path.c_str(), false) >= 0);
	}
	MockFolder(MockFolder&& other) : _path(std::move(other._path)), repo(std::move(other.repo)) {}
	~MockFolder() { destroy(); }

	MockFolder& operator=(MockFolder&& other) {
		destroy();
		_path = std::move(other._path);
		repo = std::move(other.repo);
		return *this;
	}

	const std::filesystem::path& path() const noexcept { return _path; }

	void addFile(const File& file) {
		std::ofstream fs{(_path / file.relpath).c_str()};
		fs << file.content << std::flush;
		if (file.stageInGit) {
			git_index* index = nullptr;
			git_repository_index(&index, repo);
			git_index_add_bypath(index, file.relpath.c_str());
			git_index_write(index);

			if (file.commitToGit) { // Commit
				git_reference* ref = nullptr;
				git_object* parent = nullptr;
				git_revparse_ext(
						&parent, &ref, repo, "HEAD"
				); // Allowed values: GIT_ENOTFOUND (no previous commits) or GIT_OK

				git_oid tree_oid;
				git_index_write_tree(&tree_oid, index);
				git_tree* tree = nullptr;
				git_tree_lookup(&tree, repo, &tree_oid);

				// Create a commit
				git_signature* sig = nullptr;
				git_signature_now(&sig, "Author", "author@example.com");

				git_oid commit_oid;
				REQUIRE(git_commit_create_v(
								&commit_oid, repo, "HEAD", sig, sig, nullptr, "Commit", tree,
								(parent != nullptr) ? 1 : 0, parent
						) >= 0);

				// Clean up
				git_signature_free(sig);
				git_tree_free(tree);
				git_reference_free(ref);
				git_object_free(parent);
			}

			git_index_free(index);
		}
	}
};

#endif