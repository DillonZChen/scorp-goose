#include "feature_generator.h"

FeatureGenerator::FeatureGenerator(
    const std::shared_ptr<AbstractTask> &transform)
    : task(transform), task_proxy(*task) {
}

void add_feature_generator_options_to_feature(
    plugins::Feature &feature, const std::string &description) {
    feature.add_option<std::shared_ptr<AbstractTask>>(
        "transform",
        "Optional task transformation for the heuristic."
        " Currently, adapt_costs() and no_transform() are available.",
        "no_transform()");
    feature.add_option<std::string>(
        "description", "description used to identify feature generator in logs",
        "\"" + description + "\"");
    utils::add_log_options_to_feature(feature);
}

static class FeatureGeneratorCategoryPlugin
    : public plugins::TypedCategoryPlugin<FeatureGenerator> {
public:
    FeatureGeneratorCategoryPlugin() : TypedCategoryPlugin("FeatureGenerator") {
        document_synopsis(
            "A feature generator generates features for a state.");
        allow_variable_binding();
    }
} _category_plugin;
