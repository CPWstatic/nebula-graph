/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef PLANNER_MUTATE_H_
#define PLANNER_MUTATE_H_

#include "common/interface/gen-cpp2/storage_types.h"

#include "planner/Query.h"
#include "parser/TraverseSentences.h"

/**
 * All mutate-related nodes would put in this file.
 */
namespace nebula {
namespace graph {
class InsertVertices final : public SingleInputNode {
public:
    static InsertVertices* make(
            ExecutionPlan* plan,
            PlanNode* input,
            GraphSpaceID spaceId,
            std::vector<storage::cpp2::NewVertex> vertices,
            std::unordered_map<TagID, std::vector<std::string>> tagPropNames,
            bool overwritable) {
        return new InsertVertices(plan,
                                  input,
                                  spaceId,
                                  std::move(vertices),
                                  std::move(tagPropNames),
                                  overwritable);
    }

    std::unique_ptr<cpp2::PlanNodeDescription> explain() const override;

    const std::vector<storage::cpp2::NewVertex>& getVertices() const {
        return vertices_;
    }

    const std::unordered_map<TagID, std::vector<std::string>>& getPropNames() const {
        return tagPropNames_;
    }

    bool getOverwritable() const {
        return overwritable_;
    }

    GraphSpaceID getSpace() const {
        return spaceId_;
    }

private:
    InsertVertices(ExecutionPlan* plan,
                   PlanNode* input,
                   GraphSpaceID spaceId,
                   std::vector<storage::cpp2::NewVertex> vertices,
                   std::unordered_map<TagID, std::vector<std::string>> tagPropNames,
                   bool overwritable)
        : SingleInputNode(plan, Kind::kInsertVertices, input)
        , spaceId_(spaceId)
        , vertices_(std::move(vertices))
        , tagPropNames_(std::move(tagPropNames))
        , overwritable_(overwritable) {
    }

private:
    GraphSpaceID                                               spaceId_{-1};
    std::vector<storage::cpp2::NewVertex>                      vertices_;
    std::unordered_map<TagID, std::vector<std::string>>        tagPropNames_;
    bool                                                       overwritable_;
};

class InsertEdges final : public SingleInputNode {
public:
    static InsertEdges* make(ExecutionPlan* plan,
                             PlanNode* input,
                             GraphSpaceID spaceId,
                             std::vector<storage::cpp2::NewEdge> edges,
                             std::vector<std::string> propNames,
                             bool overwritable) {
        return new InsertEdges(plan,
                               input,
                               spaceId,
                               std::move(edges),
                               std::move(propNames),
                               overwritable);
    }

    std::unique_ptr<cpp2::PlanNodeDescription> explain() const override;

    const std::vector<std::string>& getPropNames() const {
        return propNames_;
    }

    const std::vector<storage::cpp2::NewEdge>& getEdges() const {
        return edges_;
    }

    bool getOverwritable() const {
        return overwritable_;
    }

    GraphSpaceID getSpace() const {
        return spaceId_;
    }

private:
    InsertEdges(ExecutionPlan* plan,
                PlanNode* input,
                GraphSpaceID spaceId,
                std::vector<storage::cpp2::NewEdge> edges,
                std::vector<std::string> propNames,
                bool overwritable)
        : SingleInputNode(plan, Kind::kInsertEdges, input)
        , spaceId_(spaceId)
        , edges_(std::move(edges))
        , propNames_(std::move(propNames))
        , overwritable_(overwritable) {
    }

private:
    GraphSpaceID                               spaceId_{-1};
    std::vector<storage::cpp2::NewEdge>        edges_;
    std::vector<std::string>                   propNames_;
    bool                                       overwritable_;
};

class Update : public SingleInputNode {
public:
    bool getInsertable() const {
        return insertable_;
    }

    const std::vector<std::string>& getReturnProps() const {
        return returnProps_;
    }

    const std::string getCondition() const {
        return condition_;
    }

    const std::vector<std::string>& getYieldNames() const {
        return yieldNames_;
    }

    GraphSpaceID getSpaceId() const {
        return spaceId_;
    }

    const std::vector<storage::cpp2::UpdatedProp>& getUpdatedProps() const {
        return updatedProps_;
    }

    const std::string& getName() const {
        return schemaName_;
    }

    std::unique_ptr<cpp2::PlanNodeDescription> explain() const override;

protected:
    Update(Kind kind,
           ExecutionPlan* plan,
           PlanNode* input,
           GraphSpaceID spaceId,
           std::string name,
           bool insertable,
           std::vector<storage::cpp2::UpdatedProp> updatedProps,
           std::vector<std::string> returnProps,
           std::string condition,
           std::vector<std::string> yieldNames)
        : SingleInputNode(plan, kind, input)
        , spaceId_(spaceId)
        , schemaName_(std::move(name))
        , insertable_(insertable)
        , updatedProps_(std::move(updatedProps))
        , returnProps_(std::move(returnProps))
        , condition_(std::move(condition))
        , yieldNames_(std::move(yieldNames)) {}

protected:
    GraphSpaceID                                        spaceId_{-1};
    std::string                                         schemaName_;
    bool                                                insertable_;
    std::vector<storage::cpp2::UpdatedProp>             updatedProps_;
    std::vector<std::string>                            returnProps_;
    std::string                                         condition_;
    std::vector<std::string>                            yieldNames_;
};

class UpdateVertex final : public Update {
public:
    static UpdateVertex* make(ExecutionPlan* plan,
                              PlanNode* input,
                              GraphSpaceID spaceId,
                              std::string name,
                              std::string vId,
                              TagID tagId,
                              bool insertable,
                              std::vector<storage::cpp2::UpdatedProp> updatedProps,
                              std::vector<std::string> returnProps,
                              std::string condition,
                              std::vector<std::string> yieldNames) {
        return new UpdateVertex(plan,
                                input,
                                spaceId,
                                std::move(name),
                                std::move(vId),
                                tagId,
                                insertable,
                                std::move(updatedProps),
                                std::move(returnProps),
                                std::move(condition),
                                std::move(yieldNames));
    }

    std::unique_ptr<cpp2::PlanNodeDescription> explain() const override;

    const std::string& getVId() const {
        return vId_;
    }

    TagID getTagId() const {
        return tagId_;
    }

private:
    UpdateVertex(ExecutionPlan* plan,
                 PlanNode* input,
                 GraphSpaceID spaceId,
                 std::string name,
                 std::string vId,
                 TagID tagId,
                 bool insertable,
                 std::vector<storage::cpp2::UpdatedProp> updatedProps,
                 std::vector<std::string> returnProps,
                 std::string condition,
                 std::vector<std::string> yieldNames)
        : Update(Kind::kUpdateVertex,
                 plan,
                 input,
                 spaceId,
                 std::move(name),
                 insertable,
                 std::move(updatedProps),
                 std::move(returnProps),
                 std::move(condition),
                 std::move(yieldNames))
        , vId_(std::move(vId))
        , tagId_(tagId) {}

private:
    std::string                                     vId_;
    TagID                                           tagId_{-1};
};

class UpdateEdge final : public Update {
public:
    static UpdateEdge* make(ExecutionPlan* plan,
                            PlanNode* input,
                            GraphSpaceID spaceId,
                            std::string name,
                            std::string srcId,
                            std::string dstId,
                            EdgeType edgeType,
                            int64_t rank,
                            bool insertable,
                            std::vector<storage::cpp2::UpdatedProp> updatedProps,
                            std::vector<std::string> returnProps,
                            std::string condition,
                            std::vector<std::string> yieldNames) {
        return new UpdateEdge(plan,
                              input,
                              spaceId,
                              std::move(name),
                              std::move(srcId),
                              std::move(dstId),
                              edgeType,
                              rank,
                              insertable,
                              std::move(updatedProps),
                              std::move(returnProps),
                              std::move(condition),
                              std::move(yieldNames));
    }

    std::unique_ptr<cpp2::PlanNodeDescription> explain() const override;

    const std::string& getSrcId() const {
        return srcId_;
    }

    const std::string& getDstId() const {
        return dstId_;
    }

    int64_t getRank() const {
        return rank_;
    }

    int64_t getEdgeType() const {
        return edgeType_;
    }

    const std::vector<storage::cpp2::UpdatedProp>& getUpdatedProps() const {
        return updatedProps_;
    }

private:
    UpdateEdge(ExecutionPlan* plan,
               PlanNode* input,
               GraphSpaceID spaceId,
               std::string name,
               std::string srcId,
               std::string dstId,
               EdgeType edgeType,
               int64_t rank,
               bool insertable,
               std::vector<storage::cpp2::UpdatedProp> updatedProps,
               std::vector<std::string> returnProps,
               std::string condition,
               std::vector<std::string> yieldNames)
        : Update(Kind::kUpdateEdge,
                 plan,
                 input,
                 spaceId,
                 std::move(name),
                 insertable,
                 std::move(updatedProps),
                 std::move(returnProps),
                 std::move(condition),
                 std::move(yieldNames))

        , srcId_(std::move(srcId))
        , dstId_(std::move(dstId))
        , rank_(rank)
        , edgeType_(edgeType) {}

private:
    std::string                                         srcId_;
    std::string                                         dstId_;
    int64_t                                             rank_{0};
    EdgeType                                            edgeType_{-1};
};

class DeleteVertices final : public SingleInputNode {
public:
    static DeleteVertices* make(ExecutionPlan* plan,
                                PlanNode* input,
                                GraphSpaceID spaceId,
                                Expression* vidRef_) {
        return new DeleteVertices(plan,
                                  input,
                                  spaceId,
                                  vidRef_);
    }

    std::unique_ptr<cpp2::PlanNodeDescription> explain() const override;

    GraphSpaceID getSpace() const {
        return space_;
    }

    Expression* getVidRef() const {
        return vidRef_;
    }

private:
    DeleteVertices(ExecutionPlan* plan,
                   PlanNode* input,
                   GraphSpaceID spaceId,
                   Expression* vidRef)
        : SingleInputNode(plan, Kind::kDeleteVertices, input)
        , space_(spaceId)
        , vidRef_(vidRef) {}

private:
    GraphSpaceID                            space_;
    Expression                             *vidRef_{nullptr};
};

class DeleteEdges final : public SingleInputNode {
public:
    static DeleteEdges* make(ExecutionPlan* plan,
                             PlanNode* input,
                             GraphSpaceID spaceId,
                             std::vector<EdgeKeyRef*> edgeKeyRefs) {
        return new DeleteEdges(plan,
                               input,
                               spaceId,
                               std::move(edgeKeyRefs));
    }

    std::unique_ptr<cpp2::PlanNodeDescription> explain() const override;

    GraphSpaceID getSpace() const {
        return space_;
    }

    const std::vector<EdgeKeyRef*>& getEdgeKeyRefs() const {
        return edgeKeyRefs_;
    }

private:
    DeleteEdges(ExecutionPlan* plan,
                PlanNode* input,
                GraphSpaceID spaceId,
                std::vector<EdgeKeyRef*> edgeKeyRefs)
        : SingleInputNode(plan, Kind::kDeleteEdges, input)
        , space_(spaceId)
        , edgeKeyRefs_(std::move(edgeKeyRefs)) {}

private:
    GraphSpaceID space_{-1};
    std::vector<EdgeKeyRef*> edgeKeyRefs_;
};

}  // namespace graph
}  // namespace nebula
#endif  // PLANNER_MUTATE_H_
