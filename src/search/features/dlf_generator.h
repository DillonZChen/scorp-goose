#ifndef FEATURES_DLF_GENERATOR_H
#define FEATURES_DLF_GENERATOR_H

#include "feature_utils.h"

#include "../feature_generator.h"
#include "../task_proxy.h"

#include <map>
#include <memory>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>
#include <utility>
#include <vector>

namespace features {

class DLFGenerator : public FeatureGenerator {
private:
    pybind11::scoped_interpreter guard{};
    pybind11::object generator;

    std::map<FactPair, int> fact_to_i;

public:
    DLFGenerator(const std::shared_ptr<AbstractTask> transform);
    ~DLFGenerator();

    Generator<StateFeature> compute_features(const State &state);
};

} // namespace features

#endif
