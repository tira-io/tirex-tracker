#include "devcontainerstats.hpp"

#include "../../logging.hpp"
#include "../utils/osutils.hpp"
#include "../utils/rangeutils.hpp"
#include "./gitstats.hpp"

#include <filesystem>

using namespace std::string_literals;

using tirex::DevContainerStats;
using tirex::Stats;

namespace fs = std::filesystem;

const char* DevContainerStats::version = nullptr;
const std::set<tirexMeasure> DevContainerStats::measures{TIREX_DEVCONTAINER_CONF_PATHS};

static std::vector<fs::path> searchDevcontainerFiles(fs::path basepath) {
	// https://containers.dev/implementors/spec/#devcontainerjson
	std::vector<fs::path> files;
	for (auto&& path : {basepath / ".devcontainer" / "devcontainer.json", basepath / ".devcontainer.json"}) {
		if (fs::exists(path) && fs::is_regular_file(path))
			files.emplace_back(std::move(path));
	}
	for (const auto& entry : fs::recursive_directory_iterator(basepath)) {
		if (entry.is_directory()) {
			for (auto&& path :
				 {entry.path() / ".devcontainer" / "devcontainer.json", entry.path() / ".devcontainer.json"}) {
				if (fs::exists(path) && fs::is_regular_file(path))
					files.emplace_back(std::move(path));
			}
		}
	}
	return files;
}

DevContainerStats::DevContainerStats(const tirex::TrackingTarget& target)
		: targetWorkingDir(tirex::utils::getProcessWorkingDirectory(target.pid).value_or(fs::current_path())) {
	auto devcontainerConfs = searchDevcontainerFiles(targetWorkingDir);
	tirex::log::info(
			"devcontainer", "Found {} devcontainer configurations by searching started at {}", devcontainerConfs.size(),
			targetWorkingDir.string().c_str()
	);
	for (const auto& path : devcontainerConfs) {
		tirex::log::info("devcontainer", "  {}", path.string().c_str());
	}
	if (auto root = GitStats::getRepoRootDir(targetWorkingDir); root.has_value()) {
		auto confs = searchDevcontainerFiles(*root);
		tirex::log::info(
				"devcontainer", "Found {} devcontainer configurations by searching started at git root ({})",
				confs.size(), root->string().c_str()
		);
		for (const auto& path : confs) {
			tirex::log::info("devcontainer", "  {}", path.string().c_str());
			devcontainerConfs.emplace_back(path);
		}
	}
}

std::set<tirexMeasure> DevContainerStats::providedMeasures() noexcept { return measures; }

Stats DevContainerStats::getInfo() {
	return makeFilteredStats(
			enabled, std::pair{
							 TIREX_DEVCONTAINER_CONF_PATHS,
							 "["s + utils::join(searchDevcontainerFiles(targetWorkingDir), ", ") + "]"
					 }
	);
}