#ifndef GOOSE_PN_TABLE_H
#define GOOSE_PN_TABLE_H

#include "wl_utils.hpp"

#include "../task_proxy.h"

#include "../algorithms/array_pool.h"
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
    std::set<wl_utils::WLFeature> seen_wl_features;

    bool at;
    bool wl;

    const std::shared_ptr<wl_utils::DownwardToWlplanAtomMapper>
        fd_fact_to_wlplan_atom;

public:
    NoveltyTable(
        int width, const novelty::TaskInfo &task_info, bool at, bool wl,
        const std::shared_ptr<wl_utils::DownwardToWlplanAtomMapper>
            &fd_fact_to_wlplan_atom);

    static const int UNKNOWN_NOVELTY = 3;

    int compute_novelty_and_update_table(const State &state);
    int compute_novelty_and_update_table(
        const State &parent_state, int op_id, const State &succ_state);
    void reset();
    void dump();
};
}

#endif
