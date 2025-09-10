#ifndef GOOSE_WL_UTILS_H
#define GOOSE_WL_UTILS_H

#include "../task_proxy.h"

#include "../ext/wlplan/include/feature_generator/features.hpp"
#include "../ext/wlplan/include/planning/atom.hpp"
#include "../ext/wlplan/include/planning/predicate.hpp"
#include "../ext/wlplan/include/planning/problem.hpp"
#include "../ext/wlplan/include/planning/state.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace wl_utils {

using WLFeature = std::pair<int, int>;
using PredArgsString = std::pair<std::string, std::vector<std::string>>;
using DownwardToWlplanAtomMapper =
    std::map<FactPair, std::shared_ptr<planning::Atom>>;

class WLFeatureGenerator {
protected:
    std::shared_ptr<feature_generator::Features> model;
    DownwardToWlplanAtomMapper mapper;

public:
    WLFeatureGenerator(
        const std::shared_ptr<AbstractTask> task, const TaskProxy &task_proxy,
        int wl_iterations, const std::string &graph_representation,
        const std::string &wl_algorithm);

    WLFeatureGenerator(
        const std::shared_ptr<AbstractTask> task, const TaskProxy &task_proxy,
        const std::string &model_file);

    planning::State to_wlplan_state(const State &state) const;

    std::unordered_map<int, int> collect_embed(const State &state);

    double predict(const State &state) const;
};

PredArgsString fd_fact_to_pred_args(std::string &name);
std::map<FactPair, std::pair<std::string, bool>> get_pddl_facts(
    FactsProxy facts);

std::map<FactPair, PredArgsString> get_fd_fact_to_pred_args_map(
    const std::shared_ptr<AbstractTask> task);

std::pair<DownwardToWlplanAtomMapper, planning::Problem>
construct_wlplan_problem(
    const planning::Domain &domain,
    const std::map<FactPair, PredArgsString> &mapper,
    const TaskProxy &task_proxy);

} // namespace wl_utils

#endif // GOOSE_WL_UTILS_H
