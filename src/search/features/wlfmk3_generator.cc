#include "wlfmk3_generator.h"

#include "../task_proxy.h"

namespace features {

WLFmk3Generator::WLFmk3Generator(
    const std::shared_ptr<AbstractTask> transform, int wl_iterations)
    : FeatureGenerator(transform), wl_iterations(wl_iterations) {
    std::unordered_map<std::string, int> obj_to_i;
    n_vars = 0;
    n_vals = 0;
    max_arity = 0;
    int fact_i;

    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;
    std::cout << "still work in progress" << std::endl;

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
        n_vars = std::max(n_vars, fact_pair.var);
        n_vals = std::max(n_vals, fact_pair.value);
    }

    n_vars += 1;
    n_vals += 1;
    int n_facts = n_vars * n_vals;
    ci.resize(n_facts);
    skip.resize(n_facts);
    pred_i.resize(n_facts);
    arity.resize(n_facts);
    edge_atom_colours.resize(n_facts);

    std::unordered_map<std::string, int> nullary_pred_to_i;
    std::unordered_map<std::string, int> unary_pred_to_i;
    std::unordered_map<std::string, int> nary_pred_to_i;
    for (const auto &[fact_pair, pred_args] : mapper) {
        // scrape predicates
        std::string predicate_name = pred_args.first;
        fact_i = fact_pair.var * n_vals + fact_pair.value;
        if (predicate_name == "--bad--") {
            skip[fact_i] = true;
            continue;
        }
        // init pred index
        int pred_arity = (int)pred_args.second.size();
        if (pred_arity == 0 && !nullary_pred_to_i.count(predicate_name)) {
            nullary_pred_to_i[predicate_name] = (int)nullary_pred_to_i.size();
        }
        if (pred_arity == 1 && !unary_pred_to_i.count(predicate_name)) {
            unary_pred_to_i[predicate_name] = (int)unary_pred_to_i.size();
        }
        if (pred_arity >= 2 && !nary_pred_to_i.count(predicate_name)) {
            nary_pred_to_i[predicate_name] = (int)nary_pred_to_i.size();
        }
        max_arity = std::max(max_arity, pred_arity);
        // set pred index of fact
        if (pred_arity == 0) {
            pred_i[fact_i] = nullary_pred_to_i.at(predicate_name);
        }
        if (pred_arity == 1) {
            pred_i[fact_i] = unary_pred_to_i.at(predicate_name);
        }
        if (pred_arity >= 2) {
            pred_i[fact_i] = nary_pred_to_i.at(predicate_name);
        }
        // scrape objects
        for (const std::string &obj : pred_args.second) {
            if (!obj_to_i.count(obj)) {
                obj_to_i[obj] = (int)obj_to_i.size();
            }
        }
        arity[fact_i] = pred_arity;
    }

    n_objects = (int)obj_to_i.size();
    node_colours = std::vector<int>(n_objects, 0);

    // scrape goals
    for (FactProxy goal : task_proxy.get_goals()) {
        std::pair<int, int> pair = goal.get_int_pair();
        fact_i = pair.first * n_vals + pair.second;
        std::pair<std::string, bool> pddl_fact_info = get_pddl_fact(goal);
        std::string pddl_fact_name = pddl_fact_info.first;
        bool positive = pddl_fact_info.second;
        if (pddl_fact_name.empty()) {
            continue;
        }
        std::pair<std::string, std::vector<std::string>> pred_args =
            fd_fact_to_pred_args(pddl_fact_name);
        if (!positive) {
            std::cout << "Error: negative goals not supported" << std::endl;
            exit(1);
        }

        if (pred_args.second.size() == 1) {
            int obj = obj_to_i.at(pred_args.second[0]);
            node_colours[obj] =
                (1 << (pred_i.at(fact_i) + unary_pred_to_i.size()));
        } else if (pred_args.second.size() >= 2) {
            edge_goal_colours[fact_i] = 3 * pred_i[fact_i] + 0; // UG
        }
    }

    // scrape ground atoms
    int cnt = 0;
    std::map<int, std::vector<int>> sorted_connected_objects;
    for (auto &[fact_pair, pred_args] : mapper) {
        std::string predicate_name = pred_args.first;
        if (predicate_name == "--bad--") {
            continue;
        }
        fact_i = fact_pair.var * n_vals + fact_pair.value;

        // get edges
        std::vector<int> atom_objects;
        for (const std::string &obj : pred_args.second) {
            atom_objects.push_back(obj_to_i.at(obj));
        }
        sorted_connected_objects[fact_i] = atom_objects;
        cnt += (int)atom_objects.size();

        if (arity.at(fact_i) <= 1) {
            continue;
        }

        // get colour
        if (edge_goal_colours.count(fact_i)) {
            edge_atom_colours[fact_i] = 3 * pred_i[fact_i] + 1; // AG
        } else {
            edge_atom_colours[fact_i] = 3 * pred_i[fact_i] + 2; // AP
        }
    }

    // flatten connected_objects
    int count = 0;
    connected_objects.clear();
    connected_objects.reserve(cnt);
    for (int i = 0; i < n_facts; i++) {
        ci[i] = count;
        if (sorted_connected_objects.count(i)) {
            for (int obj : sorted_connected_objects[i]) {
                connected_objects.push_back(obj);
                count++;
            }
        }
    }
}

WLFmk3Generator::~WLFmk3Generator() {
    // Destructor
    std::cout << "WL features collected: " << hash.size() << std::endl;
}

Generator<StateFeature> WLFmk3Generator::compute_features(const State &state) {
    std::unordered_map<int, int> features;

    // --- Graph Initialization ---
    std::vector<int> obj_colours = node_colours;

    std::unordered_map<int, int> edge_atom_colours_map;
    std::pair<int, int> pair;
    int col, fact_i;

    // copy edge_atom_colours_map
    for (FactProxy fact : state) {
        pair = fact.get_int_pair();
        fact_i = pair.first * n_vals + pair.second;
        if (skip[fact_i]) {
            continue;
        }
        if (arity[fact_i] == 0) {
            // nullary atoms don't exist in the graph
            features[pred_i[fact_i]] = 1;
        } else if (arity[fact_i] == 1) {
            // unary atoms colour nodes
            obj_colours[connected_objects[ci[fact_i]]] += (1 << pred_i[fact_i]);
        } else {
            // n-ary atoms colour edges
            edge_atom_colours_map[fact_i] = edge_atom_colours[fact_i];
        }
    }
    for (const auto &[i, c] : edge_goal_colours) {
        edge_atom_colours_map.try_emplace(i, c);
    }

    // --- Initial Feature Collection ---
    for (size_t i = 0; i < obj_colours.size(); i++) {
        col = obj_colours[i];
        hash.try_emplace({{col, 0}}, (int)hash.size());
        col = hash[{{col, 0}}];
        features[col]++;
        obj_colours[i] = col;
    }

    // --- Main WL Loop ---
    std::vector<std::vector<std::pair<int, int>>> object_neighbours(n_objects);
    std::vector<int> new_obj_colours(n_objects);
    int n, l, o1, o2;

    for (int iteration = 1; iteration < wl_iterations + 1; iteration++) {
        for (int i = 0; i < n_objects; i++) {
            object_neighbours[i].clear();
        }

        for (const auto &[fact_i, c] : edge_atom_colours_map) {
            n = arity[fact_i];
            l = ci[fact_i];
            for (int i = 0; i < n; i++) {
                o1 = connected_objects[l + i];
                for (int j = i + 1; j < n; j++) {
                    o2 = connected_objects[l + j];
                    object_neighbours[o1].emplace_back(
                        obj_colours[o2],
                        i + max_arity * j + max_arity * max_arity * c);
                    object_neighbours[o2].emplace_back(
                        obj_colours[o1],
                        j + max_arity * i + max_arity * max_arity * c);
                }
            }
        }

        for (int obj_i = 0; obj_i < n_objects; obj_i++) {
            // sort neighbours then add own colour
            std::sort(
                object_neighbours[obj_i].begin(),
                object_neighbours[obj_i].end());
            object_neighbours[obj_i].emplace_back(
                obj_colours[obj_i], iteration);

            hash.try_emplace(object_neighbours[obj_i], (int)hash.size());
            col = hash[object_neighbours[obj_i]];
            new_obj_colours[obj_i] = col;
            features.try_emplace(col, 0);
            features[col]++;
        }

        std::swap(obj_colours, new_obj_colours);
    }

    for (const auto &[f, count] : features) {
        co_yield std::make_pair(f, count);
    }
}

class WLFmk3GeneratorFeature
    : public plugins::TypedFeature<FeatureGenerator, WLFmk3Generator> {
public:
    WLFmk3GeneratorFeature() : TypedFeature("wlfgenmk3") {
        document_title("WL Feature Generator");

        add_option<int>("l", "Number of wl iterations", "2");
        add_feature_generator_options_to_feature(*this, "wlfgenmk3");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "ignored by design");
        document_language_support("axioms", "ignored by design");
    }

    virtual std::shared_ptr<WLFmk3Generator> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<WLFmk3Generator>(
            opts.get<std::shared_ptr<AbstractTask>>("transform"),
            opts.get<int>("l"));
    }
};

static plugins::FeaturePlugin<WLFmk3GeneratorFeature> _plugin;
} // features
