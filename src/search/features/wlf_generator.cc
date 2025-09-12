#include "wlf_generator.h"

#include "../task_proxy.h"

#include "../ext/wlplan/include/feature_generator/feature_generator_loader.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/iwl.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/lwl2.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/wl.hpp"
#include "../plugins/plugin.h"

namespace features {
PredArgsString fd_fact_to_pred_args(std::string &name) {
    // Replace all occurrences of '(' and ')' by ' '
    std::replace(name.begin(), name.end(), '(', ' ');
    std::replace(name.begin(), name.end(), ')', ' ');
    // Remove occurrences of ','
    name.erase(std::remove(name.begin(), name.end(), ','), name.end());
    // Trim string
    if (std::isspace(name[0]))
        name.erase(0, 1);
    if (std::isspace(name.back()))
        name.erase(name.end() - 1, name.end());

    std::istringstream iss(name);
    std::string token;
    std::string predicate_name = "";
    std::vector<planning::Object> args;

    while (std::getline(iss, token, ' ')) {
        if (predicate_name == "") {
            predicate_name = token;
        } else {
            args.push_back(token);
        }
    }

    return {predicate_name, args};
}

std::map<FactPair, std::pair<std::string, bool>> get_pddl_facts(
    FactsProxy facts) {
    std::map<FactPair, std::pair<std::string, bool>> ret;
    for (FactProxy fact : facts) {
        std::string name = fact.get_name();
        bool positive;

        // Convert from FDR var-val pairs back to propositions
        if (name == "<none of those>" || name.substr(0, 12) == "NegatedAtom ") {
            continue;
        } else if (name.substr(0, 12) == "NegatedAtom ") {
            name = name.substr(12);
            positive = false;
        } else if (name.substr(0, 5) == "Atom ") {
            name = name.substr(5);
            positive = true;
        } else {
            std::cout
                << "Error: substring of downward fact does not start with 'Atom ': "
                << "or 'NegatedAtom '" << name << std::endl;
            exit(-1);
        }

        ret[fact.get_pair()] = {name, positive};
    }
    return ret;
}

std::map<FactPair, PredArgsString> get_fd_fact_to_pred_args_map(
    const std::shared_ptr<AbstractTask> task) {
    FactsProxy facts(*task);
    std::map<FactPair, PredArgsString> ret;
    for (const auto &[fact_pair, pddl_fact] : get_pddl_facts(facts)) {
        std::string pddl_fact_name = pddl_fact.first;
        bool positive = pddl_fact.second;
        if (!positive) {
            continue;
        }

        std::pair<std::string, std::vector<std::string>> pred_args =
            fd_fact_to_pred_args(pddl_fact_name);

        ret.insert({fact_pair, pred_args});
    }

    return ret;
}

std::pair<
    std::map<FactPair, std::shared_ptr<planning::Atom>>, planning::Problem>
construct_wlplan_problem(
    const planning::Domain &domain,
    const std::map<FactPair, PredArgsString> &mapper,
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
        std::vector<planning::Object> args = pred_args.second;

        for (planning::Object arg : args) {
            objects.insert(arg);
        }

        if (name_to_predicate.count(predicate_name)) {
            planning::Atom wlplan_atom =
                planning::Atom(name_to_predicate.at(predicate_name), args);
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
        std::string name = goal.get_name();
        bool positive;

        // Convert from FDR var-val pairs back to propositions
        if (name == "<none of those>") {
            continue;
        } else if (name.substr(0, 12) == "NegatedAtom ") {
            name = name.substr(12);
            positive = false;
        } else if (name.substr(0, 5) == "Atom ") {
            name = name.substr(5);
            positive = true;
        } else {
            std::cout
                << "Error: substring of downward fact does not start with 'Atom ': "
                << "or 'NegatedAtom '" << name << std::endl;
            exit(-1);
        }

        std::pair<std::string, std::vector<std::string>> pred_args =
            fd_fact_to_pred_args(name);
        std::string predicate_name = pred_args.first;
        std::vector<planning::Object> args = pred_args.second;
        planning::Atom atom =
            planning::Atom(name_to_predicate.at(predicate_name), args);

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
    const std::string &graph_representation, const std::string &wl_algorithm)
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
            domain, graph_representation, wl_iterations, "none", true);
    } else if (wl_algorithm == "lwl2") {
        model = std::make_shared<feature_generator::LWL2Features>(
            domain, graph_representation, wl_iterations, "none", true);
    } else if (wl_algorithm == "iwl") {
        model = std::make_shared<feature_generator::IWLFeatures>(
            domain, graph_representation, wl_iterations, "none", true);
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
    const planning::Domain domain = *(model->get_domain());
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
            opts.get<std::string>("w"));
    }
};

static plugins::FeaturePlugin<WLFGeneratorFeature> _plugin;
} // features
