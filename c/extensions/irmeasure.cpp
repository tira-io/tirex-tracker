#include "irmeasure.h"

#include <cassert>
#include <fstream>
#include <map>

using ResultMap = std::map<msrMeasure, std::string>;

/** \todo this file *needs* to be refactored! */

/**
 * @brief Convers the measure results into a std::map.
 * 
 * @param result 
 * @return
 */
static ResultMap asMap(const msrResult* result) {
	ResultMap ret;
	size_t num;
	msrResultEntry entry;
	if (msrResultEntryNum(result, &num) != msrError::MSR_SUCCESS)
		abort();
	for (size_t i = 0; i < num; ++i) {
		if (msrResultEntryGetByIndex(result, i, &entry))
			abort();
		assert(entry.type == MSR_STRING);
		ret[entry.source] = static_cast<const char*>(entry.value);
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
	stream << "    cpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_MODEL_NAME)) != results.end())
		stream << "      model: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_ARCHITECTURE)) != results.end())
		stream << "      architecture: " << it->second << '\n';
	/*if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_OPMODES)) != results.end())
		stream << "      operation mode: " <<  it->second  << '\n';*/ /** \todo implement **/
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_CORES_PER_SOCKET)) != results.end())
		stream << "      number of cores: " << it->second << '\n';
	stream << "    gpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_MODEL_NAME)) != results.end())
		stream << "      model: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB)) != results.end())
		stream << "      memory: " << it->second << " MB\n"; /** \todo ir metadata wants GB */
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_NUM_CORES)) != results.end())
		stream << "      number of cores: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_RAM_AVAILABLE_SYSTEM_MB)) != results.end())
		stream << "    ram: " << it->second << " MB\n"; /** \todo ir metadata wants GB */
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
}

static void writeSystemData(const ResultMap& results, std::ostream& stream) {
	stream << "system:\n";
	stream << "  runtime:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_TIME_ELAPSED_WALL_CLOCK_MS)) != results.end())
		stream << "  wallclock: " << it->second << " ms\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_TIME_ELAPSED_USER_MS)) != results.end())
		stream << "  user: " << it->second << " ms\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_TIME_ELAPSED_SYSTEM_MS)) != results.end())
		stream << "  system: " << it->second << " ms\n";
}
static void writeCPUData(const ResultMap& results, std::ostream& stream) {
	stream << "cpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_USED_PROCESS_PERCENT)) != results.end())
		stream << "  used process: " << it->second << " %\n";
	stream << "  used system:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_USED_SYSTEM_PERCENT)) != results.end())
		stream << "    max: " << it->second << " %\n";
	stream << "    timeseries: "
		   << "[16,69,76,12,66,33,13,85,71,41,95,30,49,31,91,35,38,68,8,80,47,9,45,6,20,70,97,74,63,1,3,5,56,82,22,77,"
			  "34,40,28,10,79,90,89,52,61,26,55,81,15,23,72,93,67,42,53,43,37,36,21,60,11,0,48,51,92,29,24,98,65,84,27,"
			  "2,78,14,46,57,100,25,96,88,58,62,94,73,18,50,59,32,39,83,54,44,19,64,86,17,87,7,99,4]"
		   << '\n'; /** \todo implement **/
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_FEATURES)) != results.end())
		stream << "  features: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_FREQUENCY_MHZ)) != results.end())
		stream << "  frequency: " << it->second << " MHz\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_FREQUENCY_MIN_MHZ)) != results.end())
		stream << "  frequency min: " << it->second << " MHz\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_FREQUENCY_MAX_MHZ)) != results.end())
		stream << "  frequency max: " << it->second << " MHz\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_VENDOR_ID)) != results.end())
		stream << "  vendor id: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_BYTE_ORDER)) != results.end())
		stream << "  byte order: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_THREADS_PER_CORE)) != results.end())
		stream << "  threads per core: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_CACHES_L1_KB)) != results.end())
		stream << "  caches l1 kb: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_CACHES_L2_KB)) != results.end())
		stream << "  caches l2 kb: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_CACHES_L3_KB)) != results.end())
		stream << "  caches l3 kb: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_VIRTUALIZATION)) != results.end())
		stream << "  virtualization: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_CPU_BOGO_MIPS)) != results.end())
		stream << "  bogo mips: " << it->second << '\n';
}
static void writeRAMData(const ResultMap& results, std::ostream& stream) {
	stream << "ram:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_RAM_USED_PROCESS_KB)) != results.end())
		stream << "  used process: " << it->second << " KB\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_RAM_USED_SYSTEM_MB)) != results.end())
		stream << "  used system: " << it->second << " MB\n";
}
static void writeGPUData(const ResultMap& results, std::ostream& stream) {
	stream << "gpu:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_SUPPORTED)) != results.end())
		stream << "  supported: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_USED_PROCESS_PERCENT)) != results.end())
		stream << "  used process: " << it->second << " %\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_USED_SYSTEM_PERCENT)) != results.end())
		stream << "  used system: " << it->second << " %\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_AVAILABLE_SYTEM_CORES)) != results.end())
		stream << "  available sytem cores: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_VRAM_USED_PROCESS_MB)) != results.end())
		stream << "  vram used process: " << it->second << " MB\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_GPU_VRAM_USED_SYSTEM_MB)) != results.end())
		stream << "  vram used system: " << it->second << " MB\n";
}
static void writeGitData(const ResultMap& results, std::ostream& stream) {
	stream << "git:\n";
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_IS_REPO)) != results.end())
		stream << "  is repo: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_HASH)) != results.end())
		stream << "  hash: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_BRANCH)) != results.end())
		stream << "  branch: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_TAGS)) != results.end())
		stream << "  tags: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_UNCOMMITTED_CHANGES)) != results.end())
		stream << "  uncommitted changes: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_UNPUSHED_CHANGES)) != results.end())
		stream << "  unpushed changes: " << it->second << '\n';
	if (ResultMap::const_iterator it; (it = results.find(MSR_GIT_UNCHECKED_FILES)) != results.end())
		stream << "  unchecked files: " << it->second << '\n';
}

msrError msrResultExportIrMetadata(const msrResult* result, const char* filepath) {
	auto results = asMap(result);

	std::ofstream stream(filepath);
	if (!stream)
		return msrError::MSR_INVALID_ARGUMENT;
	stream << "ir_metadata.start\n";
	stream << "schema version: 0.1\n";
	writePlatform(results, stream);
	writeImplementation(results, stream);
	stream << "ir_metadata.end\n";
	stream << "measures.start\n";
	stream << "schema version: 0.1\n";
	writeSystemData(results, stream);
	writeCPUData(results, stream);
	writeRAMData(results, stream);
	writeGPUData(results, stream);
	writeGitData(results, stream);
	stream << "measures.end\n";
	return msrError::MSR_SUCCESS;
}