/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef EXECUTOR_QUERY_GETNSTEPSNEIGHBORSEXECUTOR_H_
#define EXECUTOR_QUERY_GETNSTEPSNEIGHBORSEXECUTOR_H_

#include <vector>

#include "common/base/StatusOr.h"
#include "common/datatypes/Value.h"
#include "common/datatypes/Vertex.h"
#include "common/interface/gen-cpp2/storage_types.h"
#include "common/clients/storage/GraphStorageClient.h"

#include "executor/StorageAccessExecutor.h"
#include "planner/Query.h"

namespace nebula {
namespace graph {
class GetVarStepsNeighborsExecutor final : public StorageAccessExecutor {
public:
    GetVarStepsNeighborsExecutor(const PlanNode *node, QueryContext *qctx)
        : StorageAccessExecutor("GetVarStepsNeighborsExecutor", node, qctx) {
        gn_ = asNode<GetVarStepsNeighbors>(node);
    }

    folly::Future<Status> execute() override;

    Status close() override;

private:
    Status buildRequestDataSet();

    folly::Future<Status> getVarStepsNeighbors();

    folly::Future<Status> getNeighbors();

    using RpcResponse = storage::StorageRpcResponse<storage::cpp2::GetNeighborsResponse>;
    Status handleResponse(RpcResponse& resps);

private:
    DataSet                             reqDs_;
    const GetVarStepsNeighbors*         gn_{nullptr};
    folly::Promise<Status>              promise_;
    size_t                              steps_{0};
    size_t                              currentStep_{0};
};

}   // namespace graph
}   // namespace nebula

#endif   // EXECUTOR_QUERY_GETVARSTEPSNEIGHBORSEXECUTOR_H_