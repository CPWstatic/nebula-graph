/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "common/base/Base.h"
#include "validator/PipeValidator.h"
#include "parser/TraverseSentences.h"
#include "planner/PlanNode.h"
#include "planner/Query.h"

namespace nebula {
namespace graph {

PipeValidator::PipeValidator(Sentence* sentence, QueryContext* context)
        : Validator(sentence, context) {
    pipeCtx_ = std::make_unique<PipeAstContext>();
    pipeCtx_->sentence = sentence;
    pipeCtx_->qctx = context;
}

Status PipeValidator::validateImpl() {
    auto pipeSentence = static_cast<PipedSentence*>(sentence_);
    auto left = pipeSentence->left();
    pipeCtx_->lValidator = makeValidator(left, qctx_);
    pipeCtx_->lValidator->setInputCols(std::move(inputs_));
    pipeCtx_->lValidator->setInputVarName(inputVarName_);
    NG_RETURN_IF_ERROR(pipeCtx_->lValidator->validate());

    auto right = pipeSentence->right();
    pipeCtx_->rValidator = makeValidator(right, qctx_);
    pipeCtx_->rValidator->setInputCols(pipeCtx_->lValidator->outputCols());
    pipeCtx_->rValidator->setInputVarName(pipeCtx_->lValidator->root()->outputVar());
    NG_RETURN_IF_ERROR(pipeCtx_->rValidator->validate());

    outputs_ = pipeCtx_->rValidator->outputCols();
    return Status::OK();
}

}  // namespace graph
}  // namespace nebula
