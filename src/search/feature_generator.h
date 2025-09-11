#ifndef FEATURES_FEATURE_GENERATOR_H
#define FEATURES_FEATURE_GENERATOR_H

#include "task_proxy.h"

#include "plugins/plugin.h"

#include <vector>

// We could use templates to generalised this. However, I'm not sure how this
// would work with Fast Downward plugins
using StateFeature = typename std::pair<int, int>;
using StateFeatureIndexed = typename std::pair<StateFeature, int>;

class FeatureGenerator {
protected:
    // Hold a reference to the task implementation and pass it to objects that
    // need it.
    const std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;

public:
    FeatureGenerator(const std::shared_ptr<AbstractTask> &transform);

    virtual ~FeatureGenerator() = default;

    // NOTE: could be optimised by implementing generators?
    virtual std::vector<StateFeature> compute_features(const State &state) = 0;
};

extern void add_feature_generator_options_to_feature(
    plugins::Feature &feature, const std::string &description);

#endif
