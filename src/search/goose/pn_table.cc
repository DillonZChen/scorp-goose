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

    StateFeatureIndexed feat_i;
    std::tuple<StateFeature, StateFeature, int> feature_pair_i;
    for (int i = 0; i < n_fgens; i++) {
        std::vector<StateFeature> features = fgens[i]->compute_features(state);

        // Check for novelty 1.
        for (const StateFeature &feat : features) {
            feat_i = std::make_pair(feat, i);
            if (!seen_features.count(feat_i)) {
                seen_features.insert(feat_i);
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
                    feature_pair_i = {f1, f2, i};
                    if (!seen_feature_pairs.count(feature_pair_i)) {
                        seen_feature_pairs.insert(feature_pair_i);
                        min_novelty = 2;
                    }
                }
            }
        }
    }

    return min_novelty;
}
}
