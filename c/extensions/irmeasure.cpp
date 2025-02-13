#include "irmeasure.h"

#include <cassert>
#include <fstream>
#include <functional>
#include <map>
#include <set>

using ResultMap = std::map<msrMeasure, std::string>;

static const std::map<std::string, std::set<msrMeasure>> measuresPerVersion{
		{"0.1",
		 {MSR_CPU_MODEL_NAME, MSR_CPU_ARCHITECTURE, MSR_CPU_CORES_PER_SOCKET, MSR_GPU_MODEL_NAME,
		  MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB, MSR_GPU_NUM_CORES, MSR_OS_KERNEL, MSR_OS_NAME, MSR_GIT_REMOTE_ORIGIN,
		  MSR_GIT_LAST_COMMIT_HASH}},
		{"0.2",
		 {MSR_OS_NAME,
		  MSR_OS_KERNEL,
		  MSR_TIME_ELAPSED_WALL_CLOCK_MS,
		  MSR_TIME_ELAPSED_USER_MS,
		  MSR_TIME_ELAPSED_SYSTEM_MS,
		  MSR_CPU_USED_PROCESS_PERCENT,
		  MSR_CPU_USED_SYSTEM_PERCENT,
		  MSR_CPU_AVAILABLE_SYSTEM_CORES,
		  MSR_CPU_ENERGY_SYSTEM_JOULES,
		  MSR_CPU_FEATURES,
		  MSR_CPU_FREQUENCY_MHZ,
		  MSR_CPU_FREQUENCY_MIN_MHZ,
		  MSR_CPU_FREQUENCY_MAX_MHZ,
		  MSR_CPU_VENDOR_ID,
		  MSR_CPU_BYTE_ORDER,
		  MSR_CPU_ARCHITECTURE,
		  MSR_CPU_MODEL_NAME,
		  MSR_CPU_CORES_PER_SOCKET,
		  MSR_CPU_THREADS_PER_CORE,
		  MSR_CPU_CACHES_KB,
		  MSR_CPU_VIRTUALIZATION,
		  MSR_RAM_USED_PROCESS_KB,
		  MSR_RAM_USED_SYSTEM_MB,
		  MSR_RAM_AVAILABLE_SYSTEM_MB,
		  MSR_RAM_ENERGY_SYSTEM_JOULES,
		  MSR_GPU_SUPPORTED,
		  MSR_GPU_MODEL_NAME,
		  MSR_GPU_NUM_CORES,
		  MSR_GPU_USED_PROCESS_PERCENT,
		  MSR_GPU_USED_SYSTEM_PERCENT,
		  MSR_GPU_AVAILABLE_SYSTEM_CORES,
		  MSR_GPU_VRAM_USED_PROCESS_MB,
		  MSR_GPU_VRAM_USED_SYSTEM_MB,
		  MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB,
		  MSR_GPU_ENERGY_SYSTEM_JOULES,
		  MSR_GIT_IS_REPO,
		  MSR_GIT_HASH,
		  MSR_GIT_LAST_COMMIT_HASH,
		  MSR_GIT_BRANCH,
		  MSR_GIT_TAGS,
		  MSR_GIT_REMOTE_ORIGIN,
		  MSR_GIT_UNCOMMITTED_CHANGES,
		  MSR_GIT_UNPUSHED_CHANGES,
		  MSR_GIT_UNCHECKED_FILES}}
};

static std::function<bool(const msrResultEntry&)> versionFilter(const std::string& version) {
	const auto& measures = measuresPerVersion.at(version);
	return [measures](const msrResultEntry& entry) { return measures.find(entry.source) != measures.end(); };
}

/** \todo this file *needs* to be refactored! */

/**
 * @brief Convers the measure results into a std::map.
 * 
 * @param result 
 * @return
 */
template <typename C>
static ResultMap asMap(const msrResult* result, C&& filter) {
	ResultMap ret;
	size_t num;
	msrResultEntry entry;
	if (msrResultEntryNum(result, &num) != msrError::MSR_SUCCESS)
		abort();
	for (size_t i = 0; i < num; ++i) {
		if (msrResultEntryGetByIndex(result, i, &entry))
			abort();
		if (filter(entry)) {
			assert(entry.type == MSR_STRING);
			ret[entry.source] = static_cast<const char*>(entry.value);
		}
	}
	return ret;
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
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_MODEL_NAME)) != results.end())
		stream << "      model: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_ARCHITECTURE)) != results.end())
		stream << "      architecture: " << it->second << '\n';
	/*if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_OPMODES)) != results.end())
		stream << "      operation mode: " <<  it->second  << '\n';*/ /** \todo implement **/
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_CORES_PER_SOCKET)) != results.end())
		stream << "      number of cores: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_FEATURES)) != results.end())
		stream << "      features: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_FREQUENCY_MHZ)) != results.end())
		stream << "      frequency: " << it->second << " MHz\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_FREQUENCY_MIN_MHZ)) != results.end())
		stream << "      frequency min: " << it->second << " MHz\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_FREQUENCY_MAX_MHZ)) != results.end())
		stream << "      frequency max: " << it->second << " MHz\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_VENDOR_ID)) != results.end())
		stream << "      vendor id: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_BYTE_ORDER)) != results.end())
		stream << "      byte order: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_THREADS_PER_CORE)) != results.end())
		stream << "      threads per core: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_CACHES_KB)) != results.end())
		stream << "      caches: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_VIRTUALIZATION)) != results.end())
		stream << "      virtualization: " << it->second << '\n';

	//// GPU INFO
	stream << "    gpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_MODEL_NAME)) != results.end())
		stream << "      model: " << it->second << '\n';
	//if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_DRIVER_VERSION)) != results.end())
	//	stream << "      driver: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB)) != results.end())
		stream << "      memory: " << it->second << " MB\n"; /** \todo ir metadata wants GB */
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_NUM_CORES)) != results.end())
		stream << "      number of cores: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_SUPPORTED)) != results.end())
		stream << "      supported: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_AVAILABLE_SYSTEM_CORES)) != results.end())
		stream << "      available sytem cores: " << it->second << '\n';

	//// RAM INFO
	if (ResultMap::const_iterator it; (it = results.find(MSR_RAM_AVAILABLE_SYSTEM_MB)) != results.end())
		stream << "    ram: " << it->second << " MB\n"; /** \todo ir metadata wants GB */

	//// OS INFO
	stream << "  operating system:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_OS_KERNEL)) != results.end())
		stream << "    kernel: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_OS_NAME)) != results.end())
		stream << "    distribution: " << it->second << '\n';
	stream << "  software: {}\n";
}

/**
 * @brief 
 * @details For more information on the specification, refer to https://www.ir metadata.org/metadata/implementation/.
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
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_REMOTE_ORIGIN)) != results.end())
		stream << "    repository: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_LAST_COMMIT_HASH)) != results.end())
		stream << "    commit: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_IS_REPO)) != results.end())
		stream << "    is repo: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_HASH)) != results.end())
		stream << "    hash: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_BRANCH)) != results.end())
		stream << "    branch: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_TAGS)) != results.end())
		stream << "    tags: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_UNCOMMITTED_CHANGES)) != results.end())
		stream << "    uncommitted changes: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_UNPUSHED_CHANGES)) != results.end())
		stream << "    unpushed changes: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_UNCHECKED_FILES)) != results.end())
		stream << "    unchecked files: " << it->second << '\n';
}

static void writeResources(const ResultMap& results, std::ostream& stream) {
	stream << "resources:\n";

	//// RUNTIME DATA
	stream << "  runtime:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_TIME_ELAPSED_WALL_CLOCK_MS)) != results.end())
		stream << "    wallclock: " << it->second << " ms\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_TIME_ELAPSED_USER_MS)) != results.end())
		stream << "    user: " << it->second << " ms\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_TIME_ELAPSED_SYSTEM_MS)) != results.end())
		stream << "    system: " << it->second << " ms\n";

	//// CPU DATA
	stream << "  cpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_USED_PROCESS_PERCENT)) != results.end())
		stream << "    used process: " << it->second << " %\n";
	stream << "    used system:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_USED_SYSTEM_PERCENT)) != results.end()) {
		stream << "      max: " << it->second << " %\n";
		stream << "      avg: " << it->second << " %\n"; /** \todo implement **/
		stream << "      min: " << it->second << " %\n"; /** \todo implement **/
		stream << "      timeseries:\n";
		stream << "        max: [12,25,40,9]\n";
		stream << "        avg: [16,69,76,8]\n";
		stream << "        min: [15,68,73,7]\n";
		stream << "        timestamps_ms: [0, 100, 200, 300]\n"; /** \todo implement **/
	}
	//// GPU DATA
	stream << "  gpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_USED_PROCESS_PERCENT)) != results.end()) {
		stream << "    used process: " << it->second << " %\n";
		stream << "      max: " << it->second << " %\n";
		stream << "      avg: " << it->second << " %\n"; /** \todo implement **/
		stream << "      min: " << it->second << " %\n"; /** \todo implement **/
		stream << "      timeseries:\n";
		stream << "        max: [12,25,40,9]\n";
		stream << "        avg: [16,69,76,8]\n";
		stream << "        min: [15,68,73,7]\n";
		stream << "        timestamps_ms: [0, 100, 200, 300]\n"; /** \todo implement **/
	}
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_USED_SYSTEM_PERCENT)) != results.end()) {
		stream << "    used system: " << it->second << " %\n";
		stream << "      max: " << it->second << " %\n";
		stream << "      avg: " << it->second << " %\n"; /** \todo implement **/
		stream << "      min: " << it->second << " %\n"; /** \todo implement **/
		stream << "      timeseries:\n";
		stream << "        max: [12,25,40,9]\n";
		stream << "        avg: [16,69,76,8]\n";
		stream << "        min: [15,68,73,7]\n";
		stream << "        timestamps_ms: [0, 100, 200, 300]\n"; /** \todo implement **/
	}
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_VRAM_USED_PROCESS_MB)) != results.end()) {
		stream << "    vram used process: " << it->second << " MB\n";
		stream << "      max: " << it->second << " %\n";
		stream << "      avg: " << it->second << " %\n"; /** \todo implement **/
		stream << "      min: " << it->second << " %\n"; /** \todo implement **/
		stream << "      timeseries:\n";
		stream << "        max: [12,25,40,9]\n";
		stream << "        avg: [16,69,76,8]\n";
		stream << "        min: [15,68,73,7]\n";
		stream << "        timestamps_ms: [0, 100, 200, 300]\n"; /** \todo implement **/
	}
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_VRAM_USED_SYSTEM_MB)) != results.end()) {
		stream << "    vram used system: " << it->second << " MB\n";
		stream << "      max: " << it->second << " %\n";
		stream << "      avg: " << it->second << " %\n"; /** \todo implement **/
		stream << "      min: " << it->second << " %\n"; /** \todo implement **/
		stream << "      timeseries:\n";
		stream << "        max: [12,25,40,9]\n";
		stream << "        avg: [16,69,76,8]\n";
		stream << "        min: [15,68,73,7]\n";
		stream << "        timestamps_ms: [0, 100, 200, 300]\n"; /** \todo implement **/
	}
	//// RAM DATA
	stream << "  ram:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_RAM_USED_PROCESS_KB)) != results.end()) {
		stream << "    used process: " << it->second << " KB\n";
		stream << "      max: " << it->second << " %\n";
		stream << "      avg: " << it->second << " %\n"; /** \todo implement **/
		stream << "      min: " << it->second << " %\n"; /** \todo implement **/
		stream << "      timeseries:\n";
		stream << "        max: [12,25,40,9]\n";
		stream << "        avg: [16,69,76,8]\n";
		stream << "        min: [15,68,73,7]\n";
		stream << "        timestamps_ms: [0, 100, 200, 300]\n"; /** \todo implement **/
	}
	if (ResultMap::const_iterator it; (it = results.find(MSR_RAM_USED_SYSTEM_MB)) != results.end()) {
		stream << "    used system: " << it->second << " MB\n";
		stream << "      max: " << it->second << " %\n";
		stream << "      avg: " << it->second << " %\n"; /** \todo implement **/
		stream << "      min: " << it->second << " %\n"; /** \todo implement **/
		stream << "      timeseries:\n";
		stream << "        max: [12,25,40,9]\n";
		stream << "        avg: [16,69,76,8]\n";
		stream << "        min: [15,68,73,7]\n";
		stream << "        timestamps_ms: [0, 100, 200, 300]\n"; /** \todo implement **/
	}
}

msrError msrResultExportIrMetadata(const msrResult* result, const char* filepath) {
	std::string version = "0.2";
	auto results = asMap(result, versionFilter(version));

	std::ofstream stream(filepath);
	if (!stream)
		return msrError::MSR_INVALID_ARGUMENT;
	stream << "ir_metadata.start\n";
	stream << "schema version: " << version << '\n';
	writePlatform(results, stream);
	writeImplementation(results, stream);
	writeResources(results, stream);
	stream << "ir_metadata.end\n";
	return msrError::MSR_SUCCESS;
}