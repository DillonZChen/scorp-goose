#ifndef GOOSE_PN_HEURISTIC_H
#define GOOSE_PN_HEURISTIC_H

#include "pn_table.h"

#include "../heuristic.h"

#include "../novelty/novelty_table.h"

namespace pn_heuristic {
class PnHeuristic : public Heuristic {
    const int width;
    const bool consider_only_novel_states;

    const std::vector<std::shared_ptr<Evaluator>> evals;
    const std::vector<std::shared_ptr<FeatureGenerator>> fgens;
    const int n_fgens;
    const novelty::TaskInfo task_info;

    std::unordered_map<std::vector<int>, PnTable, utils::Hash<std::vector<int>>>
        novelty_tables;
    std::vector<int> novelty_to_num_states;

    void set_novelty(const State &state, int novelty);
    std::vector<int> evaluate_state(const State &state);

    std::shared_ptr<features::WLFGenerator> wlf_generator;

protected:
    virtual int compute_heuristic(const State &ancestor_state) override;

public:
    PnHeuristic(
        int width, const std::vector<std::shared_ptr<Evaluator>> &evals,
        const std::vector<std::shared_ptr<FeatureGenerator>>
            &feature_generators,
        bool consider_only_novel_states,
        const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
        const std::string &description, utils::Verbosity verbosity);
    virtual ~PnHeuristic() override;

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override;
    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(
        const State &parent, OperatorID op_id, const State &state) override;
    virtual bool dead_ends_are_reliable() const override;
};

// HACK: we need to notify landmark heuristics before evaluating the novelty
// heuristics that depend on them.
struct OrderPnHeuristicsLastHack {
    bool operator()(const Evaluator *lhs, const Evaluator *rhs) const {
        if (dynamic_cast<const pn_heuristic::PnHeuristic *>(lhs) != nullptr &&
            dynamic_cast<const pn_heuristic::PnHeuristic *>(rhs) == nullptr) {
            return false;
        }
        if (dynamic_cast<const pn_heuristic::PnHeuristic *>(lhs) == nullptr &&
            dynamic_cast<const pn_heuristic::PnHeuristic *>(rhs) != nullptr) {
            return true;
        }
        return lhs < rhs;
    }
};
}

#endif
