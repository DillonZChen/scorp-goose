#ifndef FEATURES_FEATURE_GENERATOR_H
#define FEATURES_FEATURE_GENERATOR_H

#include "../task_proxy.h"

#include <vector>

template<typename C>
class FeatureGenerator {
public:
    // TODO implement generators
    virtual C compute_features(const State &state) = 0;

    virtual ~FeatureGenerator() = default;
};

#endif
