#pragma once

#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace ParticleZoo {

/**
 * @brief Resolved particle-generation filter shared by the operations.
 *
 * Internal helper (not installed): the single place where the
 * primariesOnly/excludePrimaries/generations options are checked for
 * conflicts, validated, and mapped to a generation range.
 */
struct GenerationFilter {
    const bool useFilter;
    const int  minimumGeneration;
    const int  maximumGeneration;

    GenerationFilter(bool use, int mn, int mx)
        : useFilter(use), minimumGeneration(mn), maximumGeneration(mx) {}

    /**
     * @brief Resolve the generation-filter options into a GenerationFilter.
     *
     * @param primariesOnly    Keep only primary particles (generation == 1)
     * @param excludePrimaries Exclude primary particles
     * @param generations      Keep particles whose generation is in [first, second]
     * @return GenerationFilter The resolved filter (useFilter == false when no option is set)
     * @throws std::runtime_error if more than one option is set or the range is invalid
     */
    static GenerationFilter Resolve(bool primariesOnly,
                                    bool excludePrimaries,
                                    const std::optional<std::pair<int,int>>& generations) {
        int commandsUsed = (primariesOnly ? 1 : 0) +
                           (excludePrimaries ? 1 : 0) +
                           (generations.has_value() ? 1 : 0);
        if (commandsUsed > 1)
            throw std::runtime_error("Cannot specify more than one of primariesOnly, excludePrimaries, or generations at the same time.");
        if (generations.has_value() &&
            (generations->first > generations->second || generations->first < 1))
            throw std::runtime_error("Invalid generation filter range. Ensure that min <= max and that min is at least 1.");
        if (primariesOnly)
            return GenerationFilter(true, 1, 1);
        if (excludePrimaries)
            return GenerationFilter(true, 2, std::numeric_limits<int>::max());
        if (generations.has_value())
            return GenerationFilter(true, generations->first, generations->second);
        return GenerationFilter(false, 1, std::numeric_limits<int>::max());
    }
};

} // namespace ParticleZoo
