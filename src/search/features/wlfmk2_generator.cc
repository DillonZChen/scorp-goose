#include "wlfmk2_generator.h"

#include "../task_proxy.h"

#include "../ext/wlplan/include/feature_generator/feature_generator_loader.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/iwl.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/lwl2.hpp"
#include "../ext/wlplan/include/feature_generator/feature_generators/wl.hpp"
#include "../plugins/plugin.h"

namespace features {

WLFmk2Generator::WLFmk2Generator(
    const std::shared_ptr<AbstractTask> transform, int wl_iterations)
    : FeatureGenerator(transform), wl_iterations(wl_iterations) {
    std::unordered_map<std::string, int> objects;
    std::unordered_map<std::string, int> predicates;

    int max_var = -1;
    int max_val = -1;
    FactsProxy facts(*transform);
    std::map<FactPair, PredArgsString> mapper;
    for (const auto &fact : facts) {
        std::pair<std::string, bool> pddl_fact_info = get_pddl_fact(fact);
        std::string pddl_fact_name = pddl_fact_info.first;
        bool positive = pddl_fact_info.second;
        FactPair fact_pair = fact.get_pair();
        if (!positive || pddl_fact_name.empty()) {
            mapper[fact_pair] = {"--bad--", {}};
        } else {
            mapper[fact_pair] = fd_fact_to_pred_args(pddl_fact_name);
        }
        // scrape number of atoms
        if (fact_pair.var > max_var) {
            max_var = fact_pair.var;
        }
        if (fact_pair.value > max_val) {
            max_val = fact_pair.value;
        }
    }

    connected_objects.resize(max_var + 1);
    colour.resize(max_var + 1);
    skip.resize(max_var + 1);
    for (int var = 0; var <= max_var; ++var) {
        connected_objects[var].resize(max_val + 1);
        colour[var].resize(max_val + 1, -1);
        skip[var].resize(max_val + 1, false);
    }

    max_arity = 0;
    for (const auto &[fact_pair, pred_args] : mapper) {
        // scrape predicates
        std::string predicate_name = pred_args.first;
        if (predicate_name == "--bad--") {
            skip[fact_pair.var][fact_pair.value] = true;
            continue;
        }
        if (!predicates.count(predicate_name)) {
            predicates[predicate_name] = (int)predicates.size();
        }
        max_arity = std::max(max_arity, (int)pred_args.second.size());
        // scrape objects
        for (const std::string &obj : pred_args.second) {
            if (!objects.count(obj)) {
                objects[obj] = (int)objects.size();
            }
        }
    }

    n_objects = (int)objects.size();

    // scrape goals
    int var, val, predicate;
    std::set<std::pair<int, int>> pos_goal_set;
    std::set<std::pair<int, int>> neg_goal_set;
    for (FactProxy goal : task_proxy.get_goals()) {
        std::pair<int, int> pair = goal.get_int_pair();
        var = pair.first;
        val = pair.second;
        std::pair<std::string, bool> pddl_fact_info = get_pddl_fact(goal);
        std::string pddl_fact_name = pddl_fact_info.first;
        bool positive = pddl_fact_info.second;
        if (pddl_fact_name.empty()) {
            continue;
        }
        std::pair<std::string, std::vector<std::string>> pred_args =
            fd_fact_to_pred_args(pddl_fact_name);
        predicate = predicates.at(pred_args.first);
        if (positive) {
            goal_colour[pair] = 1 + predicate * 5 + 1;
            pos_goal_set.insert(pair);
        } else {
            goal_colour[pair] = 1 + predicate * 5 + 3;
            neg_goal_set.insert(pair);
        }
    }

    // scrape ground atoms
    for (auto &[fact_pair, pred_args] : mapper) {
        std::string predicate_name = pred_args.first;
        if (predicate_name == "--bad--") {
            continue;
        }
        var = fact_pair.var;
        val = fact_pair.value;

        // get edges
        std::vector<int> atom_objects;
        for (const std::string &obj : pred_args.second) {
            atom_objects.push_back(objects.at(obj));
        }
        connected_objects[var][val] = atom_objects;

        // get colour
        predicate = predicates.at(pred_args.first);
        if (pos_goal_set.count({var, val})) {
            colour[var][val] = 1 + predicate * 5 + 0;
        } else if (neg_goal_set.count({var, val})) {
            colour[var][val] = 1 + predicate * 5 + 2;
        } else {
            colour[var][val] = 1 + predicate * 5 + 4;
        }
    }
}

WLFmk2Generator::~WLFmk2Generator() {
    // Destructor
    std::cout << "WL features collected: " << hash.size() << std::endl;
}

Generator<StateFeature> WLFmk2Generator::compute_features(const State &state) {
    std::map<int, int> features;

    // --- Graph Initialization ---
    std::vector<int> obj_colours = std::vector<int>(n_objects, 0);
    std::unordered_map<std::pair<int, int>, int, wlf_mk2_pair_hash>
        atom_colours;
    int var, val, col;

    // copy goal_colour
    std::pair<int, int> pair;
    for (FactProxy fact : state) {
        pair = fact.get_int_pair();
        if (skip[pair.first][pair.second]) {
            continue;
        }
        atom_colours[pair] = colour[pair.first][pair.second];
    }
    for (const auto &[pair, c] : goal_colour) {
        atom_colours.try_emplace(pair, c);
    }

    // --- Initial Feature Collection ---
    features[0] = n_objects;
    hash[{0, 0}] = 0;
    for (const auto &[pair, color] : atom_colours) {
        hash.try_emplace({color, 0}, (int)hash.size());
        col = hash[{color, 0}];
        features[col]++;
        atom_colours[pair] = col;
    }

    // --- Main WL Loop ---
    std::unordered_map<std::pair<int, int>, int, wlf_mk2_pair_hash>
        new_atom_colours;
    std::vector<std::vector<int>> object_neighbours(n_objects);
    std::vector<int> new_obj_colours(n_objects);
    std::vector<int> atom_neighbours;
    int n, obj;

    for (int iteration = 1; iteration < wl_iterations + 1; iteration++) {
        new_atom_colours.clear();
        for (int i = 0; i < n_objects; i++) {
            object_neighbours[i].clear();
        }

        // --- Atom Color Update ---
        for (const auto &[pair, c] : atom_colours) {
            var = pair.first;
            val = pair.second;
            n = connected_objects[var][val].size();
            atom_neighbours.clear();
            atom_neighbours.reserve(n);
            for (int i = 0; i < n; i++) {
                obj = connected_objects[var][val][i];
                atom_neighbours.push_back(obj_colours[obj] * max_arity + i);
                object_neighbours[obj].push_back(c * max_arity + i);
            }
            // sort neighbours then add own colour
            std::sort(atom_neighbours.begin(), atom_neighbours.end());
            atom_neighbours.push_back(c);
            atom_neighbours.push_back(iteration);

            hash.try_emplace(atom_neighbours, (int)hash.size());
            col = hash[atom_neighbours];

            new_atom_colours[pair] = col;
            features[col]++; // fine if col does not exist in features in cpp
        }

        // --- Object Color Update ---
        for (int obj = 0; obj < n_objects; obj++) {
            // sort neighbours then add own colour
            std::sort(
                object_neighbours[obj].begin(), object_neighbours[obj].end());
            object_neighbours[obj].push_back(obj_colours[obj]);
            object_neighbours[obj].push_back(iteration);

            hash.try_emplace(object_neighbours[obj], (int)hash.size());
            col = hash[object_neighbours[obj]];

            new_obj_colours[obj] = col;
            features[col]++; // fine if col does not exist in features in cpp
        }

        atom_colours = std::move(new_atom_colours);
        obj_colours = std::move(new_obj_colours);
    }

    for (const auto &[f, count] : features) {
        co_yield std::make_pair(f, count);
    }
}

class WLFmk2GeneratorFeature
    : public plugins::TypedFeature<FeatureGenerator, WLFmk2Generator> {
public:
    WLFmk2GeneratorFeature() : TypedFeature("wlfgenmk2") {
        document_title("WL Feature Generator");

        add_option<int>("l", "Number of wl iterations", "2");
        add_feature_generator_options_to_feature(*this, "wlfgenmk2");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "ignored by design");
        document_language_support("axioms", "ignored by design");
    }

    virtual std::shared_ptr<WLFmk2Generator> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<WLFmk2Generator>(
            opts.get<std::shared_ptr<AbstractTask>>("transform"),
            opts.get<int>("l"));
    }
};

static plugins::FeaturePlugin<WLFmk2GeneratorFeature> _plugin;
} // features
