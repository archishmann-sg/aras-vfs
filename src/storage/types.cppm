module;
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <variant>
export module storage:types;

import core;

// VFS interface contracts
export namespace storage::inline type
{
    // Exported strong type tags
    namespace tag
    {
        struct modified_time;
    } // namespace tag

    // Exported strong types
    namespace types
    {
        using modified_time_t = core::c_strong_type<std::uint64_t, tag::modified_time>;
    } // namespace types

    namespace err_code
    {
        // TODO: Define error codes for interface.

    } // namespace err_code

    struct s_node_id
    {
        enum class e_node_kind : std::uint8_t
        {
            root,              // Root of VFS, as a mount point
            database,          // A database instance of a server. Unique for (server,db) pair
            item_root,         // Virtual root node to start item hierarchy. Itemtypes in this dir
            item_type,         // A specific itemtype. Used for item search for a type
            item,              // An item instance. Unique for (server,db,item_id) triplet
            property_dir,      // Virtual directory under item to hold properties. Not a real node
            property,          // A property of an item. Unique for (server,db,item_id,property_name) quadruple
            relationships_dir, // Virtual directory under item to hold relationships. Not a real node
            relationship_type, // A specific relationship type. Used for relationship search for a type under an item
            relationship,      // A relationship instance. Unique for (server,db,related_item_id) triplet
        };

        struct s_item_type_payload
        {
            std::string item_type;
        };

        struct s_item_payload
        {
            std::string item_type;
            std::string item_id;
        };

        struct s_property_payload
        {
            std::string item_type;
            std::string item_id;
            std::string property_name;
        };

        struct s_relationship_payload
        {
            std::string item_type;
            std::string item_id;
            std::string relationship_type;
            std::string relationship_id;
        };

        e_node_kind kind;

        std::string database;

        std::variant<std::monostate, s_item_type_payload, s_item_payload, s_property_payload, s_relationship_payload> payload;

        auto operator==(const s_node_id &other) const -> bool
        {
            if (kind != other.kind)
            {
                return false;
            }

            switch (kind)
            {
            case e_node_kind::root:
                return true;
            case e_node_kind::database:
            case e_node_kind::item_root:
                return database == other.database;
            case e_node_kind::item_type:
                return database == other.database
                       && std::get<s_item_type_payload>(payload).item_type == std::get<s_item_type_payload>(other.payload).item_type;
            case e_node_kind::item:
            case e_node_kind::property_dir:
                return database == other.database
                       && std::get<s_item_payload>(payload).item_type == std::get<s_item_payload>(other.payload).item_type
                       && std::get<s_item_payload>(payload).item_id == std::get<s_item_payload>(other.payload).item_id;
            case e_node_kind::property:
                return database == other.database
                       && std::get<s_property_payload>(payload).item_type == std::get<s_property_payload>(other.payload).item_type
                       && std::get<s_property_payload>(payload).item_id == std::get<s_property_payload>(other.payload).item_id
                       && std::get<s_property_payload>(payload).property_name == std::get<s_property_payload>(other.payload).property_name;
            case e_node_kind::relationships_dir:
                return database == other.database
                       && std::get<s_item_payload>(payload).item_type == std::get<s_item_payload>(other.payload).item_type
                       && std::get<s_item_payload>(payload).item_id == std::get<s_item_payload>(other.payload).item_id;
            case e_node_kind::relationship_type:
                return database == other.database
                       && std::get<s_relationship_payload>(payload).item_type == std::get<s_relationship_payload>(other.payload).item_type
                       && std::get<s_relationship_payload>(payload).item_id == std::get<s_relationship_payload>(other.payload).item_id
                       && std::get<s_relationship_payload>(payload).relationship_type == std::get<s_relationship_payload>(other.payload).relationship_type;
            case e_node_kind::relationship:
                return database == other.database
                       && std::get<s_relationship_payload>(payload).item_type == std::get<s_relationship_payload>(other.payload).item_type
                       && std::get<s_relationship_payload>(payload).item_id == std::get<s_relationship_payload>(other.payload).item_id
                       && std::get<s_relationship_payload>(payload).relationship_type == std::get<s_relationship_payload>(other.payload).relationship_type
                       && std::get<s_relationship_payload>(payload).relationship_id == std::get<s_relationship_payload>(other.payload).relationship_id;
            }
        }
    };

    namespace types
    {
        using node_id_t = s_node_id;
    } // namespace types
} // namespace storage::inline type

export namespace std
{
    template <>
    struct hash<storage::types::node_id_t>
    {
        auto operator()(const storage::types::node_id_t &node_id) const noexcept -> std::size_t
        {
            size_t hash = std::hash<int>{}(int(node_id.kind));

            auto mix = [&](const std::string &str) -> void
            {
                hash ^= std::hash<std::string>{}(str) + 0x9e3779b9 + (hash << 6U) + (hash >> 2U);
            };

            switch (node_id.kind)
            {
            case storage::s_node_id::e_node_kind::root:
                break;
            case storage::s_node_id::e_node_kind::database:
            case storage::s_node_id::e_node_kind::item_root:
                mix(node_id.database);
                break;
            case storage::s_node_id::e_node_kind::item_type:
                mix(node_id.database);
                mix(std::get<storage::s_node_id::s_item_type_payload>(node_id.payload).item_type);
                break;
            case storage::s_node_id::e_node_kind::item:
            case storage::s_node_id::e_node_kind::property_dir:
                mix(node_id.database);
                mix(std::get<storage::s_node_id::s_item_payload>(node_id.payload).item_type);
                mix(std::get<storage::s_node_id::s_item_payload>(node_id.payload).item_id);
                break;
            case storage::s_node_id::e_node_kind::property:
                mix(node_id.database);
                mix(std::get<storage::s_node_id::s_property_payload>(node_id.payload).item_type);
                mix(std::get<storage::s_node_id::s_property_payload>(node_id.payload).item_id);
                mix(std::get<storage::s_node_id::s_property_payload>(node_id.payload).property_name);
                break;
            case storage::s_node_id::e_node_kind::relationships_dir:
                mix(node_id.database);
                mix(std::get<storage::s_node_id::s_item_payload>(node_id.payload).item_type);
                mix(std::get<storage::s_node_id::s_item_payload>(node_id.payload).item_id);
                break;
            case storage::s_node_id::e_node_kind::relationship_type:
                mix(node_id.database);
                mix(std::get<storage::s_node_id::s_relationship_payload>(node_id.payload).item_type);
                mix(std::get<storage::s_node_id::s_relationship_payload>(node_id.payload).item_id);
                mix(std::get<storage::s_node_id::s_relationship_payload>(node_id.payload).relationship_type);
                break;
            case storage::s_node_id::e_node_kind::relationship:
                mix(node_id.database);
                mix(std::get<storage::s_node_id::s_relationship_payload>(node_id.payload).item_type);
                mix(std::get<storage::s_node_id::s_relationship_payload>(node_id.payload).item_id);
                mix(std::get<storage::s_node_id::s_relationship_payload>(node_id.payload).relationship_type);
                mix(std::get<storage::s_node_id::s_relationship_payload>(node_id.payload).relationship_id);
                break;
            }

            return hash;
        }
    };

    template <>
    struct formatter<storage::types::node_id_t> : std::formatter<std::string>
    {
        constexpr auto parse(auto &ctx)
        {
            return ctx.begin();
        }

        auto format(const storage::types::node_id_t &node_id, auto &ctx) const
        {
            auto out = ctx.out();
            switch (node_id.kind)
            {
            case storage::s_node_id::e_node_kind::root:
                out = std::format_to(out, "root");
                break;
            case storage::s_node_id::e_node_kind::database:
                out = std::format_to(out, "database[{}]", node_id.database);
                break;
            case storage::s_node_id::e_node_kind::item_root:
                out = std::format_to(out, "database[{}]/items", node_id.database);
                break;
            case storage::s_node_id::e_node_kind::item_type:
                out = std::format_to(out, "database[{}]/items/[{}]", node_id.database, std::get<storage::s_node_id::s_item_type_payload>(node_id.payload).item_type);
                break;
            case storage::s_node_id::e_node_kind::item:
            {
                const auto &payload = std::get<storage::s_node_id::s_item_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]", node_id.database, payload.item_type, payload.item_id);
            }
            break;
            case storage::s_node_id::e_node_kind::property_dir:
            {
                const auto &payload = std::get<storage::s_node_id::s_item_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/properties", node_id.database, payload.item_type, payload.item_id);
            }
            break;
            case storage::s_node_id::e_node_kind::property:
            {
                const auto &payload = std::get<storage::s_node_id::s_property_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/properties/[{}]", node_id.database, payload.item_type, payload.item_id, payload.property_name);
            }
            break;
            case storage::s_node_id::e_node_kind::relationships_dir:
            {
                const auto &payload = std::get<storage::s_node_id::s_item_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/relationships", node_id.database, payload.item_type, payload.item_id);
            }
            break;
            case storage::s_node_id::e_node_kind::relationship_type:
            {
                const auto &payload = std::get<storage::s_node_id::s_relationship_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/relationships/[{}]", node_id.database, payload.item_type, payload.item_id, payload.relationship_type);
            }
            break;
            case storage::s_node_id::e_node_kind::relationship:
            {
                const auto &payload = std::get<storage::s_node_id::s_relationship_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/relationships/[{}]/[{}]", node_id.database, payload.item_type, payload.item_id, payload.relationship_type, payload.relationship_id);
            }
            break;
            }
            return out;
        }
    };
} // namespace std

export namespace storage::inline type
{
    /**
     * @class s_storage_entry
     * @brief Represents a file or directory in the storage system, encapsulating its metadata and identity.
     *
     */
    struct s_storage_entry
    {
        enum class e_entry_kind : std::uint8_t
        {
            file,
            directory,
            symlink,
        };
        std::string name;
        types::node_id_t id;
        types::node_id_t parent_id;
        std::uint64_t size;
        types::modified_time_t modified_time;
        e_entry_kind kind;
        bool is_read_only;
    };

} // namespace storage::inline type
