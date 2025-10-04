#ifndef FEATURES_WLF_RNI_GENERATOR_H
#define FEATURES_WLF_RNI_GENERATOR_H

#include "feature_utils.h"
#include "wlfmk2_generator.h"

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

namespace features {

class WLFRNI : public FeatureGenerator {
protected:
    const int wl_iterations;
    int max_arity, n_vars, n_vals;

    // a Fast Downward (var, val) pair maps to a list of object indices
    int n_objects;
    std::vector<std::vector<int>> connected_objects;

    // the colour of a (var, val) if it is seen in a state
    std::vector<int> colour;

    // useless (var, val) pair
    std::vector<bool> skip;

    // nodes that always exist because they are in goal, and their colour
    std::unordered_map<std::pair<int, int>, int, wlf_mk2_pair_hash> goal_colour;

    // hash per iteration
    std::unordered_map<std::vector<int>, int, wlf_mk2_int_vector_hasher> hash;

public:
    WLFRNI(const std::shared_ptr<AbstractTask> transform, int wl_iterations);

    ~WLFRNI();

    Generator<StateFeature> compute_features(const State &state);
};
} // namespace features

#endif
