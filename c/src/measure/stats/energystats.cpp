#include "energystats.hpp"

#include "../../logging.hpp"

#include <optional>

using namespace std::string_literals;

using tirex::EnergyStats;
using tirex::Stats;

/**
 * @brief Returns the value of the mapping \p m for the key \p key . If this entry does not exist, empty is returned.
 * 
 * @param m A mapping in which the key should be found.
 * @param key The key to return the value for.
 * @return Empty if the mapping \p m did not contain \p k . Otherwise the value of \p m at \p k is returned.
 */
template <typename C, typename K>
static auto tryget(const C& m, K const& key) -> std::optional<decltype(m.begin()->second)> {
	auto it = m.find(key);
	if (it == m.end())
		return std::nullopt;
	return std::make_optional(it->second);
}

/**
 * @brief If \p opt is empty, empty is returned. Otherwise \p transform is applied to the value and returned.
 * 
 * @param opt The optional to transform.
 * @param transform The transformation to be applied.
 * @return A new optional with the transformed value of \p opt .
 */
template <typename T, typename Fn>
static auto transform(const std::optional<T>& opt, const Fn& transform)
		-> std::optional<decltype(transform(opt.value()))> {
	// Since we can't assume std::optional::transform from c++23 :(
	if (opt.has_value())
		return std::make_optional(transform(opt.value()));
	return std::nullopt;
}

const char* EnergyStats::version = nullptr;
const std::set<tirexMeasure> EnergyStats::measures{
		TIREX_CPU_ENERGY_SYSTEM_JOULES, TIREX_RAM_ENERGY_SYSTEM_JOULES, TIREX_GPU_ENERGY_SYSTEM_JOULES
};

EnergyStats::EnergyStats() : tracker() {}

std::set<tirexMeasure> EnergyStats::providedMeasures() noexcept {
	std::set<tirexMeasure> measures{};
	auto cap = tracker.getCapabilities();
	if (cap & cppjoules::Capability::CPU_PROFILE)
		measures.insert(TIREX_CPU_ENERGY_SYSTEM_JOULES);
	if (cap & cppjoules::Capability::RAM_PROFILE)
		measures.insert(TIREX_RAM_ENERGY_SYSTEM_JOULES);
	if (cap & cppjoules::Capability::GPU_PROFILE)
		measures.insert(TIREX_GPU_ENERGY_SYSTEM_JOULES);
	return measures;
}

void EnergyStats::start() { tracker.start(); }
void EnergyStats::stop() { tracker.stop(); }
Stats EnergyStats::getStats() {
	using json = nlohmann::json;
	auto results = tracker.calculate_energy().energy;
	for (auto& [device, result] : results)
		tirex::log::debug("cppjoules", "[{}] {}", device, result);
	// Divide by 1000'000 since CPPJoules reports micro Joule (uJ)
	auto divide = [](long long divisor) { return [divisor](long long divident) { return divident / divisor; }; };
	return makeFilteredStats(
			enabled,
			std::pair{TIREX_CPU_ENERGY_SYSTEM_JOULES, json(transform(tryget(results, "core-0"), divide(1000'000)))},
			std::pair{TIREX_RAM_ENERGY_SYSTEM_JOULES, json(transform(tryget(results, "dram-0"), divide(1000'000)))},
			std::pair{TIREX_GPU_ENERGY_SYSTEM_JOULES, json(transform(tryget(results, "nvidia_gpu_0"), divide(1000)))}
	);
}