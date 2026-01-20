#ifndef STATS_PROVIDER_HPP
#define STATS_PROVIDER_HPP

#include <tirex_tracker.h>

#include "../timeseries.hpp"

#include <nlohmann/json.hpp>

#if __cpp_lib_concepts
#include <concepts>

#define CONVERTIBLE_TO_PAIR_CONCEPT std::convertible_to<std::pair<tirexMeasure, StatVal>>
#else
#define CONVERTIBLE_TO_PAIR_CONCEPT
#endif

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <variant>

namespace fs = std::filesystem;

namespace tirex {
	/**
	 * @brief Represents a temporary file that is automatically deleted on destruction.
	 * @details The TmpFile class encapsulates a filesystem path to a temporary file. When an instance of TmpFile goes
	 * out of scope, its destructor removes the file from the filesystem. This ensures automatic cleanup of temporary
	 * files without requiring explicit deletion by the caller.
	 */
	struct TmpFile {
		fs::path path; /**< @brief Path of the temporary file. */

		explicit TmpFile(fs::path path) : path(path) {}
		TmpFile(const TmpFile&) = delete;
		TmpFile(TmpFile&&) = default;
		TmpFile& operator=(const TmpFile&) = delete;
		TmpFile& operator=(TmpFile&&) = default;

		/**
		 * @brief Deletes the managed temporary file.
		 */
		~TmpFile() {
			if (!path.string().empty())
				fs::remove(path);
		}
	};

	using StatVal = std::variant<
			std::string, nlohmann::json, TmpFile, std::reference_wrapper<const tirex::TimeSeries<unsigned>>>;
	using Stats = std::map<tirexMeasure, StatVal>;

	tirexResult_st* createMsrResultFromStats(Stats&& stats);

	class StatsProvider {
	protected:
		std::set<tirexMeasure> enabled;

	public:
		StatsProvider() = default;
		StatsProvider(const StatsProvider&) = default;
		StatsProvider(StatsProvider&&) = default;
		virtual ~StatsProvider() = default;

		StatsProvider& operator=(const StatsProvider&) = default;
		StatsProvider& operator=(StatsProvider&&) = default;

		void requestMeasures(const std::set<tirexMeasure>& measures) noexcept;

		/**
		 * @brief The set of measures that the provider can provide.
		 */
		virtual std::set<tirexMeasure> providedMeasures() noexcept = 0;

		/**
		 * @brief Start is called once at the very beginning of collecting statistics and shortly before the command is
		 * run.
		 */
		virtual void start() {}
		/**
		 * @brief Stop is called once at the end of collecting statistics and shortly after the command is run.
		 */
		virtual void stop() {}
		/**
		 * @brief Step may be called multiple times during the execution of the command at some configured intervall (or
		 * even not at all).
		 */
		virtual void step() {}
		/**
		 * @brief Returns the statistics collected during the last measurement. This should be called once after
		 * StatsProvider::stop().
		 * 
		 * @returns the statistics that were measured
		 */
		virtual Stats getStats() { return {}; }

		/**
		 * @brief Returns the information collected by this provider.
		 * 
		 * @return the system information that is collected by this provider. 
		 */
		virtual Stats getInfo() { return {}; }
	};

	using ProviderConstructor = std::function<std::unique_ptr<StatsProvider>(void)>;
	struct ProviderEntry final {
		ProviderConstructor constructor;
		/**
		 * @brief The set of measures that the provider can provide in principle.
		 * @details This field can be used to instantiate only relevant providers (e.g., if only energy metrics are
		 * requested, then initializing the git provider may be skipped). As such, the measures listed here must contain
		 * all measures that the provider can at most support. But StatsProvider::providedMeasures may return only a
		 * subset if it can not support these measures on the system. E.g., the Git provider may not support measures if
		 * it cannot detect a git repository.
		 */
		const std::set<tirexMeasure>& measures;
		/**
		 * @brief A version string describing the provider and its data sources.
		 * @details This (potentially multiline) string can be used to pass information on dependencies to the caller.
		 * This may be useful for debugging purposes. E.g., the Git provider could report the version of libgit used.
		 */
		const char* version;
		const char* description;
	};
	extern const std::map<std::string, ProviderEntry> providers;

	/**
	 * @brief Initializes all the providers necessary to track the requested measures.
	 * 
	 * @param[in] measures The measures that are requested.
	 * @param[out] providers A vector of StatsProvider which will be populated with the initialized providers.
	 * @return A set of tirexMeasure which are not provided by any of the providers.
	 */
	std::set<tirexMeasure>
	initProviders(std::set<tirexMeasure> measures, std::vector<std::unique_ptr<StatsProvider>>& providers);

	Stats makeFilteredStats(const std::set<tirexMeasure>& filter, CONVERTIBLE_TO_PAIR_CONCEPT auto&&... args) {
		/** May be more readable when we can assume C++26 and expansion statements
		 *  (https://isocpp.org/files/papers/P1306R5.html) **/
		Stats stats;
		auto func = [&](CONVERTIBLE_TO_PAIR_CONCEPT auto&& val) {
			if (filter.contains(val.first))
				stats.insert(std::move(val));
		};
		(..., func(std::move(args)));
		return stats;
	}
} // namespace tirex

#endif