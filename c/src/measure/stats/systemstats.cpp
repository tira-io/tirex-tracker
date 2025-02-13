/**
 * @file systemstats.cpp
 * @brief Implements platform agnostic code of the systemstats.hpp header.
 */

#include "systemstats.hpp"

#include "../../logging.hpp"

#include <cpuinfo.h>

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
	auto core = cpuinfo_get_core(0);

	std::string endianness =
			(std::endian::native == std::endian::big)
					? "Big Endian"
					: ((std::endian::native == std::endian::little) ? "Little Endian" : "Mixed Endian");

	return CPUInfo{
			.modelname = cluster->package->name,
			.vendorId = vendorToStr[cluster->vendor],
			.coresPerSocket = cluster->core_count,
			.threadsPerCore = cluster->processor_count / cluster->core_count,
			.endianness = endianness,
			.frequency = core->frequency
	};
}