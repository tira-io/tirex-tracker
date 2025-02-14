/**
 * @file systemstats.cpp
 * @brief Implements platform agnostic code of the systemstats.hpp header.
 */

#include "systemstats.hpp"

#include "../../logging.hpp"

#include <cpuinfo.h>

#if __cpp_lib_format
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif

#include <map>

using msr::SystemStats;

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

template <uint32_t SystemStats::CPUInfo::Cache::*entry>
static void aggCaches(SystemStats::CPUInfo::Cache& dest, const cpuinfo_cache* caches, uint32_t num) {
	for (auto i = 0; i < num; ++i)
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

SystemStats::CPUInfo SystemStats::getCPUInfo() {
	cpuinfo_initialize();
	auto numProcessors = cpuinfo_get_processors_count();
	auto numPackages = cpuinfo_get_packages_count();
	auto numCores = cpuinfo_get_cores_count();
	auto numClusters = cpuinfo_get_clusters_count();
	msr::log::info(
			"system", "Found {} physical and {} logical processors with a total of {} cores clustered into {} clusters",
			numPackages, numProcessors, numCores, numClusters
	);
	if (numClusters > 1) {
		msr::log::warn(
				"system", "I found that {} CPU clusters are installed and will only output statistics for the first",
				numClusters
		);
	}
	auto cluster = cpuinfo_get_cluster(0);
	auto package = cluster->package;
	auto core = cpuinfo_get_core(0);

	std::string endianness =
			(std::endian::native == std::endian::big)
					? "Big Endian"
					: ((std::endian::native == std::endian::little) ? "Little Endian" : "Mixed Endian");

	return CPUInfo{
			.modelname = cluster->package->name,
#if CPUINFO_ARCH_X86 || CPUINFO_ARCH_X86_64
			.vendorId = vendorToStr[cluster->vendor],
#elif CPUINFO_ARCH_ARM || CPUINFO_ARCH_ARM64
			.vendorId = armImplementerToStr(cluster->midr),
#endif
			.coresPerSocket = package->core_count,
			.threadsPerCore = package->processor_count / package->core_count,
			.caches = getCaches(),
			.endianness = endianness,
			.frequency = core->frequency
	};
}