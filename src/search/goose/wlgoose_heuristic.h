#ifndef GOOSE_WLGOOSE_HEURISTIC_H
#define GOOSE_WLGOOSE_HEURISTIC_H

#include "../features/wlf_generator.h"
#include "../heuristic.h"

#include <memory>

namespace wlgoose_heuristic {
class WlGooseHeuristic : public Heuristic {
protected:
    std::shared_ptr<features::WLFeatureGenerator> model;
    virtual int compute_heuristic(const State &ancestor_state) override;

public:
    explicit WlGooseHeuristic(
        const std::string &model_file,
        const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
        const std::string &description, utils::Verbosity verbosity);
};
} // namespace wlgoose_heuristic

#endif
