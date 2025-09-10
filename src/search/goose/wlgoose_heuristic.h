#ifndef GOOSE_WLGOOSE_HEURISTIC_H
#define GOOSE_WLGOOSE_HEURISTIC_H

#include "wl_utils.hpp"

#include "../heuristic.h"

#include <memory>

namespace wlgoose_heuristic {
class WlGooseHeuristic : public Heuristic {
protected:
    std::shared_ptr<wl_utils::WLFeatureGenerator> model;
    wl_utils::DownwardToWlplanAtomMapper fd_fact_to_wlplan_atom;
    virtual int compute_heuristic(const State &ancestor_state) override;

public:
    explicit WlGooseHeuristic(
        const std::string &model_file,
        const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
        const std::string &description, utils::Verbosity verbosity);
};
} // namespace wlgoose_heuristic

#endif
