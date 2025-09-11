#include "pn_table.h"

#include "../algorithms/array_pool.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

using namespace std;

namespace pn_heuristic {
static inline FactPair get_fact(const State &state, int var) {
    return {var, state.get_unpacked_values()[var]};
}

PnTable::PnTable(
    int width, const novelty::TaskInfo &task_info,
    const std::vector<std::shared_ptr<FeatureGenerator>> fgens)
    : width(width), task_info(task_info), fgens(fgens), n_fgens(fgens.size()) {
}

int PnTable::compute_novelty_and_update_table(const State &state) {
    int min_novelty = UNKNOWN_NOVELTY;

    for (const auto &fg : fgens) {
        std::vector<StateFeature> features = fg->compute_features(state);

        // Check for novelty 1.
        for (const StateFeature &feat : features) {
            if (!seen_features.count(feat)) {
                seen_features.insert(feat);
                min_novelty = 1;
            }
        }

        // Check for novelty 2.
        if (width == 2) {
            for (const StateFeature &f1 : features) {
                for (const StateFeature &f2 : features) {
                    if (f1 >= f2) {
                        continue;
                    }
                    std::pair<StateFeature, StateFeature> feature_pair = {
                        f1, f2};
                    if (!seen_feature_pairs.count(feature_pair)) {
                        seen_feature_pairs.insert(feature_pair);
                        min_novelty = 2;
                    }
                }
            }
        }
    }

    return min_novelty;
}
}
