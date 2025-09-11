#include "pn_table.h"

#include "../algorithms/array_pool.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

using namespace std;

namespace pn_heuristic {
static inline FactPair get_fact(const State &state, int var) {
    return {var, state.get_unpacked_values()[var]};
}

NoveltyTable::NoveltyTable(
    int width, const novelty::TaskInfo &task_info, bool at, bool wl,
    const std::shared_ptr<features::WLFGenerator> &wlf_generator)
    : width(width),
      task_info(task_info),
      at(at),
      wl(wl),
      wlf_generator(wlf_generator) {
    reset();
}

int NoveltyTable::compute_novelty_and_update_table(const State &state) {
    const auto &primary_variables = task_info.get_primary_variables();
    int num_vars = primary_variables.size();
    int min_novelty = UNKNOWN_NOVELTY;

    /* Atom features */
    if (at) {
        // Check for novelty 1.
        for (int var : primary_variables) {
            FactPair fact = get_fact(state, var);
            int fact_id = task_info.get_fact_id(fact);
            if (!seen_facts[fact_id]) {
                seen_facts[fact_id] = true;
                min_novelty = 1;
            }
        }

        // Check for novelty 2.
        if (width == 2) {
            for (int pos1 = 0; pos1 < num_vars; ++pos1) {
                int var1 = primary_variables[pos1];
                FactPair fact1 = get_fact(state, var1);
                for (int pos2 = pos1 + 1; pos2 < num_vars; ++pos2) {
                    int var2 = primary_variables[pos2];
                    FactPair fact2 = get_fact(state, var2);
                    uint64_t pair_id = task_info.get_pair_id(fact1, fact2);
                    bool seen = seen_fact_pairs[pair_id];
                    if (!seen) {
                        seen_fact_pairs[pair_id] = true;
                        min_novelty = min(min_novelty, 2);
                    }
                }
            }
        }
    }

    /* WL features */
    if (wl) {
        std::vector<StateFeature> features =
            wlf_generator->compute_features(state);

        // Check for novelty 1.
        for (const StateFeature &feat : features) {
            if (!seen_wl_features.count(feat)) {
                seen_wl_features.insert(feat);
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
                    std::pair<StateFeature, StateFeature>
                        feature_pair = {f1, f2};
                    if (!seen_wl_feature_pairs.count(feature_pair)) {
                        seen_wl_feature_pairs.insert(feature_pair);
                        min_novelty = 2;
                    }
                }
            }
        }
    }

    return min_novelty;
}

int NoveltyTable::compute_novelty_and_update_table(
    const State &parent_state, int op_id, const State &succ_state) {
    int min_novelty = UNKNOWN_NOVELTY;

    // Check for novelty 1.
    if (at) {
        for (FactPair effect_fact : task_info.get_effects(op_id)) {
            FactPair fact = get_fact(succ_state, effect_fact.var);
            int fact_id = task_info.get_fact_id(fact);
            if (!seen_facts[fact_id]) {
                seen_facts[fact_id] = true;
                min_novelty = 1;
            }
        }
    }

    // Check for novelty 2.
    if (at) {
        if (width == 2) {
            for (FactPair fact1 : task_info.get_effects(op_id)) {
                FactPair parent_fact1 = get_fact(parent_state, fact1.var);
                if (fact1 == parent_fact1) {
                    continue;
                }
                for (int var2 : task_info.get_primary_variables()) {
                    if (fact1.var == var2) {
                        continue;
                    }
                    FactPair fact2 = get_fact(succ_state, var2);
                    uint64_t pair_id = task_info.get_pair_id(fact1, fact2);
                    bool seen = seen_fact_pairs[pair_id];
                    if (!seen) {
                        seen_fact_pairs[pair_id] = true;
                        min_novelty = min(min_novelty, 2);
                    }
                }
            }
        }
    }

    return min_novelty;
}

void NoveltyTable::reset() {
    seen_facts.assign(task_info.get_num_facts(), false);
    if (width == 2) {
        seen_fact_pairs.assign(task_info.get_num_pairs(), false);
    }
}

void NoveltyTable::dump() {
    int num_seen_facts = count(seen_facts.begin(), seen_facts.end(), true);
    cout << "Seen " << num_seen_facts << "/" << task_info.get_num_facts()
         << " facts";
    if (width == 2) {
        int num_seen_fact_pairs =
            count(seen_fact_pairs.begin(), seen_fact_pairs.end(), true);
        cout << " and " << num_seen_fact_pairs << "/"
             << task_info.get_num_pairs() << " pairs.";
    }
    cout << endl;
}
}
