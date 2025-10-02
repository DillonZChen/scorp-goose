#ifndef FEATURES_WLF_MK2_GENERATOR_H
#define FEATURES_WLF_MK2_GENERATOR_H

#include "feature_utils.h"

#include "../feature_generator.h"
#include "../task_proxy.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// T_POS_GOAL   0
// F_POS_GOAL   1
// T_NEG_GOAL   2
// F_NEG_GOAL   3
// NON_GOAL     4

class wlf_mk2_int_vector_hasher {
public:
    // https://stackoverflow.com/a/27216842
    std::size_t operator()(std::vector<int> const &vec) const {
        std::size_t seed = vec.size();
        for (auto &i : vec) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

namespace features {

class WLFmk2Generator : public FeatureGenerator {
protected:
    const int wl_iterations;

    // a Fast Downward (var, val) pair maps to a list of object indices
    int n_objects;
    std::vector<std::vector<std::vector<int>>> connected_objects;

    // the colour of a (var, val) if it is seen in a state
    std::vector<std::vector<int>> colour;

    // nodes that always exist because they are in goal, and their colour
    std::map<std::pair<int, int>, int> goal_colour;

    // hash per iteration
    std::unordered_map<std::vector<int>, int, wlf_mk2_int_vector_hasher> hash;

    int get_hash_colour(const std::vector<int> &colours);

public:
    WLFmk2Generator(
        const std::shared_ptr<AbstractTask> transform, int wl_iterations);

    ~WLFmk2Generator();

    Generator<StateFeature> compute_features(const State &state);
};
} // namespace features

#endif
