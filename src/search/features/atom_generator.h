#ifndef FEATURES_ATOM_GENERATOR_H
#define FEATURES_ATOM_GENERATOR_H

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

namespace features {
class AtomGenerator : public FeatureGenerator {
public:
    AtomGenerator(const std::shared_ptr<AbstractTask> &transform);
    std::vector<StateFeature> compute_features(const State &state);
};
} // namespace features

#endif
