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
    std::map<FactPair, PredArgsString> mapper =
        get_fd_fact_to_pred_args_map(transform);

    std::unordered_map<std::string, int> objects;
    std::unordered_map<std::string, int> predicates;

    int max_var = -1;
    int max_val = -1;
    for (const auto &[fact_pair, pred_args] : mapper) {
        // scrape predicates
        if (!predicates.count(pred_args.first)) {
            predicates[pred_args.first] = (int)predicates.size();
        }
        // scrape objects
        for (const std::string &obj : pred_args.second) {
            if (!objects.count(obj)) {
                objects[obj] = (int)objects.size();
            }
        }
        // scrape number of atoms
        if (fact_pair.var > max_var) {
            max_var = fact_pair.var;
        }
        if (fact_pair.value > max_val) {
            max_val = fact_pair.value;
        }
    }

    n_objects = (int)objects.size();
    connected_objects.resize(max_var + 1);
    colour.resize(max_var + 1);
    for (int var = 0; var <= max_var; ++var) {
        connected_objects[var].resize(max_val + 1);
        colour[var].resize(max_val + 1, -1);
    }

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
            goal_colour[{var, val}] = 1 + predicate * 5 + 1;
            pos_goal_set.insert({var, val});
        } else {
            goal_colour[{var, val}] = 1 + predicate * 5 + 3;
            neg_goal_set.insert({var, val});
        }
    }

    // scrape ground atoms
    for (auto &[fact_pair, pred_args] : mapper) {
        var = fact_pair.var;
        val = fact_pair.value;

        // get edges
        std::vector<int> atom_objects;
        for (const std::string &obj : pred_args.second) {
            atom_objects.push_back(objects.at(obj));
        }
        connected_objects[fact_pair.var][fact_pair.value] = atom_objects;

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
    std::cout << "WL features collected: TODO" << std::endl;
}

Generator<StateFeature> WLFmk2Generator::compute_features(const State &state) {
    std::map<int, int> features;

    // init graph
    std::vector<int> obj_colours = std::vector<int>(n_objects, 0);
    std::map<std::pair<int, int>, int> atom_colours;
    int var, val, col;
    std::pair<int, int> pair;
    // copy goal_colour
    for (FactProxy fact : state) {
        pair = fact.get_int_pair();
        var = pair.first;
        val = pair.second;
        atom_colours[pair] = colour[var][val];
    }
    for (const auto &[pair, c] : goal_colour) {
        if (!atom_colours.count(pair)) {
            atom_colours[pair] = c;
        }
    }

    // to be faithful to wlplan implementation, collect initial colours
    features[0] = n_objects;
    for (const auto &entry : atom_colours) {
        col = entry.second;
        features.try_emplace(col, 0);
        features[col] += 1;
    }

    // main wl loop
    for (int iteration = 0; iteration < wl_iterations; iteration++) {
        std::map<std::pair<int, int>, int> new_atom_colours;
        std::vector<std::vector<int>> new_obj_colours_long =
            std::vector<std::vector<int>>(n_objects);
        std::vector<int> new_obj_colours(n_objects, 0);

        for (const auto &[pair, c] : atom_colours) {
            var = pair.first;
            val = pair.second;
            std::vector<int> neigh_colours;
            for (int obj : connected_objects[var][val]) {
                neigh_colours.push_back(obj_colours[obj]);
                new_obj_colours_long[obj].push_back(c);
            }
            // sort neighbours then add own colour
            std::sort(neigh_colours.begin(), neigh_colours.end());
            neigh_colours.push_back(c);

            col = get_hash_colour(neigh_colours);
            new_atom_colours[pair] = col;
            features.try_emplace(col, 0);
            features[col] += 1;
        }
        for (int obj = 0; obj < n_objects; obj++) {
            // sort neighbours then add own colour
            std::sort(
                new_obj_colours_long[obj].begin(),
                new_obj_colours_long[obj].end());
            new_obj_colours_long[obj].push_back(obj_colours[obj]);

            col = get_hash_colour(new_obj_colours_long[obj]);
            new_obj_colours[obj] = col;
            features.try_emplace(col, 0);
            features[col] += 1;
        }

        atom_colours = new_atom_colours;
        obj_colours = new_obj_colours;
    }

    for (const auto &[f, count] : features) {
        co_yield std::make_pair(f, count);
    }
}

int WLFmk2Generator::get_hash_colour(const std::vector<int> &colours) {
    hash.try_emplace(colours, (int)hash.size());
    return hash[colours];
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
