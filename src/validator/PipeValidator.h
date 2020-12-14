/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef VALIDATOR_PIPEVALIDATOR_H_
#define VALIDATOR_PIPEVALIDATOR_H_

#include "common/base/Base.h"
#include "context/ast/QueryAstContext.h"
#include "validator/Validator.h"

namespace nebula {
namespace graph {

class PipeValidator final : public Validator {
public:
    PipeValidator(Sentence* sentence, QueryContext* context);

private:
    Status validateImpl() override;

private:
    std::unique_ptr<PipeAstContext>     pipeCtx_;
};
}  // namespace graph
}  // namespace nebula
#endif  // VALIDATOR_PIPEVALIDATOR_H_
