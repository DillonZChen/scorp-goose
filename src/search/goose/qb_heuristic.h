#ifndef GOOSE_QB_HEURISTIC_H
#define GOOSE_QB_HEURISTIC_H

#include "../evaluation_context.h"
#include "../heuristic.h"
#include "../search_statistics.h"

#include "../features/atom_generator.h"
#include "../features/wlf_generator.h"
#include "../utils/logging.h"

#include <map>
#include <memory>

namespace qb_heuristic {
class QbHeuristic : public Heuristic {
protected:
    std::shared_ptr<Evaluator> base_heuristic;
    std::vector<std::shared_ptr<FeatureGenerator>> fgens;
    const int n_fgens;
    utils::LogProxy log;
    SearchStatistics statistics;

    std::map<StateFeature, int> feat_to_min_h;

    virtual int compute_heuristic(const State &ancestor_state) override;

public:
    explicit QbHeuristic(
        const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
        const std::string &description, utils::Verbosity verbosity,
        const std::shared_ptr<Evaluator> base_heuristic,
        std::vector<std::shared_ptr<FeatureGenerator>> feature_generators);
};
} // namespace qb_heuristic

#endif
