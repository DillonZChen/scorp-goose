#ifndef GOOSE_QB_PNWL_HEURISTIC_H
#define GOOSE_QB_PNWL_HEURISTIC_H

#include "qb_heuristic.h"
#include "wl_utils.hpp"

#include <memory>

namespace qb_heuristic {
class QbAtWlHeuristic : public QbHeuristic {
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;

public:
    explicit QbAtWlHeuristic(
        const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
        const std::string &description, utils::Verbosity verbosity,
        const std::shared_ptr<Evaluator> base_heuristic, int wl_iterations,
        const std::string &graph_representation,
        const std::string &wl_algorithm);
};
} // namespace qb_heuristic

#endif
