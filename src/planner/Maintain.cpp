/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "planner/Maintain.h"

#include <sstream>

#include "common/interface/gen-cpp2/graph_types.h"
#include "util/ToJson.h"

namespace nebula {
namespace graph {

std::unique_ptr<cpp2::PlanNodeDescription> CreateSchemaNode::explain() const {
    auto desc = SingleInputNode::explain();
    addDescription("name", name_, desc.get());
    addDescription("ifNotExists", folly::to<std::string>(ifNotExists_), desc.get());
    addDescription("schema", folly::toJson(util::toJson(schema_)), desc.get());
    return desc;
}

std::unique_ptr<cpp2::PlanNodeDescription> AlterSchemaNode::explain() const {
    auto desc = SingleInputNode::explain();
    addDescription("space", folly::to<std::string>(space_), desc.get());
    addDescription("name", name_, desc.get());
    addDescription("schemaItems", folly::toJson(util::toJson(schemaItems_)), desc.get());
    addDescription("schemaProp", folly::toJson(util::toJson(schemaProp_)), desc.get());
    return desc;
}

std::unique_ptr<cpp2::PlanNodeDescription> DescSchema::explain() const {
    auto desc = SingleInputNode::explain();
    addDescription("name", name_, desc.get());
    return desc;
}

std::unique_ptr<cpp2::PlanNodeDescription> DropSchema::explain() const {
    auto desc = SingleInputNode::explain();
    addDescription("name", name_, desc.get());
    addDescription("ifExists", util::toJson(ifExists_), desc.get());
    return desc;
}

}   // namespace graph
}   // namespace nebula
