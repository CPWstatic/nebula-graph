/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "planner/PipePlanner.h"

#include "context/ast/QueryAstContext.h"
#include "parser/Sentence.h"

namespace nebula {
namespace graph {
bool PipePlanner::match(AstContext* astCtx) {
    return astCtx->sentence->kind() == Sentence::Kind::kPipe;
}

StatusOr<SubPlan> PipePlanner::transform(AstContext* astCtx) {
    auto pipeCtx = static_cast<PipeAstContext*>(astCtx);

    SubPlan plan;
    plan.root = pipeCtx->rValidator->root();
    plan.tail = pipeCtx->lValidator->tail();
    NG_RETURN_IF_ERROR(pipeCtx->rValidator->appendPlan(pipeCtx->lValidator->root()));
    auto node = static_cast<SingleInputNode*>(pipeCtx->rValidator->tail());
    if (node->inputVar().empty()) {
        // If the input variable was not set, set it dynamically.
        node->setInputVar(pipeCtx->lValidator->root()->outputVar());
    }
    return Status::OK();
}
}  // namespace graph
}  // namespace nebula
