#include "dlf_generator.h"

#include "../task_proxy.h"

namespace features {
DLFGenerator::DLFGenerator(
    const std::shared_ptr<AbstractTask> transform, int concept_c, int role_c,
    int boolean_c, int count_c, int distance_c)
    : FeatureGenerator(transform),
      concept_complexity_limit(concept_c),
      role_complexity_limit(role_c),
      boolean_complexity_limit(boolean_c),
      count_numerical_complexity_limit(count_c),
      distance_numerical_complexity_limit(distance_c) {
    /* Pybind */
    pybind11::module sys = pybind11::module::import("sys");
    sys.attr("path").attr("insert")(
        1, "./ext/planners/scorpion/src/search/features");
    pybind11::module dlf_module = pybind11::module::import("dlf_generator");

    /* Get problem info */
    std::vector<std::pair<std::string, std::vector<std::string>>> facts;
    std::vector<std::pair<std::string, std::vector<std::string>>> goals;

    std::map<FactPair, PredArgsString> helper =
        get_fd_fact_to_pred_args_map(transform);

    // facts
    std::set<std::tuple<std::string, int>> predicates_set;
    for (const auto &[fact_pair, pred_args] : helper) {
        const std::string &predicate_name = pred_args.first;
        const int arity = pred_args.second.size();

        fact_to_i[fact_pair] = facts.size();
        predicates_set.insert(std::make_tuple(predicate_name, arity));
        facts.push_back(pred_args);
    }

    // goals
    for (FactProxy goal : task_proxy.get_goals()) {
        std::pair<std::string, bool> pddl_fact_info = get_pddl_fact(goal);
        std::string pddl_fact_name = pddl_fact_info.first;
        bool positive = pddl_fact_info.second;
        if (pddl_fact_name.empty()) {
            continue;
        }
        if (!positive) {
            std::cerr
                << "Error: negative goals are not supported in DLFGenerator"
                << std::endl;
            exit(1);
        }

        goals.push_back(fd_fact_to_pred_args(pddl_fact_name));
    }

    generator = dlf_module.attr("DLFGenerator")(
        predicates_set, facts, goals, concept_complexity_limit,
        role_complexity_limit, boolean_complexity_limit,
        count_numerical_complexity_limit, distance_numerical_complexity_limit);

    std::cout << "DLFGenerator initialised." << std::endl;
}

DLFGenerator::~DLFGenerator() {
    std::cout << "DL features collected: "
              << generator.attr("get_num_features")().cast<int>() << std::endl;
}

Generator<StateFeature> DLFGenerator::compute_features(const State &state) {
    std::vector<int> indices;
    for (const FactProxy &fact : state) {
        if (fact_to_i.count(fact.get_pair()) == 0) {
            continue;
        }
        indices.push_back(fact_to_i.at(fact.get_pair()));
    }
    std::vector<std::pair<int, int>> features =
        generator.attr("generate_features")(indices)
            .cast<std::vector<std::pair<int, int>>>();
    for (const auto &feature : features) {
        co_yield feature;
    }
}

class DLFGeneratorFeature
    : public plugins::TypedFeature<FeatureGenerator, DLFGenerator> {
public:
    DLFGeneratorFeature() : TypedFeature("dlfgen") {
        document_title("DL Feature Generator");

        add_option<int>("concept_c", "Concept complexity limit", "5");
        add_option<int>("role_c", "Role complexity limit", "5");
        add_option<int>("boolean_c", "Boolean complexity limit", "5");
        add_option<int>("count_c", "Count numerical complexity limit", "5");
        add_option<int>(
            "distance_c", "Distance numerical complexity limit", "0");
        add_feature_generator_options_to_feature(*this, "dlfgen");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "ignored by design");
        document_language_support("axioms", "ignored by design");
    }

    virtual std::shared_ptr<DLFGenerator> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<DLFGenerator>(
            opts.get<std::shared_ptr<AbstractTask>>("transform"),
            opts.get<int>("concept_c"), opts.get<int>("role_c"),
            opts.get<int>("boolean_c"), opts.get<int>("count_c"),
            opts.get<int>("distance_c"));
    }
};

static plugins::FeaturePlugin<DLFGeneratorFeature> _plugin;
} // features
