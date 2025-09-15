#include "qb_heuristic.h"

#include "../heuristics/additive_heuristic.h"
#include "../heuristics/ff_heuristic.h"
#include "../heuristics/goal_count_heuristic.h"
#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <iostream>
using namespace std;

namespace qb_heuristic {
QbHeuristic::QbHeuristic(
    const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const std::string &description, utils::Verbosity verbosity,
    const std::shared_ptr<Evaluator> base_heuristic, const int width,
    const std::vector<std::shared_ptr<FeatureGenerator>> &fgens)
    : Heuristic(transform, cache_estimates, description, verbosity),
      base_heuristic(base_heuristic),
      width(width),
      fgens(fgens),
      n_fgens(fgens.size()),
      log(utils::get_log_for_verbosity(verbosity)),
      statistics(log) {
}

int QbHeuristic::compute_heuristic(const State &ancestor_state) {
    EvaluationContext eval_context(ancestor_state, 0, false, &statistics);
    int h = eval_context.get_evaluator_value_or_infinity(base_heuristic.get());
    if (h == EvaluationResult::INFTY)
        return DEAD_END;

    int nov_h = 0;
    int non_h = 0;

    State state = convert_ancestor_state(ancestor_state);

    StateFeatureIndexed feat_i;
    for (int i = 0; i < n_fgens; i++) {
        // Width = 1
        for (const StateFeature &feat : fgens[i]->compute_features(state)) {
            feat_i = std::make_pair(feat, i);

            bool in_map = feat_to_min_h.count(feat_i) > 0;
            if (!in_map || h < feat_to_min_h[feat_i]) {
                feat_to_min_h[feat_i] = h;
                nov_h -= 1;
            } else if (in_map && h > feat_to_min_h[feat_i]) {
                non_h += 1;
            }
        }

        // Width = 2
        if (width == 2) {
            StateFeaturePairIndexed feature_pair_i;
            std::vector<StateFeature> features;
            for (const StateFeature &feat : fgens[i]->compute_features(state)) {
                features.push_back(feat);
            }
            std::sort(features.begin(), features.end());
            for (size_t j = 0; j < features.size(); j++) {
                for (size_t k = j + 1; k < features.size(); k++) {
                    feature_pair_i = {features[j], features[k], i};

                    bool in_map = pair_feat_to_min_h.count(feature_pair_i) > 0;
                    if (!in_map || h < pair_feat_to_min_h[feature_pair_i]) {
                        pair_feat_to_min_h[feature_pair_i] = h;
                        nov_h -= 1;
                    } else if (
                        in_map && h > pair_feat_to_min_h[feature_pair_i]) {
                        non_h += 1;
                    }
                }
            }
        }
    }

    return nov_h < 0 ? nov_h : non_h;
}

class QbHeuristicFeature
    : public plugins::TypedFeature<Evaluator, QbHeuristic> {
public:
    QbHeuristicFeature() : TypedFeature("qb") {
        document_title("Quantified both heuristic");

        add_option<std::shared_ptr<Evaluator>>(
            "eval", "Heuristic for novelty calculation");
        add_option<int>(
            "width", "maximum feature size", "1", plugins::Bounds("1", "2"));
        add_option<int>(
            "max_variables_for_width2",
            "if there are more variables, use width=1", "infinity",
            plugins::Bounds("0", "infinity"));
        add_list_option<std::shared_ptr<FeatureGenerator>>(
            "feats", "Feature generators");
        add_heuristic_options_to_feature(*this, "qb");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "supported");
        document_language_support("axioms", "supported");

        document_property("admissible", "no");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<QbHeuristic> create_component(
        const plugins::Options &opts) const override {
        int width = opts.get<int>("width");
        int num_vars =
            TaskProxy(*opts.get<shared_ptr<AbstractTask>>("transform"))
                .get_variables()
                .size();
        if (num_vars > opts.get<int>("max_variables_for_width2")) {
            utils::g_log << "Number of variables exceeds limit "
                         << " --> use width=1" << endl;
            width = 1;
        }
        return plugins::make_shared_from_arg_tuples<QbHeuristic>(
            opts.get<std::shared_ptr<AbstractTask>>("transform"),
            opts.get<bool>("cache_estimates"),
            opts.get<std::string>("description"),
            opts.get<utils::Verbosity>("verbosity"),
            opts.get<std::shared_ptr<Evaluator>>("eval"), width,
            opts.get_list<std::shared_ptr<FeatureGenerator>>("feats"));
    }
};

static plugins::FeaturePlugin<QbHeuristicFeature> _plugin;
} // namespace qb_heuristic
