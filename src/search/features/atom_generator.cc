#include "atom_generator.h"

#include "../plugins/plugin.h"

namespace features {

AtomGenerator::AtomGenerator(const std::shared_ptr<AbstractTask> &transform)
    : FeatureGenerator(transform) {
}

std::vector<StateFeature> AtomGenerator::compute_features(const State &state) {
    std::vector<StateFeature> features;
    for (const FactProxy &fact : state) {
        features.emplace_back(fact.get_var(), fact.get_value());
    }
    return features;
}

class AtomGeneratorFeature
    : public plugins::TypedFeature<FeatureGenerator, AtomGenerator> {
public:
    AtomGeneratorFeature() : TypedFeature("atomgen") {
        document_title("Atom Generator");

        add_feature_generator_options_to_feature(*this, "atomgen");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "ignored by design");
        document_language_support("axioms", "ignored by design");
    }

    virtual std::shared_ptr<AtomGenerator> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<AtomGenerator>(
            opts.get<std::shared_ptr<AbstractTask>>("transform"));
    }
};

static plugins::FeaturePlugin<AtomGeneratorFeature> _plugin;
} // features
