#include "irtracker.h"

#include <cassert>
#include <fstream>
#include <functional>
#include <map>
#include <set>

using ResultMap = std::map<tirexMeasure, std::string>;

static const std::map<std::string, std::set<tirexMeasure>> measuresPerVersion{
		{"0.1",
		 {TIREX_CPU_MODEL_NAME, TIREX_CPU_ARCHITECTURE, TIREX_CPU_CORES_PER_SOCKET, TIREX_GPU_MODEL_NAME,
		  TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB, TIREX_GPU_NUM_CORES, TIREX_OS_KERNEL, TIREX_OS_NAME,
		  TIREX_GIT_REMOTE_ORIGIN, TIREX_GIT_LAST_COMMIT_HASH}},
		{"0.2",
		 {TIREX_OS_NAME,
		  TIREX_OS_KERNEL,
		  TIREX_TIME_START,
		  TIREX_TIME_STOP,
		  TIREX_TIME_ELAPSED_WALL_CLOCK_MS,
		  TIREX_TIME_ELAPSED_USER_MS,
		  TIREX_TIME_ELAPSED_SYSTEM_MS,
		  TIREX_CPU_USED_PROCESS_PERCENT,
		  TIREX_CPU_USED_SYSTEM_PERCENT,
		  TIREX_CPU_AVAILABLE_SYSTEM_CORES,
		  TIREX_CPU_ENERGY_SYSTEM_JOULES,
		  TIREX_CPU_FEATURES,
		  TIREX_CPU_FREQUENCY_MHZ,
		  TIREX_CPU_FREQUENCY_MIN_MHZ,
		  TIREX_CPU_FREQUENCY_MAX_MHZ,
		  TIREX_CPU_VENDOR_ID,
		  TIREX_CPU_BYTE_ORDER,
		  TIREX_CPU_ARCHITECTURE,
		  TIREX_CPU_MODEL_NAME,
		  TIREX_CPU_CORES_PER_SOCKET,
		  TIREX_CPU_THREADS_PER_CORE,
		  TIREX_CPU_CACHES,
		  TIREX_CPU_VIRTUALIZATION,
		  TIREX_RAM_USED_PROCESS_KB,
		  TIREX_RAM_USED_SYSTEM_MB,
		  TIREX_RAM_AVAILABLE_SYSTEM_MB,
		  TIREX_RAM_ENERGY_SYSTEM_JOULES,
		  TIREX_GPU_SUPPORTED,
		  TIREX_GPU_MODEL_NAME,
		  TIREX_GPU_NUM_CORES,
		  TIREX_GPU_USED_PROCESS_PERCENT,
		  TIREX_GPU_USED_SYSTEM_PERCENT,
		  TIREX_GPU_VRAM_USED_PROCESS_MB,
		  TIREX_GPU_VRAM_USED_SYSTEM_MB,
		  TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB,
		  TIREX_GPU_ENERGY_SYSTEM_JOULES,
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
		  TIREX_GIT_ARCHIVE_PATH}}
};

static std::function<bool(const tirexResultEntry&)> versionFilter(const std::string& version) {
	const auto& measures = measuresPerVersion.at(version);
	return [measures](const tirexResultEntry& entry) { return measures.find(entry.source) != measures.end(); };
}

/** \todo this file *needs* to be refactored! */

/**
 * @brief Convers the measure results into a std::map.
 * 
 * @param result 
 * @return
 */
template <typename C>
static void asMap(ResultMap& ret, const tirexResult* result, C&& filter) {
	size_t num;
	tirexResultEntry entry;
	if (tirexResultEntryNum(result, &num) != tirexError::TIREX_SUCCESS)
		abort();
	for (size_t i = 0; i < num; ++i) {
		if (tirexResultEntryGetByIndex(result, i, &entry))
			abort();
		if (filter(entry)) {
			assert(entry.type == TIREX_STRING);
			ret[entry.source] = static_cast<const char*>(entry.value);
		}
	}
}

/**
 * @brief 
 * @details For more information on the specification, refer to https://www.ir metadata.org/metadata/platform/.
 * 
 * @param results 
 * @param stream 
 */
static void writePlatform(const ResultMap& results, std::ostream& stream) {
	stream << "platform:\n";
	stream << "  hardware:\n";

	//// CPU INFO
	stream << "    cpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_MODEL_NAME)) != results.end())
		stream << "      model: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_ARCHITECTURE)) != results.end())
		stream << "      architecture: " << it->second << '\n';
	/*if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_OPMODES)) != results.end())
		stream << "      operation mode: " <<  it->second  << '\n';*/ /** \todo implement **/
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_CORES_PER_SOCKET)) != results.end())
		stream << "      number of cores: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_FEATURES)) != results.end())
		stream << "      features: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_FREQUENCY_MHZ)) != results.end())
		stream << "      frequency: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_FREQUENCY_MIN_MHZ)) != results.end())
		stream << "      frequency min: " << it->second << " MHz\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_FREQUENCY_MAX_MHZ)) != results.end())
		stream << "      frequency max: " << it->second << " MHz\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_VENDOR_ID)) != results.end())
		stream << "      vendor id: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_BYTE_ORDER)) != results.end())
		stream << "      byte order: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_THREADS_PER_CORE)) != results.end())
		stream << "      threads per core: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_CACHES)) != results.end())
		stream << "      caches: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_VIRTUALIZATION)) != results.end())
		stream << "      virtualization: " << it->second << '\n';

	//// GPU INFO
	stream << "    gpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_MODEL_NAME)) != results.end())
		stream << "      model: " << it->second << '\n';
	//if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_DRIVER_VERSION)) != results.end())
	//	stream << "      driver: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB)) != results.end())
		stream << "      memory: " << it->second << " MB\n"; /** \todo ir metadata wants GB */
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_NUM_CORES)) != results.end())
		stream << "      number of cores: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_SUPPORTED)) != results.end())
		stream << "      supported: " << it->second << '\n';

	//// RAM INFO
	if (ResultMap::const_iterator it; (it = results.find(TIREX_RAM_AVAILABLE_SYSTEM_MB)) != results.end())
		stream << "    ram: " << it->second << " MB\n"; /** \todo ir metadata wants GB */

	//// OS INFO
	stream << "  operating system:\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_OS_KERNEL)) != results.end())
		stream << "    kernel: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_OS_NAME)) != results.end())
		stream << "    distribution: " << it->second << '\n';
	stream << "  software: {}\n";
}

/**
 * @brief 
 * @details For more information on the specification, refer to https://www.ir-metadata.org/metadata/implementation/.
 * 
 * @param results 
 * @param stream 
 */
static void writeImplementation(const ResultMap& results, std::ostream& stream) {
	stream << "implementation:\n";
	stream << "  executable:\n";
	//stream << "      cmd:\n"; /** \todo add support **/
	stream << "  source:\n";
	stream << "    lang: null\n"; /** unsupported **/
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_REMOTE_ORIGIN)) != results.end())
		stream << "    repository: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_LAST_COMMIT_HASH)) != results.end())
		stream << "    commit: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_IS_REPO)) != results.end())
		stream << "    is repo: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_HASH)) != results.end())
		stream << "    hash: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_BRANCH)) != results.end())
		stream << "    branch: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_BRANCH_UPSTREAM)) != results.end())
		stream << "    upstream branch: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_TAGS)) != results.end())
		stream << "    tags: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_UNCOMMITTED_CHANGES)) != results.end())
		stream << "    uncommitted changes: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_UNPUSHED_CHANGES)) != results.end())
		stream << "    unpushed changes: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_UNCHECKED_FILES)) != results.end())
		stream << "    unchecked files: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_ROOT)) != results.end())
		stream << "    root: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GIT_ARCHIVE_PATH)) != results.end())
		stream << "    archive path: " << it->second << '\n';
}

static void writeResources(const ResultMap& results, std::ostream& stream) {
	stream << "resources:\n";

	//// RUNTIME DATA
	stream << "  runtime:\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_TIME_START)) != results.end())
		stream << "    start time: \"" << it->second << "\"\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_TIME_STOP)) != results.end())
		stream << "    stop time: \"" << it->second << "\"\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_TIME_ELAPSED_WALL_CLOCK_MS)) != results.end())
		stream << "    wallclock: " << it->second << " ms\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_TIME_ELAPSED_USER_MS)) != results.end())
		stream << "    user: " << it->second << " ms\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_TIME_ELAPSED_SYSTEM_MS)) != results.end())
		stream << "    system: " << it->second << " ms\n";

	//// CPU DATA
	stream << "  cpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_USED_PROCESS_PERCENT)) != results.end())
		stream << "    used process: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_USED_SYSTEM_PERCENT)) != results.end())
		stream << "    used system: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_CPU_ENERGY_SYSTEM_JOULES)) != results.end())
		stream << "    energy used system: " << it->second << " J\n";
	//// GPU DATA
	stream << "  gpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_USED_PROCESS_PERCENT)) != results.end())
		stream << "    used process: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_USED_SYSTEM_PERCENT)) != results.end())
		stream << "    used system: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_VRAM_USED_PROCESS_MB)) != results.end())
		stream << "    vram used process: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_VRAM_USED_SYSTEM_MB)) != results.end())
		stream << "    vram used system: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_GPU_ENERGY_SYSTEM_JOULES)) != results.end())
		stream << "    energy used system: " << it->second << " J\n";
	//// RAM DATA
	stream << "  ram:\n";
	if (ResultMap::const_iterator it; (it = results.find(TIREX_RAM_USED_PROCESS_KB)) != results.end())
		stream << "    used process: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_RAM_USED_SYSTEM_MB)) != results.end())
		stream << "    used system: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(TIREX_RAM_ENERGY_SYSTEM_JOULES)) != results.end())
		stream << "    energy used system: " << it->second << " J\n";
}

// Not static because internally the measurecommand calls this. Not pretty :(
TIREX_TRACKER_EXPORT tirexError
writeIrMetadata(const tirexResult* info, const tirexResult* result, std::ostream& stream) {
	std::string version = "0.2";
	ResultMap map;
	if (info != nullptr)
		asMap(map, info, versionFilter(version));
	if (result != nullptr)
		asMap(map, result, versionFilter(version));

	stream << "schema version: " << version << '\n';
	writePlatform(map, stream);
	writeImplementation(map, stream);
	writeResources(map, stream);
	return tirexError::TIREX_SUCCESS;
}

tirexError tirexResultExportIrMetadata(const tirexResult* info, const tirexResult* result, const char* filepath) {
	std::ofstream stream(filepath);
	if (!stream)
		return tirexError::TIREX_INVALID_ARGUMENT;
	return writeIrMetadata(info, result, stream);
}