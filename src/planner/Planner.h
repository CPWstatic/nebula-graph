/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef PLANNER_PLANNERS_PLANNER_H_
#define PLANNER_PLANNERS_PLANNER_H_

#include "common/base/Base.h"
#include "planner/PlanNode.h"
#include "context/AstContext.h"

namespace nebula {
namespace graph {
class Planner;

struct SubPlan {
    // root and tail of a subplan.
    PlanNode*   root{nullptr};
    PlanNode*   tail{nullptr};
};

using MatchFunc = std::function<bool(AstContext* astContext)>;
using PlannerInstanceFunc = std::function<std::unique_ptr<Planner>()>;
struct MatchAndInstance {
    MatchAndInstance(MatchFunc m, PlannerInstanceFunc p)
        : match(std::move(m)), instance(std::move(p)) {}
    MatchFunc match;
    PlannerInstanceFunc instance;
};

class Planner {
public:
    virtual ~Planner() = default;

    static auto& plannersMap() {
        return plannersMap_;
    }

    static StatusOr<SubPlan> toPlan(AstContext* astCtx);

    virtual StatusOr<SubPlan> transform(AstContext* astCtx) = 0;

protected:
    Planner() = default;

private:
    static std::unordered_map<Sentence::Kind, std::vector<MatchAndInstance>> plannersMap_;
};
}  // namespace graph
}  // namespace nebula
#endif  // PLANNER_PLANNERS_PLANNER_H_
