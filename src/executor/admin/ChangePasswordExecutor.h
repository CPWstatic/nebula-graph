/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef EXECUTOR_ADMIN_CHANGEPASSWORDEXECUTOR_H_
#define EXECUTOR_ADMIN_CHANGEPASSWORDEXECUTOR_H_

#include "executor/Executor.h"

namespace nebula {
namespace graph {

class ChangePasswordExecutor final : public Executor {
public:
    ChangePasswordExecutor(const PlanNode *node, QueryContext *ectx)
        : Executor("ChangePasswordExecutor", node, ectx) {}

    folly::Future<Status> execute() override;

private:
    folly::Future<Status> changePassword();
};

}   // namespace graph
}   // namespace nebula

#endif  // EXECUTOR_ADMIN_CHANGEPASSWORDEXECUTOR_H_
