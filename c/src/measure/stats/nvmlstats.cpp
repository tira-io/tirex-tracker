#include "nvmlstats.hpp"

#include "../../logging.hpp"
#include "../utils/sharedlib.hpp"

#include <nvml/nvml.h>

using namespace std::literals;

using tirex::NVMLStats;
using tirex::Stats;

const char* NVMLStats::version = "nvml v." NVML_API_VERSION_STR;
const std::set<tirexMeasure> NVMLStats::measures{TIREX_GPU_SUPPORTED,			TIREX_GPU_MODEL_NAME,
												 TIREX_GPU_NUM_CORES,			TIREX_GPU_USED_PROCESS_PERCENT,
												 TIREX_GPU_USED_SYSTEM_PERCENT, TIREX_GPU_VRAM_USED_PROCESS_MB,
												 TIREX_GPU_VRAM_USED_SYSTEM_MB, TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB};

struct NVMLLib final : tirex::utils::SharedLib {
public:
	using INIT_V2 = nvmlReturn_t (*)(void);
	INIT_V2 init = load<INIT_V2>({"nvmlInit_v2"});

	using ERROR_STRING = const char* (*)(nvmlReturn_t result);
	ERROR_STRING errorString = load<ERROR_STRING>({"nvmlErrorString"});

	/** SYSTEM */
	using SYSTEM_GET_DRIVER_VERSION = nvmlReturn_t (*)(char* version, unsigned int length);
	SYSTEM_GET_DRIVER_VERSION systemGetDriverVersion = load<SYSTEM_GET_DRIVER_VERSION>({"nvmlSystemGetDriverVersion"});

	/** DEVICE **/
	using DEVICE_GET_COUNT = nvmlReturn_t (*)(unsigned int* deviceCount);
	DEVICE_GET_COUNT deviceGetCount = load<DEVICE_GET_COUNT>({"nvmlDeviceGetCount_v2"});
	using DEVICE_GET_HANDLE_BY_INDEX = nvmlReturn_t (*)(unsigned int index, nvmlDevice_t* device);
	DEVICE_GET_HANDLE_BY_INDEX deviceGetHandleByIndex =
			load<DEVICE_GET_HANDLE_BY_INDEX>({"nvmlDeviceGetHandleByIndex_v2"});
	using DEVICE_GET_ARCHITECTURE = nvmlReturn_t (*)(nvmlDevice_t device, nvmlDeviceArchitecture_t* arch);
	DEVICE_GET_ARCHITECTURE deviceGetArchitecture = load<DEVICE_GET_ARCHITECTURE>({"nvmlDeviceGetArchitecture"});
	using DEVICE_GET_NAME = nvmlReturn_t (*)(nvmlDevice_t device, char* name, unsigned int length);
	DEVICE_GET_NAME deviceGetName = load<DEVICE_GET_NAME>({"nvmlDeviceGetName"});
	using DEVICE_GET_NUM_GPU_CORES = nvmlReturn_t (*)(nvmlDevice_t device, unsigned int* numCores);
	DEVICE_GET_NUM_GPU_CORES deviceGetNumGpuCores = load<DEVICE_GET_NUM_GPU_CORES>({"nvmlDeviceGetNumGpuCores"});
	using NVML_DEVICE_GET_UTILIZATION_RATES = nvmlReturn_t (*)(nvmlDevice_t device, nvmlUtilization_t* utilization);
	NVML_DEVICE_GET_UTILIZATION_RATES deviceGetUtilizationRates =
			load<NVML_DEVICE_GET_UTILIZATION_RATES>({"nvmlDeviceGetUtilizationRates"});
	using DEVICE_GET_MEMORY_INFO = nvmlReturn_t (*)(nvmlDevice_t device, nvmlMemory_t* memory);
	DEVICE_GET_MEMORY_INFO deviceGetMemoryInfo = load<DEVICE_GET_MEMORY_INFO>({"nvmlDeviceGetMemoryInfo"});

	// nvmlDeviceGetProcessUtilization

	// using DEVICE_GET_COMPUTE_RUNNING_PROCESSES =	nvmlReturn_t (*)(nvmlDevice_t device, unsigned int* infoCount, nvmlProcessInfo_t* infos);
	// DEVICE_GET_COMPUTE_RUNNING_PROCESSES deviceGetComputeRunningProcesses = oad<DEVICE_GET_COMPUTE_RUNNING_PROCESSES>({"nvmlDeviceGetComputeRunningProcesses_v3"});

	using DEVICE_GET_RUNNING_PROCESS_DETAIL_LIST =
			nvmlReturn_t (*)(nvmlDevice_t device, nvmlProcessDetailList_t* plist);
	DEVICE_GET_RUNNING_PROCESS_DETAIL_LIST deviceGetRunningProcessDetailList =
			load<DEVICE_GET_RUNNING_PROCESS_DETAIL_LIST>({"nvmlDeviceGetRunningProcessDetailList"});

#if defined(__linux__)
	NVMLLib() : tirex::utils::SharedLib("libnvidia-ml.so.1") {}
#elif defined(__APPLE__)
	/* "MacOS is not supported to fetch NVIDIA GPU information */
	NVMLLib() : tirex::utils::SharedLib() {}
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
	NVMLLib() : tirex::utils::SharedLib("nvml.dll") {}
#else
#error "Unsupported OS"
#endif
} nvml;

static const char* nvmlArchToStr(nvmlDeviceArchitecture_t arch) {
	switch (arch) {
	case NVML_DEVICE_ARCH_KEPLER:
		return "Kepler";
	case NVML_DEVICE_ARCH_MAXWELL:
		return "Maxwell";
	case NVML_DEVICE_ARCH_PASCAL:
		return "Pascal";
	case NVML_DEVICE_ARCH_VOLTA:
		return "Volta";
	case NVML_DEVICE_ARCH_TURING:
		return "Turing";
	case NVML_DEVICE_ARCH_AMPERE:
		return "Ampere";
	case NVML_DEVICE_ARCH_ADA:
		return "Ada";
	case NVML_DEVICE_ARCH_HOPPER:
		return "Hopper";
	}
	return "Unknown";
}

static bool initNVML() {
	if (!nvml.good())
		return false;
	switch (nvml.init()) {
	case NVML_SUCCESS:
		char buf[80];
		nvml.systemGetDriverVersion(buf, sizeof(buf) - 1);
		tirex::log::info("gpustats", "NVML was loaded successfully with driver version {}", buf);
		return true;
	case NVML_ERROR_DRIVER_NOT_LOADED:
		tirex::log::warn("gpustats", "No NVIDIA driver is running");
		break;
	case NVML_ERROR_NO_PERMISSION:
		tirex::log::error("gpustats", "I don't have permission to talk to the driver");
		break;
	default:
		break;
	}
	return false;
}

NVMLStats::NVMLStats() : nvml({.supported = initNVML(), .devices = {}}) {
	if (!nvml.supported)
		return;
	unsigned int count;
	switch (nvmlReturn_t err; err = ::nvml.deviceGetCount(&count)) {
	case NVML_SUCCESS:
		tirex::log::info("gpustats", "Found {} device(s):", count);
		for (unsigned i = 0u; i < count; ++i) {
			nvmlDevice_t device;
			switch (nvmlReturn_t ret; ret = ::nvml.deviceGetHandleByIndex(i, &device)) {
			case NVML_SUCCESS:
				nvmlDeviceArchitecture_t arch;
				::nvml.deviceGetArchitecture(device, &arch);
				char name[96];
				::nvml.deviceGetName(device, name, sizeof(name) - 1);
				tirex::log::info("gpustats", "\t[{}] {} ({} Architecture)", i, name, nvmlArchToStr(arch));
				nvml.devices.emplace_back(device);
				break;
			default:
				tirex::log::error(
						"gpustats", "\t[{}] fetching handle failed with error {}", i, ::nvml.errorString(ret)
				);
				break;
			}
		}
		break;
	default:
		tirex::log::error("gpustats", "Fetching devices failed with error {}", ::nvml.errorString(err));
		break;
	}
}

void NVMLStats::step() {
	if (!nvml.supported)
		return;
	nvmlMemory_t memory;
	/** \todo support multi-gpu **/
	for (auto device : nvml.devices) {
		if (nvmlReturn_t ret; (ret = ::nvml.deviceGetMemoryInfo(device, &memory)) == NVML_SUCCESS) {
			nvml.vramUsageTotal.addValue(memory.used / 1000 / 1000);
		} else {
			tirex::log::critical("gpustats", "Could not fetch memory information: {}", ::nvml.errorString(ret));
			abort(); /** \todo how to handle? **/
		}
		nvmlUtilization_t util;
		if (nvmlReturn_t ret; (ret = ::nvml.deviceGetUtilizationRates(device, &util)) == NVML_SUCCESS) {
			nvml.utilizationTotal.addValue(util.gpu);
		} else {
			tirex::log::critical("gpustats", "Could not fetch utilization information: {}", ::nvml.errorString(ret));
			abort(); /** \todo how to handle? **/
		}
	}

	/*unsigned int processCount;
	for (auto device : nvml.devices) {
		// Get the list of running processes
		nvmlProcessDetailList_t list;
		list.version = nvmlProcessDetailList_v1;
		list.numProcArrayEntries = 0;
		list.procArray = nullptr;
		auto result = ::nvml.deviceGetRunningProcessDetailList(device, &list);
		if (result == NVML_SUCCESS) {
			std::cout << "No processes running on GPU" << std::endl;
			continue;
		} else if (result == NVML_ERROR_INSUFFICIENT_SIZE) {
			std::vector<nvmlProcessDetail_v1_t> buf;
			buf.resize(list.numProcArrayEntries);
			list.procArray = buf.data();
			result = ::nvml.deviceGetRunningProcessDetailList(device, &list);
			for (auto element : buf) {
				std::cout << "PID: " << element.pid << " Mem: " << element.usedGpuMemory << std::endl;
			}
		} else {
			std::cout << "Error: " << ::nvml.errorString(result) << std::endl;
		}
	}*/
}

std::set<tirexMeasure> NVMLStats::providedMeasures() noexcept { return measures; }

Stats NVMLStats::getStats() {
	if (nvml.supported) {
		return makeFilteredStats(
				enabled, std::pair{TIREX_GPU_USED_PROCESS_PERCENT, "TODO"s},
				std::pair{TIREX_GPU_USED_SYSTEM_PERCENT, std::cref(nvml.utilizationTotal)},
				std::pair{TIREX_GPU_VRAM_USED_PROCESS_MB, "TODO"s},
				std::pair{TIREX_GPU_VRAM_USED_SYSTEM_MB, std::cref(nvml.vramUsageTotal)}
		);
	} else {
		return {};
	}
}
Stats NVMLStats::getInfo() {
	if (nvml.supported) {
		std::string modelName;
		std::string vramTotal;
		std::string cores;
		for (auto& device : nvml.devices) {
			// Device Info
			char name[96];
			unsigned int ncores;
			if (nvmlReturn_t ret; (ret = ::nvml.deviceGetName(device, name, sizeof(name) - 1)) == NVML_SUCCESS) {
				modelName += std::string(name) + ",";
			} else {
				modelName += "<error>,";
			}
			if (nvmlReturn_t ret; (ret = ::nvml.deviceGetNumGpuCores(device, &ncores)) == NVML_SUCCESS) {
				cores += std::to_string(ncores) + ",";
			} else {
				cores += "<error>,";
			}
			// Memory Info
			nvmlMemory_t memory;
			if (nvmlReturn_t ret; (ret = ::nvml.deviceGetMemoryInfo(device, &memory)) == NVML_SUCCESS) {
				vramTotal += std::to_string(memory.total / 1000 / 1000) + ",";
			} else {
				vramTotal += "<error>,";
			}
		}

		return makeFilteredStats(
				enabled, std::pair{TIREX_GPU_SUPPORTED, "1"s}, std::pair{TIREX_GPU_MODEL_NAME, modelName},
				std::pair{TIREX_GPU_NUM_CORES, cores}, std::pair{TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB, vramTotal}
		);
	} else {
		return makeFilteredStats(enabled, std::pair{TIREX_GPU_SUPPORTED, "0"s});
	}
}