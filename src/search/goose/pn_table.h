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

class NoveltyTable {
    int width;

    const novelty::TaskInfo &task_info;
    std::vector<bool> seen_facts;
    std::vector<bool> seen_fact_pairs;
    std::set<features::WLFeature> seen_wl_features;
    std::set<std::pair<features::WLFeature, features::WLFeature>>
        seen_wl_feature_pairs;

    bool at;
    bool wl;

    const std::shared_ptr<features::WLFeatureGenerator> wlf_generator;

public:
    NoveltyTable(
        int width, const novelty::TaskInfo &task_info, bool at, bool wl,
        const std::shared_ptr<features::WLFeatureGenerator> &wlf_generator);

    static const int UNKNOWN_NOVELTY = 3;

    int compute_novelty_and_update_table(const State &state);
    int compute_novelty_and_update_table(
        const State &parent_state, int op_id, const State &succ_state);
    void reset();
    void dump();
};
}

#endif
