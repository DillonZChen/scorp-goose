#include "wlf_generator.h"

#include "../task_proxy.h"

#include "../ext/wlplan/include/feature_generator/feature_generator_loader.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/iwl.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/lwl2.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/wl.hpp"
#include "../plugins/plugin.h"

namespace features {

std::pair<
    std::map<FactPair, std::shared_ptr<planning::Atom>>, planning::Problem>
construct_wlplan_problem(
    planning::Domain &domain, const std::map<FactPair, PredArgsString> &mapper,
    const TaskProxy &task_proxy) {
    std::map<FactPair, std::shared_ptr<planning::Atom>> fd_fact_to_wlplan_atom;

    std::unordered_map<std::string, planning::Predicate> name_to_predicate;

    for (const auto &pred : domain.predicates) {
        name_to_predicate[pred.name] = pred;
    }

    std::unordered_set<planning::Object> objects;

    // Preprocess Downward's FDR var-val pairs and map to WLPlan atoms.
    for (auto &[fact_pair, pred_args] : mapper) {
        std::string predicate_name = pred_args.first;
        for (const std::string &obj : pred_args.second) {
            objects.insert(obj);
        }
        if (name_to_predicate.count(predicate_name)) {
            planning::Atom wlplan_atom = planning::Atom(
                name_to_predicate.at(predicate_name), pred_args.second);
            fd_fact_to_wlplan_atom.insert(
                {fact_pair, std::make_shared<planning::Atom>(wlplan_atom)});
        }
    }

    /* Construct a WLPlan Problem from Downward */

    // Sort objects into vector
    std::vector<planning::Object> objects_vec_sorted(
        objects.begin(), objects.end());
    std::sort(objects_vec_sorted.begin(), objects_vec_sorted.end());

    // Deal with goals
    std::vector<planning::Atom> positive_goals;
    std::vector<planning::Atom> negative_goals;

    for (FactProxy goal : task_proxy.get_goals()) {
        std::pair<std::string, bool> pddl_fact_info = get_pddl_fact(goal);
        std::string pddl_fact_name = pddl_fact_info.first;
        bool positive = pddl_fact_info.second;
        if (pddl_fact_name.empty()) {
            continue;
        }

        std::pair<std::string, std::vector<std::string>> pred_args =
            fd_fact_to_pred_args(pddl_fact_name);
        planning::Atom atom = planning::Atom(
            name_to_predicate.at(pred_args.first), pred_args.second);

        if (positive) {
            positive_goals.push_back(atom);
        } else {
            negative_goals.push_back(atom);
        }
    }

    planning::Problem problem = planning::Problem(
        domain, objects_vec_sorted, positive_goals, negative_goals);

    return {fd_fact_to_wlplan_atom, problem};
}

/* WLFeature Generator */

WLFGenerator::WLFGenerator(
    const std::shared_ptr<AbstractTask> transform, int wl_iterations,
    const std::string &graph_representation, const std::string &wl_algorithm,
    const bool multiset_hash)
    : FeatureGenerator(transform) {
    /* Construct domain */
    std::map<FactPair, PredArgsString> helper =
        get_fd_fact_to_pred_args_map(transform);
    // get predicates
    std::set<planning::Predicate> predicates_set;
    for (const auto &[_, pred_args] : helper) {
        const std::string &predicate_name = pred_args.first;
        const int arity = pred_args.second.size();
        predicates_set.insert(planning::Predicate(predicate_name, arity));
    }
    std::vector<planning::Predicate> predicates(
        predicates_set.begin(), predicates_set.end());
    // create domain
    planning::Domain domain = planning::Domain("domain", predicates);

    /* Construct problem */
    auto [mapper_loc, problem] =
        construct_wlplan_problem(domain, helper, task_proxy);
    mapper = mapper_loc;

    /* Initialise feature generator */
    if (wl_algorithm == "wl") {
        model = std::make_shared<feature_generator::WLFeatures>(
            domain, graph_representation, wl_iterations, "none", multiset_hash);
    } else if (wl_algorithm == "lwl2") {
        model = std::make_shared<feature_generator::LWL2Features>(
            domain, graph_representation, wl_iterations, "none", multiset_hash);
    } else if (wl_algorithm == "iwl") {
        model = std::make_shared<feature_generator::IWLFeatures>(
            domain, graph_representation, wl_iterations, "none", multiset_hash);
    } else {
        std::cerr << "Unknown WL algorithm: " << wl_algorithm << std::endl;
        exit(1);
    }

    model->set_problem(problem);
    model->be_quiet();
}

WLFGenerator::WLFGenerator(
    const std::shared_ptr<AbstractTask> transform,
    const std::string &model_file)
    : FeatureGenerator(transform) {
    model = load_feature_generator(model_file);

    /* Get domain from model */
    planning::Domain domain = *(model->get_domain());
    const std::map<FactPair, PredArgsString> &helper =
        get_fd_fact_to_pred_args_map(transform);

    /* Construct problem */
    auto [mapper_loc, problem] =
        construct_wlplan_problem(domain, helper, task_proxy);
    mapper = mapper_loc;

    model->set_problem(problem);
    model->be_quiet();
}

WLFGenerator::~WLFGenerator() {
    // Destructor
    std::cout << "WL features collected: " << model->get_n_features()
              << std::endl;
}

planning::State WLFGenerator::to_wlplan_state(const State &state) const {
    std::vector<std::shared_ptr<planning::Atom>> atoms;
    for (const FactProxy &fact : state) {
        if (mapper.count(fact.get_pair())) {
            atoms.push_back(mapper.at(fact.get_pair()));
        }
    }
    return planning::State(atoms);
}

Generator<StateFeature> WLFGenerator::compute_features(const State &state) {
    planning::State wl_state = to_wlplan_state(state);
    for (const auto &[key, value] : model->collect_embed(wl_state)) {
        if (value == 0) {
            // feature not present, their values do not matter
            continue;
        }
        co_yield std::make_pair(key, value);
    }
}

double WLFGenerator::predict(const State &state) const {
    planning::State wl_state = to_wlplan_state(state);
    double h = model->predict(wl_state);
    return h;
}

class WLFGeneratorFeature
    : public plugins::TypedFeature<FeatureGenerator, WLFGenerator> {
public:
    WLFGeneratorFeature() : TypedFeature("wlfgen") {
        document_title("WL Feature Generator");

        add_option<int>("l", "Number of wl iterations", "2");
        add_option<std::string>("g", "Graph representation", "\"ilg\"");
        add_option<std::string>("w", "WL algorithm", "\"wl\"");
        add_option<bool>("mset", "Use multi-set hash", "false");
        add_feature_generator_options_to_feature(*this, "wlfgen");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "ignored by design");
        document_language_support("axioms", "ignored by design");
    }

    virtual std::shared_ptr<WLFGenerator> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<WLFGenerator>(
            opts.get<std::shared_ptr<AbstractTask>>("transform"),
            opts.get<int>("l"), opts.get<std::string>("g"),
            opts.get<std::string>("w"), opts.get<bool>("mset"));
    }
};

static plugins::FeaturePlugin<WLFGeneratorFeature> _plugin;
} // features
