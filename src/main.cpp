#include <print>

import core;
import storage;

int main()
{
    storage::s_node_id node_id{ .kind = storage::s_node_id::e_node_kind::item, .database = "mydb", .payload = storage::s_node_id::s_item_payload{ .item_type = "book", .item_id = "123" } };
    std::println("Node ID: {}", node_id);
}
