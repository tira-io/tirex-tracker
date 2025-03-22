/**
 * @file systemstats.cpp
 * @brief Implements platform agnostic code of the systemstats.hpp header.
 */

#include "systemstats.hpp"

#include "../../logging.hpp"

#include <cpuinfo.h>

#if __cpp_lib_format and __cplusplus >= 202207L // __cplusplus >= 202207L required for std::format_string
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif

#include <bit>
#include <fstream>
#include <map>
#include <sstream>
#include <tuple>

using namespace std::string_literals;

using tirex::Stats;
using tirex::SystemStats;

#ifdef __linux__
extern "C" {
// Not part of the public API but we use them for now until there is a public API for frequency
uint32_t cpuinfo_linux_get_processor_min_frequency(uint32_t processor);
uint32_t cpuinfo_linux_get_processor_max_frequency(uint32_t processor);
}
std::tuple<uint32_t, uint32_t> getProcessorMinMaxFreq(uint32_t processor) {
	return {cpuinfo_linux_get_processor_min_frequency(processor), cpuinfo_linux_get_processor_max_frequency(processor)};
}
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#include <powrprof.h>
#include <windows.h>

std::tuple<uint32_t, uint32_t> getProcessorMinMaxFreq(uint32_t processor) {
	struct PROCESSOR_POWER_INFORMATION {
		ULONG Number;
		ULONG MaxMhz;
		ULONG CurrentMhz;
		ULONG MhzLimit;
		ULONG MaxIdleState;
		ULONG CurrentIdleState;
	};
	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);

	std::vector<PROCESSOR_POWER_INFORMATION> data(si.dwNumberOfProcessors);
	DWORD dwSize = sizeof(PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors;
	CallNtPowerInformation(ProcessorInformation, NULL, 0, &data[0], dwSize);

	return {0, data[processor].MaxMhz};
}
#elif __APPLE__
#include "macos/sysctl.hpp"

std::tuple<uint32_t, uint32_t> getProcessorMinMaxFreq(uint32_t processor) {
	return {0, 0}; /** Not implemented as I don't know of any way to get this information from macOS. **/
}
#endif

const char* SystemStats::version = nullptr;
const std::set<tirexMeasure> SystemStats::measures{
		TIREX_OS_NAME,
		TIREX_OS_KERNEL,

		TIREX_TIME_ELAPSED_WALL_CLOCK_MS,
		TIREX_TIME_ELAPSED_USER_MS,
		TIREX_TIME_ELAPSED_SYSTEM_MS,

		TIREX_CPU_USED_PROCESS_PERCENT,
		TIREX_CPU_USED_SYSTEM_PERCENT,
		TIREX_CPU_AVAILABLE_SYSTEM_CORES,
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
		TIREX_RAM_AVAILABLE_SYSTEM_MB
};

std::map<cpuinfo_vendor, const char*> vendorToStr{
		{cpuinfo_vendor_unknown, "Unknown"},
		{cpuinfo_vendor_intel, "Intel Corporation"},
		{cpuinfo_vendor_amd, "Advanced Micro Devices, Inc."},
		{cpuinfo_vendor_arm, "ARM Holdings plc."},
		{cpuinfo_vendor_qualcomm, "Qualcomm Incorporated"},
		{cpuinfo_vendor_apple, "Apple Inc."},
		{cpuinfo_vendor_samsung, "Samsung Electronics Co., Ltd."},
		{cpuinfo_vendor_nvidia, "Nvidia Corporation"},
		{cpuinfo_vendor_mips, "MIPS Technologies, Inc."},
		{cpuinfo_vendor_ibm, "International Business Machines Corporation"},
		{cpuinfo_vendor_ingenic, "Ingenic Semiconductor"},
		{cpuinfo_vendor_via, "VIA Technologies, Inc."},
		{cpuinfo_vendor_cavium, "Cavium, Inc."},
		{cpuinfo_vendor_broadcom, "Broadcom, Inc."},
		{cpuinfo_vendor_apm, "Applied Micro Circuits Corporation (APM)"},
		{cpuinfo_vendor_huawei, "Huawei Technologies Co., Ltd."},
		{cpuinfo_vendor_hygon, "Hygon (Chengdu Haiguang Integrated Circuit Design Co., Ltd)"},
		{cpuinfo_vendor_sifive, "SiFive, Inc."},
		{cpuinfo_vendor_texas_instruments, "Texas Instruments Inc."},
		{cpuinfo_vendor_marvell, "Marvell Technology Group Ltd."},
		{cpuinfo_vendor_rdc, "RDC Semiconductor Co."},
		{cpuinfo_vendor_dmp, "DM&P Electronics Inc."},
		{cpuinfo_vendor_motorola, "Motorola, Inc."},
		{cpuinfo_vendor_transmeta, "Transmeta Corporation"},
		{cpuinfo_vendor_cyrix, "Cyrix Corporation"},
		{cpuinfo_vendor_rise, "Rise Technology"},
		{cpuinfo_vendor_nsc, "National Semiconductor"},
		{cpuinfo_vendor_sis, "Silicon Integrated Systems"},
		{cpuinfo_vendor_nexgen, "NexGen"},
		{cpuinfo_vendor_umc, "United Microelectronics Corporation"},
		{cpuinfo_vendor_dec, "Digital Equipment Corporation"}
};

static const std::map<uint8_t, const char*> armimplementers = {
		{0x41, "ARM"},		 {0x42, "Broadcom"},  {0x43, "Cavium"},	  {0x44, "DEC"},
		{0x46, "FUJITSU"},	 {0x48, "HiSilicon"}, {0x49, "Infineon"}, {0x4d, "Motorola/Freescale"},
		{0x4e, "NVIDIA"},	 {0x50, "APM"},		  {0x51, "Qualcomm"}, {0x53, "Samsung"},
		{0x56, "Marvell"},	 {0x61, "Apple"},	  {0x66, "Faraday"},  {0x69, "Intel"},
		{0x6d, "Microsoft"}, {0x70, "Phytium"},	  {0xc0, "Ampere"}
};

// Commented out extensions are added in cpuinfo's isa flags but don't have a getter (yet?)
static const std::pair<const char*, bool (*)(void)> flags[]{
		/** x86 Extensions ############################################################################## **/
		{"rdtsc", cpuinfo_has_x86_rdtsc},
		{"rdtscp", cpuinfo_has_x86_rdtscp},
		{"rdpid", cpuinfo_has_x86_rdpid},
		//{"sysenter", cpuinfo_has_x86_sysenter},
		//{"syscall", cpuinfo_has_x86_syscall},
		//{"msr", cpuinfo_has_x86_msr},
		{"clzero", cpuinfo_has_x86_clzero},
		//{"clflush", cpuinfo_has_x86_clflush},
		//{"clflushopt", cpuinfo_has_x86_clflushopt},
		{"mwait", cpuinfo_has_x86_mwait},
		{"mwaitx", cpuinfo_has_x86_mwaitx},
		//{"emmx", cpuinfo_has_x86_emmx},
		{"fxsave", cpuinfo_has_x86_fxsave},
		{"xsave", cpuinfo_has_x86_xsave},
		{"fpu", cpuinfo_has_x86_fpu},
		{"mmx", cpuinfo_has_x86_mmx},
		{"mmx_plus", cpuinfo_has_x86_mmx_plus},
		//{"three_d_now", cpuinfo_has_x86_three_d_now},
		//{"three_d_now_plus", cpuinfo_has_x86_three_d_now_plus},
		//{"three_d_now_geode", cpuinfo_has_x86_three_d_now_geode},
		{"prefetch", cpuinfo_has_x86_prefetch},
		{"prefetchw", cpuinfo_has_x86_prefetchw},
		{"prefetchwt1", cpuinfo_has_x86_prefetchwt1},
		{"daz", cpuinfo_has_x86_daz},
		{"sse", cpuinfo_has_x86_sse},
		{"sse2", cpuinfo_has_x86_sse2},
		{"sse3", cpuinfo_has_x86_sse3},
		{"ssse3", cpuinfo_has_x86_ssse3},
		{"sse4_1", cpuinfo_has_x86_sse4_1},
		{"sse4_2", cpuinfo_has_x86_sse4_2},
		{"sse4a", cpuinfo_has_x86_sse4a},
		{"misaligned_sse", cpuinfo_has_x86_misaligned_sse},
		{"avx", cpuinfo_has_x86_avx},
		{"avxvnni", cpuinfo_has_x86_avxvnni},
		{"fma3", cpuinfo_has_x86_fma3},
		{"fma4", cpuinfo_has_x86_fma4},
		{"xop", cpuinfo_has_x86_xop},
		{"f16c", cpuinfo_has_x86_f16c},
		{"avx2", cpuinfo_has_x86_avx2},
		{"avx512f", cpuinfo_has_x86_avx512f},
		{"avx512pf", cpuinfo_has_x86_avx512pf},
		{"avx512er", cpuinfo_has_x86_avx512er},
		{"avx512cd", cpuinfo_has_x86_avx512cd},
		{"avx512dq", cpuinfo_has_x86_avx512dq},
		{"avx512bw", cpuinfo_has_x86_avx512bw},
		{"avx512vl", cpuinfo_has_x86_avx512vl},
		{"avx512ifma", cpuinfo_has_x86_avx512ifma},
		{"avx512vbmi", cpuinfo_has_x86_avx512vbmi},
		{"avx512vbmi2", cpuinfo_has_x86_avx512vbmi2},
		{"avx512bitalg", cpuinfo_has_x86_avx512bitalg},
		{"avx512vpopcntdq", cpuinfo_has_x86_avx512vpopcntdq},
		{"avx512vnni", cpuinfo_has_x86_avx512vnni},
		{"avx512bf16", cpuinfo_has_x86_avx512bf16},
		{"avx512fp16", cpuinfo_has_x86_avx512fp16},
		{"avx512vp2intersect", cpuinfo_has_x86_avx512vp2intersect},
		{"avx512_4vnniw", cpuinfo_has_x86_avx512_4vnniw},
		{"avx512_4fmaps", cpuinfo_has_x86_avx512_4fmaps},
		{"avx10_1", cpuinfo_has_x86_avx10_1},
		{"avx10_2", cpuinfo_has_x86_avx10_2},
		{"amx_bf16", cpuinfo_has_x86_amx_bf16},
		{"amx_tile", cpuinfo_has_x86_amx_tile},
		{"amx_int8", cpuinfo_has_x86_amx_int8},
		{"amx_fp16", cpuinfo_has_x86_amx_fp16},
		{"avx_vnni_int8", cpuinfo_has_x86_avx_vnni_int8},
		{"avx_vnni_int16", cpuinfo_has_x86_avx_vnni_int16},
		{"avx_ne_convert", cpuinfo_has_x86_avx_ne_convert},
		{"hle", cpuinfo_has_x86_hle},
		{"rtm", cpuinfo_has_x86_rtm},
		{"xtest", cpuinfo_has_x86_xtest},
		{"mpx", cpuinfo_has_x86_mpx},
		{"cmov", cpuinfo_has_x86_cmov},
		{"cmpxchg8b", cpuinfo_has_x86_cmpxchg8b},
		{"cmpxchg16b", cpuinfo_has_x86_cmpxchg16b},
		{"clwb", cpuinfo_has_x86_clwb},
		{"movbe", cpuinfo_has_x86_movbe},
		{"lahf_sahf", cpuinfo_has_x86_lahf_sahf},
		//{"fs_gs_base", cpuinfo_has_x86_fs_gs_base},
		{"lzcnt", cpuinfo_has_x86_lzcnt},
		{"popcnt", cpuinfo_has_x86_popcnt},
		{"tbm", cpuinfo_has_x86_tbm},
		{"bmi", cpuinfo_has_x86_bmi},
		{"bmi2", cpuinfo_has_x86_bmi2},
		{"adx", cpuinfo_has_x86_adx},
		{"aes", cpuinfo_has_x86_aes},
		{"vaes", cpuinfo_has_x86_vaes},
		{"pclmulqdq", cpuinfo_has_x86_pclmulqdq},
		{"vpclmulqdq", cpuinfo_has_x86_vpclmulqdq},
		{"gfni", cpuinfo_has_x86_gfni},
		{"rdrand", cpuinfo_has_x86_rdrand},
		{"rdseed", cpuinfo_has_x86_rdseed},
		{"sha", cpuinfo_has_x86_sha},
		//{"rng", cpuinfo_has_x86_rng},
		//{"ace", cpuinfo_has_x86_ace},
		//{"ace2", cpuinfo_has_x86_ace2},
		//{"phe", cpuinfo_has_x86_phe},
		//{"pmm", cpuinfo_has_x86_pmm},
		//{"lwp", cpuinfo_has_x86_lwp},

		/** ARM Extensions ############################################################################## **/
		{"thumb", cpuinfo_has_arm_thumb},
		{"thumb2", cpuinfo_has_arm_thumb2},
		//{"thumbee", cpuinfo_has_arm_thumbee},
		//{"jazelle", cpuinfo_has_arm_jazelle},
		//{"armv5e", cpuinfo_has_arm_armv5e},
		//{"armv6", cpuinfo_has_arm_armv6},
		//{"armv6k", cpuinfo_has_arm_armv6k},
		//{"armv7", cpuinfo_has_arm_armv7},
		//{"armv7mp", cpuinfo_has_arm_armv7mp},
		//{"armv8", cpuinfo_has_arm_armv8},
		{"idiv", cpuinfo_has_arm_idiv},
		{"vfpv2", cpuinfo_has_arm_vfpv2},
		{"vfpv3", cpuinfo_has_arm_vfpv3},
		//{"d32", cpuinfo_has_arm_d32},
		//{"fp16", cpuinfo_has_arm_fp16},
		//{"fma", cpuinfo_has_arm_fma},
		{"wmmx", cpuinfo_has_arm_wmmx},
		{"wmmx2", cpuinfo_has_arm_wmmx2},
		{"neon", cpuinfo_has_arm_neon},
		{"atomics", cpuinfo_has_arm_atomics},
		{"bf16", cpuinfo_has_arm_bf16},
		{"sve", cpuinfo_has_arm_sve},
		{"sve2", cpuinfo_has_arm_sve2},
		{"i8mm", cpuinfo_has_arm_i8mm},
		{"sme", cpuinfo_has_arm_sme},
		{"sme2", cpuinfo_has_arm_sme2},
		{"sme2p1", cpuinfo_has_arm_sme2p1},
		{"sme_i16i32", cpuinfo_has_arm_sme_i16i32},
		{"sme_bi32i32", cpuinfo_has_arm_sme_bi32i32},
		{"sme_b16b16", cpuinfo_has_arm_sme_b16b16},
		{"sme_f16f16", cpuinfo_has_arm_sme_f16f16},
		//{"rdm", cpuinfo_has_arm_rdm},
		//{"fp16arith", cpuinfo_has_arm_fp16arith},
		//{"dot", cpuinfo_has_arm_dot},
		{"jscvt", cpuinfo_has_arm_jscvt},
		{"fcma", cpuinfo_has_arm_fcma},
		{"fhm", cpuinfo_has_arm_fhm},
		{"aes", cpuinfo_has_arm_aes},
		{"sha1", cpuinfo_has_arm_sha1},
		{"sha2", cpuinfo_has_arm_sha2},
		{"pmull", cpuinfo_has_arm_pmull},
		{"crc32", cpuinfo_has_arm_crc32}
};

/**
 * @brief Returns a textual representation of the implementer as stored in the MIDR register
 * @details https://developer.arm.com/documentation/100442/0100/register-descriptions/aarch32-system-registers/midr--main-id-register
 * 
 * @param midr 
 * @return  
 */
static std::string armImplementerToStr(uint32_t midr) {
	auto implementer = static_cast<uint8_t>(midr >> 24);
	auto it = armimplementers.find(implementer);
	if (it != armimplementers.end())
		return it->second;
	return _fmt::format("Unknown Implementer ({:#04x})", implementer);
}

template <uint32_t SystemStats::CPUInfo::Cache::* entry>
static void aggCaches(SystemStats::CPUInfo::Cache& dest, const cpuinfo_cache* caches, uint32_t num) {
	for (auto i = 0u; i < num; ++i)
		dest.*entry += caches[i].size;
}

static std::vector<SystemStats::CPUInfo::Cache> getCaches() {
	// Assumes cpuinfo to be initialized
	auto cluster = cpuinfo_get_cluster(0);
	SystemStats::CPUInfo::Cache layer[4] = {};
	aggCaches<&SystemStats::CPUInfo::Cache::instruct>(
			layer[0], cpuinfo_get_l1i_caches(), cpuinfo_get_l1i_caches_count()
	);
	aggCaches<&SystemStats::CPUInfo::Cache::data>(layer[0], cpuinfo_get_l1d_caches(), cpuinfo_get_l1d_caches_count());
	aggCaches<&SystemStats::CPUInfo::Cache::unified>(layer[1], cpuinfo_get_l2_caches(), cpuinfo_get_l2_caches_count());
	aggCaches<&SystemStats::CPUInfo::Cache::unified>(layer[2], cpuinfo_get_l3_caches(), cpuinfo_get_l3_caches_count());
	aggCaches<&SystemStats::CPUInfo::Cache::unified>(layer[3], cpuinfo_get_l4_caches(), cpuinfo_get_l4_caches_count());
	return {std::begin(layer), std::end(layer)};
}

static std::string getFlagStr() {
	std::ostringstream stream;
	for (auto&& [name, pred] : flags) {
		if (pred())
			stream << name << " ";
	}
	return stream.str();
}

static SystemStats::CPUInfo::VirtFlags getVirtSupport();
#ifdef __APPLE__
SystemStats::CPUInfo::VirtFlags getVirtSupport() { return {.svm = false, .vmx = false}; }
#elif defined(__linux__)
SystemStats::CPUInfo::VirtFlags getVirtSupport() {
	/** This is a crude implementation for now that only takes into account the flags of the very first processor **/
	std::ifstream is("/proc/cpuinfo");
	for (std::string line; std::getline(is, line);)
		if (line.substr(0, 9) == "Features")
			return {.svm = line.find("svm") != std::string::npos, .vmx = line.find("vmx") != std::string::npos};
	return {.svm = false, .vmx = false};
}
#elif defined(_WIN64)
#include <windows.h>

SystemStats::CPUInfo::VirtFlags getVirtSupport() {
	bool hasvirt = IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED);
	auto cluster = cpuinfo_get_cluster(0);
	return {.svm = hasvirt && (cluster->vendor == cpuinfo_vendor_amd),
			.vmx = hasvirt && (cluster->vendor == cpuinfo_vendor_intel)};
}
#else
#error "getVirtSupport not supported for this OS"
#endif

SystemStats::SystemStats() {}

bool isBigEndian() {
	// Store 0x0001 in memory.
	uint16_t word = 1;
	// Get a pointer to the first byte of the word.
	uint8_t* firstByte = (uint8_t*)&word;
	// Check if the first byte is zero.
	return !(*firstByte);
}

SystemStats::CPUInfo SystemStats::getCPUInfo() {
	cpuinfo_initialize();
	auto numProcessors = cpuinfo_get_processors_count();
	auto numPackages = cpuinfo_get_packages_count();
	auto numCores = cpuinfo_get_cores_count();
	auto numClusters = cpuinfo_get_clusters_count();

	tirex::log::info(
			"system", "Found {} physical and {} logical processors with a total of {} cores clustered into {} clusters",
			numPackages, numProcessors, numCores, numClusters
	);
	if (numClusters > 1) {
		tirex::log::warn(
				"system", "I found that {} CPU clusters are installed and will only output statistics for the first",
				numClusters
		);
	}
	auto cluster = cpuinfo_get_cluster(0);
	auto package = cluster->package;
	auto core = cpuinfo_get_core(0);

	// FIXME: Can we somehow check for mixed endianess without using the std::endian enum?
	std::string endianness = (isBigEndian()) ? "Big Endian" : "Little Endian";

	auto [minFreq, maxFreq] = getProcessorMinMaxFreq(0);

	return CPUInfo{
			.modelname = cluster->package->name,
#if CPUINFO_ARCH_X86 || CPUINFO_ARCH_X86_64
			.vendorId = vendorToStr[cluster->vendor],
#elif CPUINFO_ARCH_ARM || CPUINFO_ARCH_ARM64
			.vendorId = (cluster->vendor != cpuinfo_vendor_unknown) ? vendorToStr[cluster->vendor]
																	: armImplementerToStr(cluster->midr),
#endif
			.numCores = cpuinfo_get_cores_count(),
			.coresPerSocket = package->core_count,
			.threadsPerCore = package->processor_count / package->core_count,
			.caches = getCaches(),
			.endianness = endianness,
			.frequency_min = minFreq,
			.frequency_max = maxFreq,
			.flags = std::move(getFlagStr()),
			.virtualization = getVirtSupport()
	};
}

Stats SystemStats::getInfo() {
	/** \todo: filter by requested metrics */
	auto info = getSysInfo();
	auto cpuInfo = getCPUInfo();

#ifdef __APPLE__
	/** \todo this should not be needed once https://github.com/pytorch/cpuinfo/pull/246/files is merged. **/
	cpuInfo.modelname = getSysctl<std::string>("machdep.cpu.brand_string");
#endif

	std::stringstream cachesStream;
	cachesStream << "{";
	size_t cacheIdx = 1;
	bool first = true;
	for (auto& [unified, instruct, data] : cpuInfo.caches) {
		if (unified) {
			if (!first)
				cachesStream << ",";
			cachesStream << _fmt::format("\"l{}\": {}", cacheIdx, unified);
			first = false;
		}
		if (instruct) {
			if (!first)
				cachesStream << ",";
			cachesStream << _fmt::format("\"l{}i\": {}", cacheIdx, instruct);
			first = false;
		}
		if (data) {
			if (!first)
				cachesStream << ",";
			cachesStream << _fmt::format("\"l{}d\": {}", cacheIdx, data);
			first = false;
		}
		++cacheIdx;
	}
	cachesStream << "}";

	return {{TIREX_OS_NAME, info.osname},
			{TIREX_OS_KERNEL, info.kerneldesc},
			{TIREX_CPU_AVAILABLE_SYSTEM_CORES, std::to_string(cpuInfo.numCores)},
			{TIREX_CPU_FEATURES, cpuInfo.flags},
			{TIREX_CPU_FREQUENCY_MIN_MHZ, std::to_string(cpuInfo.frequency_min)},
			{TIREX_CPU_FREQUENCY_MAX_MHZ, std::to_string(cpuInfo.frequency_max)},
			{TIREX_CPU_VENDOR_ID, cpuInfo.vendorId},
			{TIREX_CPU_BYTE_ORDER, cpuInfo.endianness},
			{TIREX_CPU_ARCHITECTURE, info.architecture},
			{TIREX_CPU_MODEL_NAME, cpuInfo.modelname},
			{TIREX_CPU_CORES_PER_SOCKET, std::to_string(cpuInfo.coresPerSocket)},
			{TIREX_CPU_THREADS_PER_CORE, std::to_string(cpuInfo.threadsPerCore)},
			{TIREX_CPU_CACHES, cachesStream.str()},
			{TIREX_CPU_VIRTUALIZATION,
			 (cpuInfo.virtualization.svm ? "AMD-V "s : ""s) + (cpuInfo.virtualization.vmx ? "VT-x"s : ""s)},
			{TIREX_RAM_AVAILABLE_SYSTEM_MB, std::to_string(info.totalRamMB)}};
}

Stats SystemStats::getStats() {
	/** \todo: filter by requested metrics */
	auto wallclocktime =
			std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stoptime - starttime).count());

	return {
			{{TIREX_TIME_ELAPSED_WALL_CLOCK_MS, wallclocktime},
			 {TIREX_TIME_ELAPSED_USER_MS, std::to_string(tickToMs(stopUTime - startUTime))},
			 {TIREX_TIME_ELAPSED_SYSTEM_MS, std::to_string(tickToMs(stopSysTime - startSysTime))},
			 {TIREX_CPU_USED_PROCESS_PERCENT, cpuUtil},
			 {TIREX_CPU_USED_SYSTEM_PERCENT, sysCpuUtil},
			 {TIREX_CPU_FREQUENCY_MHZ, frequency},
			 {TIREX_RAM_USED_PROCESS_KB, ram},
			 {TIREX_RAM_USED_SYSTEM_MB, sysRam}}
	};
}