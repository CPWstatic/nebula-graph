/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef PLANNER_PIPEPLANNER_H_
#define PLANNER_PIPEPLANNER_H_

#include "planner/Planner.h"

namespace nebula {
namespace graph {
class PipePlanner final : public Planner {
public:
    static std::unique_ptr<PipePlanner> make() {
        return std::unique_ptr<PipePlanner>(new PipePlanner());
    }

    static bool match(AstContext* astCtx);

    /**
     * Connect the execution plans for the left and right subtrees.
     * For example: Go FROM id1 OVER e1 YIELD e1._dst AS id | GO FROM $-.id OVER e2;
     * The plan of left subtree's would be:
     *  Project(_dst) -> GetNeighbor(id1, e1)
     * and the right would be:
     *  Project(_dst) -> GetNeighbor(id2, e2)
     * After connecting, it would be:
     *  Project(_dst) -> GetNeighbor(id2, e2) ->
                Project(_dst) -> GetNeighbor(id1, e1)
     */
    StatusOr<SubPlan> transform(AstContext* astCtx) override;

private:
    PipePlanner() = default;
};
}  // namespace graph
}  // namespace nebula
#endif  // PLANNER_PIPEPLANNER_H_
