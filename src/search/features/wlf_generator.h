#ifndef FEATURES_WLF_GENERATOR_H
#define FEATURES_WLF_GENERATOR_H

#include "../feature_generator.h"
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

using namespace wlplan;

namespace features {

using PredArgsString =
    typename std::pair<std::string, std::vector<std::string>>;
using DownwardToWlplanAtomMapper =
    typename std::map<FactPair, std::shared_ptr<planning::Atom>>;

class WLFGenerator : public FeatureGenerator {
protected:
    std::shared_ptr<feature_generator::Features> model;
    DownwardToWlplanAtomMapper mapper;

public:
    WLFGenerator(
        const std::shared_ptr<AbstractTask> transform, int wl_iterations,
        const std::string &graph_representation,
        const std::string &wl_algorithm, const bool multiset_hash);

    WLFGenerator(
        const std::shared_ptr<AbstractTask> transform,
        const std::string &model_file);

    ~WLFGenerator();

    planning::State to_wlplan_state(const State &state) const;

    Generator<StateFeature> compute_features(const State &state);

    double predict(const State &state) const;
};

PredArgsString fd_fact_to_pred_args(std::string &name);
std::pair<std::string, bool> get_pddl_fact(FactProxy fact);

std::map<FactPair, PredArgsString> get_fd_fact_to_pred_args_map(
    const std::shared_ptr<AbstractTask> task);

std::pair<DownwardToWlplanAtomMapper, planning::Problem>
construct_wlplan_problem(
    planning::Domain &domain, const std::map<FactPair, PredArgsString> &mapper,
    const TaskProxy &task_proxy);

} // namespace features

#endif
