#ifndef GOOSE_PN_TABLE_H
#define GOOSE_PN_TABLE_H

#include "../task_proxy.h"

#include "../algorithms/array_pool.h"
#include "../features/wlf_generator.h"
#include "../novelty/novelty_table.h"

#include <cassert>
#include <set>
#include <vector>

namespace pn_heuristic {

class PnTable {
    const int width;

    const novelty::TaskInfo &task_info;
    const std::vector<std::shared_ptr<FeatureGenerator>> fgens;
    const int n_fgens;
    std::set<StateFeatureIndexed> seen_features;
    std::set<std::tuple<StateFeature, StateFeature, int>> seen_feature_pairs;

public:
    PnTable(
        const int width, const novelty::TaskInfo &task_info,
        const std::vector<std::shared_ptr<FeatureGenerator>> fgens);

    static const int UNKNOWN_NOVELTY = 3;

    int compute_novelty_and_update_table(const State &state);
};
}

#endif
