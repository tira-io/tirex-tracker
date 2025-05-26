#include "amdstats.hpp"

#include <amd_smi/amdsmi.h>

#if __cpp_lib_format and __cplusplus >= 202207L // __cplusplus >= 202207L required for std::format_string
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif

#include <iostream> // TODO: remove

using tirex::AMDStats;
using tirex::Stats;

static const std::string tmp = _fmt::format("amd-smi v.{}", AMDSMI_LIB_VERSION_STRING);
const char* AMDStats::version = tmp.c_str();
const std::set<tirexMeasure> AMDStats::measures{TIREX_GPU_SUPPORTED,		   TIREX_GPU_MODEL_NAME,
												TIREX_GPU_NUM_CORES,		   TIREX_GPU_USED_PROCESS_PERCENT,
												TIREX_GPU_USED_SYSTEM_PERCENT, TIREX_GPU_VRAM_USED_PROCESS_MB,
												TIREX_GPU_VRAM_USED_SYSTEM_MB, TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB};

AMDStats::AMDStats() {
	// Some testode for now: todo: remove me pls
	amdsmi_status_t ret;
	amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	uint32_t socket_count = 0;

	// Get the socket count available in the system.
	ret = amdsmi_get_socket_handles(&socket_count, nullptr);
	std::vector<amdsmi_socket_handle> sockets(socket_count);
	// Get the socket handles in the system
	ret = amdsmi_get_socket_handles(&socket_count, &sockets[0]);

	std::cout << "Total Socket: " << socket_count << std::endl;

	// For each socket, get identifier and devices
	for (uint32_t i = 0; i < socket_count; i++) {
		// Get Socket info
		char socket_info[128];
		ret = amdsmi_get_socket_info(sockets[i], 128, socket_info);
		std::cout << "Socket " << socket_info << std::endl;

		// Get the device count for the socket.
		uint32_t device_count = 0;
		ret = amdsmi_get_processor_handles(sockets[i], &device_count, nullptr);

		// Allocate the memory for the device handlers on the socket
		std::vector<amdsmi_processor_handle> processor_handles(device_count);
		// Get all devices of the socket
		ret = amdsmi_get_processor_handles(sockets[i], &device_count, processor_handles.data());

		// For each device of the socket, get name and temperature.
		for (uint32_t j = 0; j < device_count; j++) {
			// Get device type. Since the amdsmi is initialized with
			// AMD_SMI_INIT_AMD_GPUS, the processor_type must be AMDSMI_PROCESSOR_TYPE_AMD_GPU.
			processor_type_t processor_type;
			ret = amdsmi_get_processor_type(processor_handles[j], &processor_type);
			if (processor_type != AMDSMI_PROCESSOR_TYPE_AMD_GPU) {
				std::cout << "Expect AMDSMI_PROCESSOR_TYPE_AMD_GPU device type!\n";
				return;
			}

			// Get device name
			amdsmi_board_info_t board_info;
			ret = amdsmi_get_gpu_board_info(processor_handles[j], &board_info);
			std::cout << "\tdevice " << j << "\n\t\tName:" << board_info.product_name << std::endl;

			// Get temperature
			int64_t val_i64 = 0;
			ret = amdsmi_get_temp_metric(
					processor_handles[j], AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CURRENT, &val_i64
			);
			std::cout << "\t\tTemperature: " << val_i64 << "C" << std::endl;
		}
	}

	// Clean up resources allocated at amdsmi_init. It will invalidate sockets
	// and devices pointers
	ret = amdsmi_shut_down();
}

std::set<tirexMeasure> AMDStats::providedMeasures() noexcept { /** \todo implement **/ return {}; }
void AMDStats::step() { /** \todo implement **/ }
Stats AMDStats::getStats() { /** \todo implement **/ return {}; }
Stats AMDStats::getInfo() { /** \todo implement **/ return {}; }