#include "feature_utils.h"

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
    std::vector<std::string> args;

    while (std::getline(iss, token, ' ')) {
        if (predicate_name == "") {
            predicate_name = token;
        } else {
            args.push_back(token);
        }
    }

    return {predicate_name, args};
}

std::pair<std::string, bool> get_pddl_fact(FactProxy fact) {
    std::string name = fact.get_name();
    bool positive;

    // Convert from FDR var-val pairs back to propositions
    if (name == "<none of those>" || name.substr(0, 12) == "NegatedAtom ") {
        return {"", true};
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

    return {name, positive};
}

std::map<FactPair, PredArgsString> get_fd_fact_to_pred_args_map(
    const std::shared_ptr<AbstractTask> task) {
    FactsProxy facts(*task);
    std::map<FactPair, PredArgsString> ret;
    for (const auto &fact : facts) {
        std::pair<std::string, bool> pddl_fact_info = get_pddl_fact(fact);
        std::string pddl_fact_name = pddl_fact_info.first;
        bool positive = pddl_fact_info.second;
        if (!positive || pddl_fact_name.empty()) {
            continue;
        }

        std::pair<std::string, std::vector<std::string>> pred_args =
            fd_fact_to_pred_args(pddl_fact_name);

        ret.insert({fact.get_pair(), pred_args});
    }

    return ret;
}

} // namespace features
