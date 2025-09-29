#ifndef FEATURES_FEATURE_UTILS_H
#define FEATURES_FEATURE_UTILS_H

#include "../feature_generator.h"
#include "../task_proxy.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace features {

using PredArgsString =
    typename std::pair<std::string, std::vector<std::string>>;

PredArgsString fd_fact_to_pred_args(std::string &name);
std::pair<std::string, bool> get_pddl_fact(FactProxy fact);

std::map<FactPair, PredArgsString> get_fd_fact_to_pred_args_map(
    const std::shared_ptr<AbstractTask> task);

} // namespace features

#endif
