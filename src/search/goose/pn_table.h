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
    int width;

    const novelty::TaskInfo &task_info;
    const std::vector<std::shared_ptr<FeatureGenerator>> fgens;
    const int n_fgens;
    std::set<StateFeature> seen_features;
    std::set<std::pair<StateFeature, StateFeature>> seen_feature_pairs;

    const std::shared_ptr<features::WLFGenerator> wlf_generator;

public:
    PnTable(
        int width, const novelty::TaskInfo &task_info,
        const std::vector<std::shared_ptr<FeatureGenerator>> fgens);

    static const int UNKNOWN_NOVELTY = 3;

    int compute_novelty_and_update_table(const State &state);
};
}

#endif
