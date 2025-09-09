#include "qb_atwl_heuristic.h"

#include "../heuristics/additive_heuristic.h"
#include "../heuristics/ff_heuristic.h"
#include "../heuristics/goal_count_heuristic.h"
#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <iostream>
using namespace std;

namespace qb_heuristic {
QbAtWlHeuristic::QbAtWlHeuristic(
    const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const std::string &description, utils::Verbosity verbosity,
    const std::shared_ptr<Evaluator> base_heuristic, int wl_iterations,
    const std::string &graph_representation, const std::string &wl_algorithm)
    : QbHeuristic(
          transform, cache_estimates, description, verbosity, base_heuristic) {
    // Construct domain
    std::cout << "Initialising domain..." << std::endl;
    planning::Domain domain = wl_utils::construct_wlplan_domain(task);

    // Construct problem
    std::cout << "Initialising problem..." << std::endl;
    const std::map<FactPair, wl_utils::PredArgsString> &mapper =
        wl_utils::get_fd_fact_to_pred_args_map(task);
    auto [fd_fact_map, problem] =
        wl_utils::construct_wlplan_problem(domain, mapper, task_proxy);
    fd_fact_to_wlplan_atom = fd_fact_map;

    // Set up WLF generator
    std::cout << "Initialising WLF generator..." << std::endl;
    model = wl_utils::init_wlf_generator(
        domain, problem, graph_representation, wl_iterations, wl_algorithm);

    std::cout << "PNWL Novelty Heuristic initialised!" << std::endl;
}

int QbAtWlHeuristic::compute_heuristic(const State &ancestor_state) {
    EvaluationContext eval_context(ancestor_state, 0, false, &statistics);
    int h = eval_context.get_evaluator_value_or_infinity(base_heuristic.get());
    if (h == EvaluationResult::INFTY)
        return DEAD_END;

    int nov_h = 0;
    int non_h = 0;

    State state = convert_ancestor_state(ancestor_state);

    // WL part
    planning::State wl_state =
        wl_utils::to_wlplan_state(state, fd_fact_to_wlplan_atom);
    std::unordered_map<int, int> features = model->collect_embed(wl_state);
    for (const std::pair<const int, int> &feat : features) {
        if (feat.second ==
            0) { // feature not present, their values do not matter
            continue;
        }
        bool in_map = feat_to_lowest_h.count(feat) > 0;
        if (!in_map || h < feat_to_lowest_h[feat]) {
            feat_to_lowest_h[feat] = h;
            nov_h -= 1;
        } else if (in_map && h > feat_to_lowest_h[feat]) {
            non_h += 1;
        }
    }

    // PN part
    for (const FactProxy &fact : state) {
        const std::pair<int, int> pair = fact.get_int_pair();
        bool in_map = feat_to_lowest_h.count(pair) > 0;
        if (!in_map || h < feat_to_lowest_h[pair]) {
            feat_to_lowest_h[pair] = h;
            nov_h -= 1;
        } else if (in_map && h > feat_to_lowest_h[pair]) {
            non_h += 1;
        }
    }

    return nov_h < 0 ? nov_h : non_h;
}

class QbAtWlHeuristicFeature
    : public plugins::TypedFeature<Evaluator, QbAtWlHeuristic> {
public:
    QbAtWlHeuristicFeature() : TypedFeature("qbatwl") {
        document_title("Goal count heuristic");

        add_option<shared_ptr<Evaluator>>(
            "eval", "Heuristic for novelty calculation");
        add_option<int>("l", "Number of wl iterations", "2");
        add_option<std::string>("g", "Graph representation", "\"ilg\"");
        add_option<std::string>("w", "WL algorithm", "\"wl\"");
        add_heuristic_options_to_feature(*this, "qbatwl");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "supported");
        document_language_support("axioms", "supported");

        document_property("admissible", "no");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<QbAtWlHeuristic> create_component(
        const plugins::Options &opts) const override {
        return std::make_shared<QbAtWlHeuristic>(
            opts.get<shared_ptr<AbstractTask>>("transform"),
            opts.get<bool>("cache_estimates"),
            opts.get<std::string>("description"),
            opts.get<utils::Verbosity>("verbosity"),
            opts.get<shared_ptr<Evaluator>>("eval"), opts.get<int>("l"),
            opts.get<std::string>("g"), opts.get<std::string>("w"));
    }
};

static plugins::FeaturePlugin<QbAtWlHeuristicFeature> _plugin;
} // namespace qb_heuristic
