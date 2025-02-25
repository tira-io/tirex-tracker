#ifndef STATS_GITSTATS_HPP
#define STATS_GITSTATS_HPP

#include "provider.hpp"

struct git_repository;

namespace tirex {
	class GitStats final : public StatsProvider {
	private:
		git_repository* repo;

	public:
		GitStats();
		~GitStats();

		bool isRepository() const noexcept;

		Stats getInfo() override;

		static constexpr const char* description = "Collects git related metrics.";
		static const char* version;
		static const std::set<tirexMeasure> measures;
	};
} // namespace tirex

#endif