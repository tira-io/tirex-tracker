#include "devcontainerstats.hpp"

#include "../../logging.hpp"

#include <filesystem>

using tirex::DevContainerStats;
using tirex::Stats;

namespace fs = std::filesystem;

const char* DevContainerStats::version = nullptr;
const std::set<tirexMeasure> DevContainerStats::measures{
		TIREX_DEVCONTAINER_CONF_PATHS
		/** \todo add measures */
};

static std::vector<fs::path> searchDevcontainerFiles(fs::path basepath) {
	std::vector<fs::path> files;
	for (const auto& entry : fs::recursive_directory_iterator(basepath)) {
		if (entry.is_directory()) {
			for (auto&& path :
				 {entry.path() / ".devcontainer.json", entry.path() / ".devcontainer" / "devcontainer.json"}) {
				if (fs::exists(path) && fs::is_regular_file(path))
					files.emplace_back(std::move(path));
			}
		}
	}
	return files;
}

DevContainerStats::DevContainerStats() {
	/** \todo check if workdir is inside a repository and take the repo root instead **/
	auto devcontainerConfs = searchDevcontainerFiles(fs::current_path());
	tirex::log::info(
			"devcontainer", "Found {} devcontainer configurations by searching started at {}", devcontainerConfs.size(),
			fs::current_path().c_str()
	);
	for (const auto& path : devcontainerConfs) {
		tirex::log::info("devcontainer", "{}", path.c_str());
	}
}

std::set<tirexMeasure> DevContainerStats::providedMeasures() noexcept { return measures; }
void DevContainerStats::start() {}
void DevContainerStats::stop() {}
Stats DevContainerStats::getStats() { return {}; /** \todo implement **/ }