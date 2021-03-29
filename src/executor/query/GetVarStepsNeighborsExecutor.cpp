/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "executor/query/GetVarStepsNeighborsExecutor.h"

#include <sstream>

#include "common/clients/storage/GraphStorageClient.h"
#include "common/datatypes/List.h"
#include "common/datatypes/Vertex.h"
#include "context/QueryContext.h"
#include "util/SchemaUtil.h"
#include "util/ScopedTimer.h"
#include "service/GraphFlags.h"

using nebula::storage::StorageRpcResponse;
using nebula::storage::cpp2::GetNeighborsResponse;
using nebula::storage::GraphStorageClient;

namespace nebula {
namespace graph {

folly::Future<Status> GetVarStepsNeighborsExecutor::execute() {
    steps_ = gn_->steps();
    currentStep_ = 1;
    auto status = buildRequestDataSet();
    if (!status.ok()) {
        return error(std::move(status));
    }
    return getVarStepsNeighbors();
}

Status GetVarStepsNeighborsExecutor::close() {
    // clear the members
    reqDs_.rows.clear();
    return Executor::close();
}

Status GetVarStepsNeighborsExecutor::buildRequestDataSet() {
    SCOPED_TIMER(&execTime_);
    auto inputVar = gn_->inputVar();
    VLOG(1) << node()->outputVar() << " : " << inputVar;
    auto& inputResult = ectx_->getResult(inputVar);
    auto iter = inputResult.iter();
    QueryExpressionContext ctx(ectx_);
    DataSet input;
    reqDs_.colNames = {kVid};
    reqDs_.rows.reserve(iter->size());
    auto* src = DCHECK_NOTNULL(gn_->src());
    std::unordered_set<Value> uniqueVid;
    const auto& spaceInfo = qctx()->rctx()->session()->space();
    for (; iter->valid(); iter->next()) {
        auto val = Expression::eval(src, ctx(iter.get()));
        if (!SchemaUtil::isValidVid(val, spaceInfo.spaceDesc.vid_type)) {
            continue;
        }
        if (gn_->dedup()) {
            auto ret = uniqueVid.emplace(val);
            if (ret.second) {
                reqDs_.rows.emplace_back(Row({std::move(val)}));
            }
        } else {
            reqDs_.rows.emplace_back(Row({std::move(val)}));
        }
    }
    return Status::OK();
}

folly::Future<Status> GetVarStepsNeighborsExecutor::getVarStepsNeighbors() {
    getNeighbors();
    return promise_.getFuture();
}

folly::Future<Status> GetVarStepsNeighborsExecutor::getNeighbors() {
    if (reqDs_.rows.empty()) {
        VLOG(1) << "Empty input.";
        List emptyResult;
        return finish(ResultBuilder()
                          .value(Value(std::move(emptyResult)))
                          .iter(Iterator::Kind::kGetNeighbors)
                          .finish());
    }

    time::Duration getNbrTime;
    GraphStorageClient* storageClient = qctx_->getStorageClient();
    return storageClient
        ->getNeighbors(gn_->space(),
                       std::move(reqDs_.colNames),
                       std::move(reqDs_.rows),
                       gn_->edgeTypes(),
                       gn_->edgeDirection(),
                       gn_->statProps(),
                       gn_->vertexProps(),
                       gn_->edgeProps(),
                       gn_->exprs(),
                       gn_->dedup(),
                       gn_->random(),
                       gn_->orderBy(),
                       gn_->limit(),
                       gn_->filter())
        .via(runner())
        .ensure([this, getNbrTime]() {
            SCOPED_TIMER(&execTime_);
            otherStats_.emplace("total_rpc_time",
                                folly::stringPrintf("%lu(us)", getNbrTime.elapsedInUSec()));
        })
        .then([this](StorageRpcResponse<GetNeighborsResponse>&& resp) {
            SCOPED_TIMER(&execTime_);
            auto& hostLatency = resp.hostLatency();
            for (size_t i = 0; i < hostLatency.size(); ++i) {
                auto& info = hostLatency[i];
                otherStats_.emplace(folly::stringPrintf("%s exec/total/vertices",
                                                        std::get<0>(info).toString().c_str()),
                                    folly::stringPrintf("%d(us)/%d(us)/%lu,",
                                                        std::get<1>(info),
                                                        std::get<2>(info),
                                                        resp.responses()[i].vertices.size()));
            }
            return handleResponse(resp);
        });
}

Status GetVarStepsNeighborsExecutor::handleResponse(RpcResponse& resps) {
    auto result = handleCompleteness(resps, FLAGS_accept_partial_success);
    NG_RETURN_IF_ERROR(result);

    auto& responses = resps.responses();
    VLOG(1) << "Resp size: " << responses.size();
    List list;
    for (auto& resp : responses) {
        auto dataset = resp.get_vertices();
        if (dataset == nullptr) {
            LOG(INFO) << "Empty dataset in response";
            continue;
        }

        VLOG(1) << "Resp row size: " << dataset->rows.size() << "Resp : " << *dataset;
        list.values.emplace_back(std::move(*dataset));
    }

    if (currentStep_ == steps_) {
        ResultBuilder builder;
        builder.state(result.value());
        builder.value(Value(std::move(list)));
        finish(builder.iter(Iterator::Kind::kGetNeighbors).finish());
        promise_.setValue(Status::OK());
    } else {
        // get next steps start vids
    }
    return Status::OK();
}

}   // namespace graph
}   // namespace nebula