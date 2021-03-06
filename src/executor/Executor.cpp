/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "executor/Executor.h"

#include <folly/String.h>
#include <folly/executors/InlineExecutor.h>

#include "common/interface/gen-cpp2/graph_types.h"
#include "context/ExecutionContext.h"
#include "context/QueryContext.h"
#include "executor/ExecutionError.h"
#include "executor/admin/BalanceExecutor.h"
#include "executor/admin/BalanceLeadersExecutor.h"
#include "executor/admin/ChangePasswordExecutor.h"
#include "executor/admin/CharsetExecutor.h"
#include "executor/admin/ConfigExecutor.h"
#include "executor/admin/CreateUserExecutor.h"
#include "executor/admin/DropUserExecutor.h"
#include "executor/admin/GrantRoleExecutor.h"
#include "executor/admin/ListRolesExecutor.h"
#include "executor/admin/ListUserRolesExecutor.h"
#include "executor/admin/ListUsersExecutor.h"
#include "executor/admin/PartExecutor.h"
#include "executor/admin/RevokeRoleExecutor.h"
#include "executor/admin/ShowBalanceExecutor.h"
#include "executor/admin/ShowHostsExecutor.h"
#include "executor/admin/SnapshotExecutor.h"
#include "executor/admin/SpaceExecutor.h"
#include "executor/admin/StopBalanceExecutor.h"
#include "executor/admin/SubmitJobExecutor.h"
#include "executor/admin/SwitchSpaceExecutor.h"
#include "executor/admin/UpdateUserExecutor.h"
#include "executor/logic/LoopExecutor.h"
#include "executor/logic/MultiOutputsExecutor.h"
#include "executor/logic/SelectExecutor.h"
#include "executor/logic/StartExecutor.h"
#include "executor/maintain/EdgeExecutor.h"
#include "executor/maintain/TagExecutor.h"
#include "executor/mutate/DeleteExecutor.h"
#include "executor/mutate/InsertExecutor.h"
#include "executor/mutate/UpdateExecutor.h"
#include "executor/query/AggregateExecutor.h"
#include "executor/query/DataCollectExecutor.h"
#include "executor/query/DataJoinExecutor.h"
#include "executor/query/DedupExecutor.h"
#include "executor/query/FilterExecutor.h"
#include "executor/query/GetEdgesExecutor.h"
#include "executor/query/GetNeighborsExecutor.h"
#include "executor/query/GetVerticesExecutor.h"
#include "executor/query/IndexScanExecutor.h"
#include "executor/query/IntersectExecutor.h"
#include "executor/query/LimitExecutor.h"
#include "executor/query/MinusExecutor.h"
#include "executor/query/ProjectExecutor.h"
#include "executor/query/SortExecutor.h"
#include "executor/query/UnionExecutor.h"
#include "planner/Admin.h"
#include "planner/Logic.h"
#include "planner/Maintain.h"
#include "planner/Mutate.h"
#include "planner/PlanNode.h"
#include "planner/Query.h"
#include "util/ObjectPool.h"
#include "util/ScopedTimer.h"

using folly::stringPrintf;

namespace nebula {
namespace graph {

// static
Executor *Executor::makeExecutor(const PlanNode *node, QueryContext *qctx) {
    std::unordered_map<int64_t, Executor *> visited;
    return makeExecutor(node, qctx, &visited);
}

// static
Executor *Executor::makeExecutor(const PlanNode *node,
                                 QueryContext *qctx,
                                 std::unordered_map<int64_t, Executor *> *visited) {
    DCHECK(qctx != nullptr);
    DCHECK(node != nullptr);
    auto iter = visited->find(node->id());
    if (iter != visited->end()) {
        return iter->second;
    }

    Executor *exec = nullptr;

    switch (node->kind()) {
        case PlanNode::Kind::kMultiOutputs: {
            auto mout = asNode<MultiOutputsNode>(node);
            auto dep = makeExecutor(mout->dep(), qctx, visited);
            exec = new MultiOutputsExecutor(mout, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kAggregate: {
            auto agg = asNode<Aggregate>(node);
            auto dep = makeExecutor(agg->dep(), qctx, visited);
            exec = new AggregateExecutor(agg, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kSort: {
            auto sort = asNode<Sort>(node);
            auto dep = makeExecutor(sort->dep(), qctx, visited);
            exec = new SortExecutor(sort, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kFilter: {
            auto filter = asNode<Filter>(node);
            auto dep = makeExecutor(filter->dep(), qctx, visited);
            exec = new FilterExecutor(filter, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kGetEdges: {
            auto ge = asNode<GetEdges>(node);
            auto dep = makeExecutor(ge->dep(), qctx, visited);
            exec = new GetEdgesExecutor(ge, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kGetVertices: {
            auto gv = asNode<GetVertices>(node);
            auto dep = makeExecutor(gv->dep(), qctx, visited);
            exec = new GetVerticesExecutor(gv, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kGetNeighbors: {
            auto gn = asNode<GetNeighbors>(node);
            auto dep = makeExecutor(gn->dep(), qctx, visited);
            exec = new GetNeighborsExecutor(gn, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kLimit: {
            auto limit = asNode<Limit>(node);
            auto dep = makeExecutor(limit->dep(), qctx, visited);
            exec = new LimitExecutor(limit, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kProject: {
            auto project = asNode<Project>(node);
            auto dep = makeExecutor(project->dep(), qctx, visited);
            exec = new ProjectExecutor(project, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kIndexScan: {
            auto indexScan = asNode<IndexScan>(node);
            auto dep = makeExecutor(indexScan->dep(), qctx, visited);
            exec = new IndexScanExecutor(indexScan, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kStart: {
            exec = new StartExecutor(node, qctx);
            break;
        }
        case PlanNode::Kind::kUnion: {
            auto uni = asNode<Union>(node);
            auto left = makeExecutor(uni->left(), qctx, visited);
            auto right = makeExecutor(uni->right(), qctx, visited);
            exec = new UnionExecutor(uni, qctx);
            exec->dependsOn(left)->dependsOn(right);
            break;
        }
        case PlanNode::Kind::kIntersect: {
            auto intersect = asNode<Intersect>(node);
            auto left = makeExecutor(intersect->left(), qctx, visited);
            auto right = makeExecutor(intersect->right(), qctx, visited);
            exec = new IntersectExecutor(intersect, qctx);
            exec->dependsOn(left)->dependsOn(right);
            break;
        }
        case PlanNode::Kind::kMinus: {
            auto minus = asNode<Minus>(node);
            auto left = makeExecutor(minus->left(), qctx, visited);
            auto right = makeExecutor(minus->right(), qctx, visited);
            exec = new MinusExecutor(minus, qctx);
            exec->dependsOn(left)->dependsOn(right);
            break;
        }
        case PlanNode::Kind::kLoop: {
            auto loop = asNode<Loop>(node);
            auto dep = makeExecutor(loop->dep(), qctx, visited);
            auto body = makeExecutor(loop->body(), qctx, visited);
            exec = new LoopExecutor(loop, qctx, body);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kSelect: {
            auto select = asNode<Select>(node);
            auto dep = makeExecutor(select->dep(), qctx, visited);
            auto then = makeExecutor(select->then(), qctx, visited);
            auto els = makeExecutor(select->otherwise(), qctx, visited);
            exec = new SelectExecutor(select, qctx, then, els);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kDedup: {
            auto dedup = asNode<Dedup>(node);
            auto dep = makeExecutor(dedup->dep(), qctx, visited);
            exec = new DedupExecutor(dedup, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kSwitchSpace: {
            auto switchSpace = asNode<SwitchSpace>(node);
            auto dep = makeExecutor(switchSpace->dep(), qctx, visited);
            exec = new SwitchSpaceExecutor(switchSpace, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kCreateSpace: {
            auto createSpace = asNode<CreateSpace>(node);
            auto dep = makeExecutor(createSpace->dep(), qctx, visited);
            exec = new CreateSpaceExecutor(createSpace, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kDescSpace: {
            auto descSpace = asNode<DescSpace>(node);
            auto dep = makeExecutor(descSpace->dep(), qctx, visited);
            exec = new DescSpaceExecutor(descSpace, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kShowSpaces: {
            auto showSpaces = asNode<ShowSpaces>(node);
            auto input = makeExecutor(showSpaces->dep(), qctx, visited);
            exec = new ShowSpacesExecutor(showSpaces, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kDropSpace: {
            auto dropSpace = asNode<DropSpace>(node);
            auto input = makeExecutor(dropSpace->dep(), qctx, visited);
            exec = new DropSpaceExecutor(dropSpace, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowCreateSpace: {
            auto showCreateSpace = asNode<ShowCreateSpace>(node);
            auto input = makeExecutor(showCreateSpace->dep(), qctx, visited);
            exec = new ShowCreateSpaceExecutor(showCreateSpace, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kCreateTag: {
            auto createTag = asNode<CreateTag>(node);
            auto dep = makeExecutor(createTag->dep(), qctx, visited);
            exec = new CreateTagExecutor(createTag, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kDescTag: {
            auto descTag = asNode<DescTag>(node);
            auto dep = makeExecutor(descTag->dep(), qctx, visited);
            exec = new DescTagExecutor(descTag, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kAlterTag: {
            auto alterTag = asNode<AlterTag>(node);
            auto dep = makeExecutor(alterTag->dep(), qctx, visited);
            exec = new AlterTagExecutor(alterTag, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kCreateEdge: {
            auto createEdge = asNode<CreateEdge>(node);
            auto dep = makeExecutor(createEdge->dep(), qctx, visited);
            exec = new CreateEdgeExecutor(createEdge, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kDescEdge: {
            auto descEdge = asNode<DescEdge>(node);
            auto dep = makeExecutor(descEdge->dep(), qctx, visited);
            exec = new DescEdgeExecutor(descEdge, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kAlterEdge: {
            auto alterEdge = asNode<AlterEdge>(node);
            auto dep = makeExecutor(alterEdge->dep(), qctx, visited);
            exec = new AlterEdgeExecutor(alterEdge, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kShowTags: {
            auto showTags = asNode<ShowTags>(node);
            auto input = makeExecutor(showTags->dep(), qctx, visited);
            exec = new ShowTagsExecutor(showTags, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowEdges: {
            auto showEdges = asNode<ShowEdges>(node);
            auto input = makeExecutor(showEdges->dep(), qctx, visited);
            exec = new ShowEdgesExecutor(showEdges, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kDropTag: {
            auto dropTag = asNode<DropTag>(node);
            auto input = makeExecutor(dropTag->dep(), qctx, visited);
            exec = new DropTagExecutor(dropTag, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kDropEdge: {
            auto dropEdge = asNode<DropEdge>(node);
            auto input = makeExecutor(dropEdge->dep(), qctx, visited);
            exec = new DropEdgeExecutor(dropEdge, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowCreateTag: {
            auto showCreateTag = asNode<ShowCreateTag>(node);
            auto input = makeExecutor(showCreateTag->dep(), qctx, visited);
            exec = new ShowCreateTagExecutor(showCreateTag, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowCreateEdge: {
            auto showCreateEdge = asNode<ShowCreateEdge>(node);
            auto input = makeExecutor(showCreateEdge->dep(), qctx, visited);
            exec = new ShowCreateEdgeExecutor(showCreateEdge, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kInsertVertices: {
            auto insertV = asNode<InsertVertices>(node);
            auto dep = makeExecutor(insertV->dep(), qctx, visited);
            exec = new InsertVerticesExecutor(insertV, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kInsertEdges: {
            auto insertE = asNode<InsertEdges>(node);
            auto dep = makeExecutor(insertE->dep(), qctx, visited);
            exec = new InsertEdgesExecutor(insertE, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kDataCollect: {
            auto dc = asNode<DataCollect>(node);
            auto dep = makeExecutor(dc->dep(), qctx, visited);
            exec = new DataCollectExecutor(dc, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kCreateSnapshot: {
            auto createSnapshot = asNode<CreateSnapshot>(node);
            auto input = makeExecutor(createSnapshot->dep(), qctx, visited);
            exec = new CreateSnapshotExecutor(createSnapshot, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kDropSnapshot: {
            auto dropSnapshot = asNode<DropSnapshot>(node);
            auto input = makeExecutor(dropSnapshot->dep(), qctx, visited);
            exec = new DropSnapshotExecutor(dropSnapshot, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowSnapshots: {
            auto showSnapshots = asNode<ShowSnapshots>(node);
            auto input = makeExecutor(showSnapshots->dep(), qctx, visited);
            exec = new ShowSnapshotsExecutor(showSnapshots, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kDataJoin: {
            auto dataJoin = asNode<DataJoin>(node);
            auto input = makeExecutor(dataJoin->dep(), qctx, visited);
            exec = new DataJoinExecutor(dataJoin, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kDeleteVertices: {
            auto deleteV = asNode<DeleteVertices>(node);
            auto input = makeExecutor(deleteV->dep(), qctx, visited);
            exec = new DeleteVerticesExecutor(deleteV, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kDeleteEdges: {
            auto deleteE = asNode<DeleteEdges>(node);
            auto input = makeExecutor(deleteE->dep(), qctx, visited);
            exec = new DeleteEdgesExecutor(deleteE, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kUpdateVertex: {
            auto updateV = asNode<UpdateVertex>(node);
            auto input = makeExecutor(updateV->dep(), qctx, visited);
            exec = new UpdateVertexExecutor(updateV, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kUpdateEdge: {
            auto updateE = asNode<UpdateEdge>(node);
            auto input = makeExecutor(updateE->dep(), qctx, visited);
            exec = new UpdateEdgeExecutor(updateE, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kCreateUser: {
            auto createUser = asNode<CreateUser>(node);
            auto input = makeExecutor(createUser->dep(), qctx, visited);
            exec = new CreateUserExecutor(createUser, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kDropUser: {
            auto dropUser = asNode<DropUser>(node);
            auto input = makeExecutor(dropUser->dep(), qctx, visited);
            exec = new DropUserExecutor(dropUser, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kUpdateUser: {
            auto updateUser = asNode<UpdateUser>(node);
            auto input = makeExecutor(updateUser->dep(), qctx, visited);
            exec = new UpdateUserExecutor(updateUser, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kGrantRole: {
            auto grantRole = asNode<GrantRole>(node);
            auto input = makeExecutor(grantRole->dep(), qctx, visited);
            exec = new GrantRoleExecutor(grantRole, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kRevokeRole: {
            auto revokeRole = asNode<RevokeRole>(node);
            auto input = makeExecutor(revokeRole->dep(), qctx, visited);
            exec = new RevokeRoleExecutor(revokeRole, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kChangePassword: {
            auto changePassword = asNode<ChangePassword>(node);
            auto input = makeExecutor(changePassword->dep(), qctx, visited);
            exec = new ChangePasswordExecutor(changePassword, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kListUserRoles: {
            auto listUserRoles = asNode<ListUserRoles>(node);
            auto input = makeExecutor(listUserRoles->dep(), qctx, visited);
            exec = new ListUserRolesExecutor(listUserRoles, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kListUsers: {
            auto listUsers = asNode<ListUsers>(node);
            auto input = makeExecutor(listUsers->dep(), qctx, visited);
            exec = new ListUsersExecutor(listUsers, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kListRoles: {
            auto listRoles = asNode<ListRoles>(node);
            auto input = makeExecutor(listRoles->dep(), qctx, visited);
            exec = new ListRolesExecutor(listRoles, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kBalanceLeaders: {
            auto balanceLeaders = asNode<BalanceLeaders>(node);
            auto dep = makeExecutor(balanceLeaders->dep(), qctx, visited);
            exec = new BalanceLeadersExecutor(balanceLeaders, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kBalance: {
            auto balance = asNode<Balance>(node);
            auto dep = makeExecutor(balance->dep(), qctx, visited);
            exec = new BalanceExecutor(balance, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kStopBalance: {
            auto stopBalance = asNode<Balance>(node);
            auto dep = makeExecutor(stopBalance->dep(), qctx, visited);
            exec = new StopBalanceExecutor(stopBalance, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kShowBalance: {
            auto showBalance = asNode<ShowBalance>(node);
            auto dep = makeExecutor(showBalance->dep(), qctx, visited);
            exec = new ShowBalanceExecutor(showBalance, qctx);
            exec->dependsOn(dep);
            break;
        }
        case PlanNode::Kind::kShowConfigs: {
            auto showConfigs = asNode<ShowConfigs>(node);
            auto input = makeExecutor(showConfigs->dep(), qctx, visited);
            exec = new ShowConfigsExecutor(showConfigs, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kSetConfig: {
            auto setConfig = asNode<SetConfig>(node);
            auto input = makeExecutor(setConfig->dep(), qctx, visited);
            exec = new SetConfigExecutor(setConfig, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kGetConfig: {
            auto getConfig = asNode<GetConfig>(node);
            auto input = makeExecutor(getConfig->dep(), qctx, visited);
            exec = new GetConfigExecutor(getConfig, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kSubmitJob: {
            auto submitJob = asNode<SubmitJob>(node);
            auto input = makeExecutor(submitJob->dep(), qctx, visited);
            exec = new SubmitJobExecutor(submitJob, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowHosts: {
            auto showHosts = asNode<ShowHosts>(node);
            auto input = makeExecutor(showHosts->dep(), qctx, visited);
            exec = new ShowHostsExecutor(showHosts, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowParts: {
            auto showParts = asNode<ShowParts>(node);
            auto input = makeExecutor(showParts->dep(), qctx, visited);
            exec = new ShowPartsExecutor(showParts, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowCharset: {
            auto showC = asNode<ShowCharset>(node);
            auto input = makeExecutor(showC->dep(), qctx, visited);
            exec = new ShowCharsetExecutor(showC, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kShowCollation: {
            auto showC = asNode<ShowCollation>(node);
            auto input = makeExecutor(showC->dep(), qctx, visited);
            exec = new ShowCollationExecutor(showC, qctx);
            exec->dependsOn(input);
            break;
        }
        case PlanNode::Kind::kUnknown:
        default:
            LOG(FATAL) << "Unknown plan node kind " << static_cast<int32_t>(node->kind());
            break;
    }

    DCHECK(!!exec);

    visited->insert({node->id(), exec});
    return qctx->objPool()->add(exec);
}

Executor::Executor(const std::string &name, const PlanNode *node, QueryContext *qctx)
    : id_(DCHECK_NOTNULL(node)->id()),
      name_(name),
      node_(DCHECK_NOTNULL(node)),
      qctx_(DCHECK_NOTNULL(qctx)),
      ectx_(DCHECK_NOTNULL(qctx->ectx())) {
    // Initialize the position in ExecutionContext for each executor before execution plan
    // starting to run. This will avoid lock something for thread safety in real execution
    if (!ectx_->exist(node->varName())) {
        ectx_->initVar(node->varName());
    }
}

Executor::~Executor() {}

void Executor::startProfiling() {
    numRows_ = 0;
    execTime_ = 0;
    totalDuration_.reset();
}

void Executor::stopProfiling() {
    cpp2::ProfilingStats stats;
    stats.set_total_duration_in_us(totalDuration_.elapsedInUSec());
    stats.set_rows(numRows_);
    stats.set_exec_duration_in_us(execTime_);
    qctx()->addProfilingData(node_->id(), std::move(stats));
}

folly::Future<Status> Executor::start(Status status) const {
    return folly::makeFuture(std::move(status)).via(runner());
}

folly::Future<Status> Executor::error(Status status) const {
    return folly::makeFuture<Status>(ExecutionError(std::move(status))).via(runner());
}

Status Executor::finish(Result &&result) {
    numRows_ = result.size();
    ectx_->setResult(node()->varName(), std::move(result));
    return Status::OK();
}

Status Executor::finish(Value &&value) {
    return finish(ResultBuilder().value(std::move(value)).iter(Iterator::Kind::kDefault).finish());
}

folly::Executor *Executor::runner() const {
    if (!qctx() || !qctx()->rctx() || !qctx()->rctx()->runner()) {
        // This is just for test
        return &folly::InlineExecutor::instance();
    }
    return qctx()->rctx()->runner();
}

}   // namespace graph
}   // namespace nebula
