#include <print>

import core;
import storage;

auto main() -> int
{
    storage::s_node_metadata node_id(storage::s_node_metadata::e_node_kind::property, "my_db", { .item_type = "my_item_type", .item_id = "my_item_id", .relationship_type = "my_property_name" });
    std::println("Node ID: {}", node_id);
}
