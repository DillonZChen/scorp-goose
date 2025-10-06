#ifndef FEATURES_WLF_MK3_GENERATOR_H
#define FEATURES_WLF_MK3_GENERATOR_H

#include "feature_utils.h"

#include "../feature_generator.h"
#include "../task_proxy.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// UG   0
// AG   1
// AP   2

class wlf_mk3_int_vector_hasher {
public:
    std::size_t operator()(std::vector<std::pair<int, int>> const &vec) const {
        std::size_t seed = vec.size();
        for (auto &i : vec) {
            seed ^= i.first + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= i.second + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

namespace features {

class WLFmk3Generator : public FeatureGenerator {
protected:
    const int wl_iterations;
    int max_arity, n_vars, n_vals;

    int n_objects;
    // flat mapping where connected_objects[ci[i], ..., ci[i + arity[i]]] are
    // objects connected to atom i
    std::vector<int> connected_objects;
    std::vector<int> ci; // index into connected_objects
    std::vector<int> arity;

    std::vector<int> pred_i;

    // initial colours of objects from goal info
    std::vector<int> node_colours;

    std::vector<int> edge_atom_colours; // the colour of atom if it gets seen
    std::unordered_map<int, int> edge_goal_colours; // edges only

    // useless (var, val) pair
    std::vector<bool> skip;

    // hash per iteration
    std::unordered_map<
        std::vector<std::pair<int, int>>, int, wlf_mk3_int_vector_hasher>
        hash;

public:
    WLFmk3Generator(
        const std::shared_ptr<AbstractTask> transform, int wl_iterations);

    ~WLFmk3Generator();

    Generator<StateFeature> compute_features(const State &state);
};
} // namespace features

#endif
