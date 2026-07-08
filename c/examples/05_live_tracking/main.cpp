#include <tirex_tracker.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <nlohmann/json.hpp>

#include <atomic>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

using namespace ftxui;

static std::map<tirexMeasure, std::string> resultToMap(const tirexResult* result) {
	std::map<tirexMeasure, std::string> m;
	size_t num = 0;
	tirexResultEntryNum(result, &num);
	for (size_t i = 0; i < num; ++i) {
		tirexResultEntry e{};
		if (tirexResultEntryGetByIndex(result, i, &e) == TIREX_SUCCESS)
			m[e.source] = static_cast<const char*>(e.value);
	}
	return m;
}

// Extracts the latest value from a JSON time-series string, or returns the raw string.
static std::string latestVal(const std::string& raw) {
	try {
		auto j = nlohmann::json::parse(raw);
		if (j.contains("timeseries")) {
			auto& vals = j["timeseries"]["values"];
			if (!vals.empty()) {
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << vals.back().get<double>();
				return oss.str();
			}
		}
	} catch (...) {
	}
	return raw;
}

static float toFloat(const std::string& s, float def = 0.f) {
	try {
		return std::stof(s);
	} catch (const std::invalid_argument&) {
	} catch (const std::out_of_range&) {
	}
	return def;
}

static const std::string& getEntry(const std::map<tirexMeasure, std::string>& m, tirexMeasure k) {
	static const std::string empty = "";
	auto it = m.find(k);
	return it != m.end() ? it->second : empty;
}

static std::string withUnit(const std::string& s, const std::string& unit) {
	return s.empty() ? "N/A" : s + " " + unit;
}

struct LiveData {
	std::string cpuProcess = "—";
	std::string cpuSystem = "—";
	std::string cpuFreq = "—";
	std::string ramProcess = "—"; // raw KB from tracker
	std::string ramSystem = "—";  // MB
	std::string gpuProcess = "—";
	std::string gpuSystem = "—";
	std::string vramProcess = "—"; // MB
	std::string vramSystem = "—";  // MB
	std::string cpuEnergy = "—";
	std::string ramEnergy = "—";
	std::string gpuEnergy = "—";
	std::string elapsed = "—";
	bool gpuActive = false;
	bool energyActive = false;
};

static std::mutex g_mutex;
static LiveData g_live;

static void refreshLoop(tirexMeasureHandle* handle, ScreenInteractive& screen, std::atomic<bool>& running) {
	while (running.load()) {
		tirexResult* snap = nullptr;
		if (tirexPeekResult(handle, &snap) == TIREX_SUCCESS) {
			auto m = resultToMap(snap);
			tirexResultFree(snap);

			auto pick = [&](tirexMeasure k) -> std::string {
				auto it = m.find(k);
				return it != m.end() ? latestVal(it->second) : "—";
			};

			LiveData d{
					.cpuProcess = pick(TIREX_CPU_USED_PROCESS_PERCENT),
					.cpuSystem = pick(TIREX_CPU_USED_SYSTEM_PERCENT),
					.cpuFreq = pick(TIREX_CPU_FREQUENCY_MHZ),
					.ramProcess = pick(TIREX_RAM_USED_PROCESS_KB),
					.ramSystem = pick(TIREX_RAM_USED_SYSTEM_MB),
					.gpuProcess = pick(TIREX_GPU_USED_PROCESS_PERCENT),
					.gpuSystem = pick(TIREX_GPU_USED_SYSTEM_PERCENT),
					.vramProcess = pick(TIREX_GPU_VRAM_USED_PROCESS_MB),
					.vramSystem = pick(TIREX_GPU_VRAM_USED_SYSTEM_MB),
					.cpuEnergy = pick(TIREX_CPU_ENERGY_SYSTEM_JOULES),
					.ramEnergy = pick(TIREX_RAM_ENERGY_SYSTEM_JOULES),
					.gpuEnergy = pick(TIREX_GPU_ENERGY_SYSTEM_JOULES),
					.elapsed = pick(TIREX_TIME_ELAPSED_WALL_CLOCK_MS),
					.gpuActive = (d.gpuProcess != "—"),
					.energyActive = (d.cpuEnergy != "—" || d.ramEnergy != "—" || d.gpuEnergy != "—"),
			};

			{
				std::lock_guard<std::mutex> lock(g_mutex);
				g_live = d;
			}
		}
		screen.PostEvent(Event::Custom);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

// A labelled gauge row for percentage values (0–100).
static Element gaugeRow(const std::string& label, const std::string& val) {
	bool na = (val == "—");
	float ratio = na ? 0.f : std::min(toFloat(val) / 100.f, 1.0f);
	return hbox({
			text(label) | size(WIDTH, EQUAL, 20) | dim,
			na ? (text("N/A") | dim | flex)
			   : hbox({gauge(ratio) | flex, text(" " + val + "%") | size(WIDTH, EQUAL, 8)}),
	});
}

static Element textRow(const std::string& label, const std::string& val) {
	bool na = (val.empty() || val == "—");
	return hbox({
			text(label) | size(WIDTH, EQUAL, 20) | dim,
			na ? (text("N/A") | dim) : text(val),
	});
}

static Element notAvailRow(const std::string& msg) {
	return hbox({text(" ⚠  ") | color(Color::Yellow), text(msg) | dim});
}

static Element renderLiveTab() {
	LiveData d;
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		d = g_live;
	}

	// Format process RAM from KB into a human-readable string.
	std::string procRamDisplay = "—";
	if (d.ramProcess != "—") {
		float kb = toFloat(d.ramProcess);
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(1);
		if (kb >= 1024.f * 1024.f)
			oss << (kb / 1024.f / 1024.f) << " GB";
		else if (kb >= 1024.f)
			oss << (kb / 1024.f) << " MB";
		else
			oss << kb << " KB";
		procRamDisplay = oss.str();
	}

	auto cpuSection =
			window(text(" CPU "), vbox({
										  gaugeRow("Process usage:  ", d.cpuProcess),
										  gaugeRow("System usage:   ", d.cpuSystem),
										  textRow("Frequency:      ", d.cpuFreq == "—" ? "—" : d.cpuFreq + " MHz"),
								  }));

	auto ramSection =
			window(text(" Memory "),
				   vbox({
						   textRow("Process RAM:    ", procRamDisplay),
						   textRow("System RAM:     ", d.ramSystem == "—" ? "—" : d.ramSystem + " MB"),
				   }));

	Element gpuContent =
			d.gpuActive ? vbox({
								  gaugeRow("Process usage:  ", d.gpuProcess),
								  gaugeRow("System usage:   ", d.gpuSystem),
								  textRow("VRAM (process): ", d.vramProcess == "—" ? "—" : d.vramProcess + " MB"),
								  textRow("VRAM (system):  ", d.vramSystem == "—" ? "—" : d.vramSystem + " MB"),
						  })
						: notAvailRow("GPU not available on this system");
	auto gpuSection = window(text(" GPU "), gpuContent);

	Element energyContent =
			d.energyActive ? vbox({
									 textRow("CPU energy:     ", d.cpuEnergy == "—" ? "—" : d.cpuEnergy + " J"),
									 textRow("RAM energy:     ", d.ramEnergy == "—" ? "—" : d.ramEnergy + " J"),
									 textRow("GPU energy:     ", d.gpuEnergy == "—" ? "—" : d.gpuEnergy + " J"),
							 })
						   : notAvailRow("Energy monitoring not available (requires RAPL or NVML)");
	auto energySection = window(text(" Energy "), energyContent);

	auto footer = hbox({
			text("  Elapsed: ") | dim,
			text(d.elapsed == "—" ? "—" : d.elapsed + " ms") | bold,
			filler(),
			text("Refreshes every 500 ms") | dim,
	});

	return vbox({cpuSection, ramSection, gpuSection, energySection, footer});
}

static Element renderHardwareTab(const std::map<tirexMeasure, std::string>& info, bool gpuSupported) {
	auto cpu =
			window(text(" CPU "),
				   vbox({
						   textRow("Model:          ", getEntry(info, TIREX_CPU_MODEL_NAME)),
						   textRow("Vendor:         ", getEntry(info, TIREX_CPU_VENDOR_ID)),
						   textRow("Architecture:   ", getEntry(info, TIREX_CPU_ARCHITECTURE)),
						   textRow("Byte order:     ", getEntry(info, TIREX_CPU_BYTE_ORDER)),
						   textRow("Cores/socket:   ", getEntry(info, TIREX_CPU_CORES_PER_SOCKET)),
						   textRow("Threads/core:   ", getEntry(info, TIREX_CPU_THREADS_PER_CORE)),
						   textRow("Total cores:    ", getEntry(info, TIREX_CPU_AVAILABLE_SYSTEM_CORES)),
						   textRow("Min frequency:  ", withUnit(getEntry(info, TIREX_CPU_FREQUENCY_MIN_MHZ), "MHz")),
						   textRow("Max frequency:  ", withUnit(getEntry(info, TIREX_CPU_FREQUENCY_MAX_MHZ), "MHz")),
						   textRow("Virtualization: ", getEntry(info, TIREX_CPU_VIRTUALIZATION)),
				   }));

	auto mem =
			window(text(" Memory "),
				   vbox({
						   textRow("Available RAM:  ", withUnit(getEntry(info, TIREX_RAM_AVAILABLE_SYSTEM_MB), "MB")),
				   }));

	Element gpuContent = gpuSupported
								 ? vbox({
										   textRow("Model:          ", getEntry(info, TIREX_GPU_MODEL_NAME)),
										   textRow("CUDA cores:     ", getEntry(info, TIREX_GPU_NUM_CORES)),
										   textRow("VRAM:           ",
												   withUnit(getEntry(info, TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB), "MB")),
								   })
								 : notAvailRow("No NVIDIA GPU detected");
	auto gpu = window(text(" GPU "), gpuContent);

	auto os =
			window(text(" Operating System "), vbox({
													   textRow("Name:           ", getEntry(info, TIREX_OS_NAME)),
													   textRow("Kernel:         ", getEntry(info, TIREX_OS_KERNEL)),
											   }));

	return vbox({cpu, mem, gpu, os});
}

static Element renderEnvTab(const std::map<tirexMeasure, std::string>& info) {
	auto tracker =
			window(text(" TIREx Tracker "), vbox({
													textRow("Version:        ", getEntry(info, TIREX_VERSION_MEASURE)),
											}));

	const auto& isRepoStr = getEntry(info, TIREX_GIT_IS_REPO);
	bool isRepo = (isRepoStr == "1" || isRepoStr == "true");

	Element gitContent = isRepo ? vbox({
										  textRow("Hash:           ", getEntry(info, TIREX_GIT_HASH)),
										  textRow("Last commit:    ", getEntry(info, TIREX_GIT_LAST_COMMIT_HASH)),
										  textRow("Branch:         ", getEntry(info, TIREX_GIT_BRANCH)),
										  textRow("Upstream:       ", getEntry(info, TIREX_GIT_BRANCH_UPSTREAM)),
										  textRow("Remote origin:  ", getEntry(info, TIREX_GIT_REMOTE_ORIGIN)),
										  textRow("Tags:           ", getEntry(info, TIREX_GIT_TAGS)),
										  textRow("Root:           ", getEntry(info, TIREX_GIT_ROOT)),
										  hbox({
												  text("Changes:        ") | size(WIDTH, EQUAL, 20) | dim,
												  text("Uncommitted: "),
												  text(getEntry(info, TIREX_GIT_UNCOMMITTED_CHANGES)),
												  text("   Unpushed: ") | dim,
												  text(getEntry(info, TIREX_GIT_UNPUSHED_CHANGES)),
												  text("   Untracked: ") | dim,
												  text(getEntry(info, TIREX_GIT_UNCHECKED_FILES)),
										  }),
								  })
								: notAvailRow("Not a git repository");
	auto git = window(text(" Git "), gitContent);

	const auto& devconf = getEntry(info, TIREX_DEVCONTAINER_CONF_PATHS);
	Element devContent = (devconf.empty() || devconf == "[]" || devconf == "null")
								 ? notAvailRow("No devcontainer configuration found")
								 : paragraph(devconf);
	auto dev = window(text(" Dev Container "), devContent);

	return vbox({tracker, git, dev});
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
	// 1. Fetch static hardware / environment info once.
	const tirexMeasureConf infoCfg[] = {
			{TIREX_OS_NAME, TIREX_AGG_NO},
			{TIREX_OS_KERNEL, TIREX_AGG_NO},
			{TIREX_CPU_MODEL_NAME, TIREX_AGG_NO},
			{TIREX_CPU_VENDOR_ID, TIREX_AGG_NO},
			{TIREX_CPU_ARCHITECTURE, TIREX_AGG_NO},
			{TIREX_CPU_BYTE_ORDER, TIREX_AGG_NO},
			{TIREX_CPU_CORES_PER_SOCKET, TIREX_AGG_NO},
			{TIREX_CPU_THREADS_PER_CORE, TIREX_AGG_NO},
			{TIREX_CPU_AVAILABLE_SYSTEM_CORES, TIREX_AGG_NO},
			{TIREX_CPU_FREQUENCY_MIN_MHZ, TIREX_AGG_NO},
			{TIREX_CPU_FREQUENCY_MAX_MHZ, TIREX_AGG_NO},
			{TIREX_CPU_VIRTUALIZATION, TIREX_AGG_NO},
			{TIREX_RAM_AVAILABLE_SYSTEM_MB, TIREX_AGG_NO},
			{TIREX_GPU_SUPPORTED, TIREX_AGG_NO},
			{TIREX_GPU_MODEL_NAME, TIREX_AGG_NO},
			{TIREX_GPU_NUM_CORES, TIREX_AGG_NO},
			{TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB, TIREX_AGG_NO},
			{TIREX_GIT_IS_REPO, TIREX_AGG_NO},
			{TIREX_GIT_HASH, TIREX_AGG_NO},
			{TIREX_GIT_LAST_COMMIT_HASH, TIREX_AGG_NO},
			{TIREX_GIT_BRANCH, TIREX_AGG_NO},
			{TIREX_GIT_BRANCH_UPSTREAM, TIREX_AGG_NO},
			{TIREX_GIT_TAGS, TIREX_AGG_NO},
			{TIREX_GIT_REMOTE_ORIGIN, TIREX_AGG_NO},
			{TIREX_GIT_UNCOMMITTED_CHANGES, TIREX_AGG_NO},
			{TIREX_GIT_UNPUSHED_CHANGES, TIREX_AGG_NO},
			{TIREX_GIT_UNCHECKED_FILES, TIREX_AGG_NO},
			{TIREX_GIT_ROOT, TIREX_AGG_NO},
			{TIREX_VERSION_MEASURE, TIREX_AGG_NO},
			{TIREX_DEVCONTAINER_CONF_PATHS, TIREX_AGG_NO},
			tirexNullConf,
	};
	tirexResult* infoResult = nullptr;
	if (tirexFetchInfo(infoCfg, &infoResult) != TIREX_SUCCESS)
		return 1;
	auto infoMap = resultToMap(infoResult);
	tirexResultFree(infoResult);

	bool gpuSupported = (getEntry(infoMap, TIREX_GPU_SUPPORTED) == "1");

	// 2. Start continuous tracking.
	const tirexMeasureConf trackCfg[] = {
			{TIREX_TIME_ELAPSED_WALL_CLOCK_MS, TIREX_AGG_NO}, {TIREX_CPU_USED_PROCESS_PERCENT, TIREX_AGG_NO},
			{TIREX_CPU_USED_SYSTEM_PERCENT, TIREX_AGG_NO},	  {TIREX_CPU_FREQUENCY_MHZ, TIREX_AGG_NO},
			{TIREX_CPU_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO},	  {TIREX_RAM_USED_PROCESS_KB, TIREX_AGG_NO},
			{TIREX_RAM_USED_SYSTEM_MB, TIREX_AGG_NO},		  {TIREX_RAM_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO},
			{TIREX_GPU_USED_PROCESS_PERCENT, TIREX_AGG_NO},	  {TIREX_GPU_USED_SYSTEM_PERCENT, TIREX_AGG_NO},
			{TIREX_GPU_VRAM_USED_PROCESS_MB, TIREX_AGG_NO},	  {TIREX_GPU_VRAM_USED_SYSTEM_MB, TIREX_AGG_NO},
			{TIREX_GPU_ENERGY_SYSTEM_JOULES, TIREX_AGG_NO},	  tirexNullConf,
	};
	tirexMeasureHandle* handle = nullptr;
	if (tirexStartTracking(trackCfg, 100, &handle) != TIREX_SUCCESS)
		return 1;

	// 3. Build FTXUI components.
	auto screen = ScreenInteractive::Fullscreen();

	int tabIndex = 0;
	std::vector<std::string> tabLabels = {" Live Metrics ", " Hardware ", " Environment "};
	auto tabToggle = Toggle(&tabLabels, &tabIndex);

	auto liveTab = Renderer([]() { return renderLiveTab(); });
	auto hwTab = Renderer([&infoMap, gpuSupported]() { return renderHardwareTab(infoMap, gpuSupported); });
	auto envTab = Renderer([&infoMap]() { return renderEnvTab(infoMap); });

	auto tabContent = Container::Tab({liveTab, hwTab, envTab}, &tabIndex);
	auto layout = Container::Vertical({tabToggle, tabContent});

	layout = CatchEvent(layout, [&screen](Event event) {
		if (event == Event::Character('q') || event == Event::Escape) {
			screen.ExitLoopClosure()();
			return true;
		}
		return false;
	});

	auto ui = Renderer(layout, [&]() {
		return vbox({
				hbox({
						text(" TIREx Live Tracker ") | bold | color(Color::Cyan),
						filler(),
						text(" [←/→] Switch tabs   [q] Quit ") | dim,
				}),
				separator(),
				tabToggle->Render(),
				separator(),
				tabContent->Render() | flex,
		});
	});

	// 4. Start background refresh thread (calls tirexPeekResult every 500 ms).
	std::atomic<bool> running{true};
	std::thread bgThread(refreshLoop, handle, std::ref(screen), std::ref(running));

	// 5. Run the UI event loop (blocks until 'q' or Escape).
	screen.Loop(ui);

	// 6. Tear down: stop refresh thread, then stop tracking.
	running.store(false);
	bgThread.join();

	tirexResult* finalResult = nullptr;
	tirexStopTracking(handle, &finalResult);
	tirexResultFree(finalResult);
	return 0;
}
