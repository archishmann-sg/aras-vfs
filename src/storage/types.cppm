module;
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <variant>
export module storage:types;

import core;

export namespace storage::inline type
{
    // Exported strong type tags
    namespace tag
    {
        struct modified_time;
        struct node_id;
    } // namespace tag

    // Exported strong types
    namespace types
    {
        using modified_time_t = core::c_strong_type<std::uint64_t, tag::modified_time>;
        using node_id_t = core::c_strong_type<std::uint64_t, tag::node_id>;
    } // namespace types

    namespace err_code
    {
        // TODO: Define error codes for interface.

    } // namespace err_code

    struct s_node_metadata
    {
        enum class e_node_kind : std::uint8_t
        {
            root = 1,              // Root of VFS, as a mount point
            database = 2,          // A database instance of a server. Unique for (server,db) pair
            item_root = 3,         // Virtual root node to start item hierarchy. Itemtypes in this dir
            item_type = 4,         // A specific itemtype. Used for item search for a type
            item = 5,              // An item instance. Unique for (server,db,item_id) triplet
            property_dir = 6,      // Virtual directory under item to hold properties. Not a real node
            property = 7,          // A property of an item. Unique for (server,db,item_id,property_name) quadruple
            relationships_dir = 8, // Virtual directory under item to hold relationships. Not a real node
            relationship_type = 9, // A specific relationship type. Used for relationship search for a type under an item
            relationship = 10,     // A relationship instance. Unique for (server,db,relationship_id) triplet
        };

        struct s_item_type_payload
        {
            std::string item_type;
            auto operator==(const s_item_type_payload &other) const -> bool = default;
        };

        struct s_item_payload
        {
            std::string item_type;
            std::string item_id;
            auto operator==(const s_item_payload &other) const -> bool = default;
        };

        struct s_property_payload
        {
            std::string item_type;
            std::string item_id;
            std::string property_name;
            auto operator==(const s_property_payload &other) const -> bool = default;
        };

        struct s_relationship_payload
        {
            std::string item_type;
            std::string item_id;
            std::string relationship_type;
            std::string relationship_id;
            auto operator==(const s_relationship_payload &other) const -> bool = default;
        };

        e_node_kind kind;

        types::node_id_t node_id;
        std::string database;

        std::variant<std::monostate, s_item_type_payload, s_item_payload, s_property_payload, s_relationship_payload> payload;

        auto operator==(const s_node_metadata &other) const -> bool = default;

        [[nodiscard]] auto to_node_id() const -> types::node_id_t;

        s_node_metadata() : kind(e_node_kind::root), payload(std::monostate{})
        {
            node_id = to_node_id();
        }

        s_node_metadata(e_node_kind kind, std::string database) : kind(kind), database(std::move(database)), payload(std::monostate{})
        {
            core::assert(kind == e_node_kind::database or kind == e_node_kind::item_root, "Only database and item_root nodes can be created with this constructor");
            node_id = to_node_id();
        }

        s_node_metadata(e_node_kind kind, std::string database, s_item_type_payload item_type_payload) : kind(kind), database(std::move(database)), payload(std::move(item_type_payload))
        {
            core::assert(kind == e_node_kind::item_type, "Only item_type nodes can be created with this constructor");
            node_id = to_node_id();
        }

        s_node_metadata(e_node_kind kind, std::string database, s_item_payload item_payload) : kind(kind), database(std::move(database)), payload(std::move(item_payload))
        {
            core::assert(kind == e_node_kind::item or kind == e_node_kind::property_dir or kind == e_node_kind::relationships_dir, "Only item, property_dir and relationships_dir nodes can be created with this constructor");
            node_id = to_node_id();
        }

        s_node_metadata(e_node_kind kind, std::string database, s_property_payload property_payload) : kind(kind), database(std::move(database)), payload(std::move(property_payload))
        {
            core::assert(kind == e_node_kind::property, "Only property nodes can be created with this constructor");
            node_id = to_node_id();
        }

        s_node_metadata(e_node_kind kind, std::string database, s_relationship_payload relationship_payload) : kind(kind), database(std::move(database)), payload(std::move(relationship_payload))
        {
            core::assert(kind == e_node_kind::relationship_type or kind == e_node_kind::relationship, "Only relationship_type and relationship nodes can be created with this constructor");
            node_id = to_node_id();
        }
    };
} // namespace storage::inline type

export namespace std
{
    template <>
    struct hash<storage::s_node_metadata>
    {
        static constexpr auto hash_combine(std::uint64_t seed, std::uint64_t val) -> std::uint64_t
        {
            // Murmur3-style finalizer mix
            val ^= val >> 33U;
            val *= 0xff51afd7ed558ccdULL;
            val ^= val >> 33U;
            val *= 0xc4ceb9fe1a85ec53ULL;
            val ^= val >> 33U;
            return seed ^ val;
        }

        static auto hash_str(std::string_view str) -> std::uint64_t
        {
            // FNV-1a over the string bytes
            std::uint64_t hash = 14695981039346656037ULL;
            for (unsigned char ctr : str)
            {
                hash = (hash ^ ctr) * 1099511628211ULL;
            }
            return hash;
        }

        // Implements the FNV-1a hash algorithm over the fields of the node metadata
        auto operator()(const storage::s_node_metadata &node_id) const noexcept -> std::size_t
        {
            auto hash = hash_combine(0U, static_cast<std::uint64_t>(node_id.kind));

            switch (node_id.kind)
            {
            case storage::s_node_metadata::e_node_kind::root:
                break;
            case storage::s_node_metadata::e_node_kind::database:
            case storage::s_node_metadata::e_node_kind::item_root:
                hash = hash_combine(hash, hash_str(node_id.database));
                break;
            case storage::s_node_metadata::e_node_kind::item_type:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_item_type_payload>(node_id.payload);
                hash = hash_combine(hash, hash_str(node_id.database));
                hash = hash_combine(hash, hash_str(payload.item_type));
            }
            break;
            case storage::s_node_metadata::e_node_kind::item:
            case storage::s_node_metadata::e_node_kind::property_dir:
            case storage::s_node_metadata::e_node_kind::relationships_dir:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_item_payload>(node_id.payload);
                hash = hash_combine(hash, hash_str(node_id.database));
                hash = hash_combine(hash, hash_str(payload.item_type));
                hash = hash_combine(hash, hash_str(payload.item_id));
            }
            break;
            case storage::s_node_metadata::e_node_kind::property:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_property_payload>(node_id.payload);
                hash = hash_combine(hash, hash_str(node_id.database));
                hash = hash_combine(hash, hash_str(payload.item_type));
                hash = hash_combine(hash, hash_str(payload.item_id));
                hash = hash_combine(hash, hash_str(payload.property_name));
            }
            break;
            case storage::s_node_metadata::e_node_kind::relationship_type:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_relationship_payload>(node_id.payload);
                hash = hash_combine(hash, hash_str(node_id.database));
                hash = hash_combine(hash, hash_str(payload.item_type));
                hash = hash_combine(hash, hash_str(payload.item_id));
                hash = hash_combine(hash, hash_str(payload.relationship_type));
            }
            break;
            case storage::s_node_metadata::e_node_kind::relationship:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_relationship_payload>(node_id.payload);
                hash = hash_combine(hash, hash_str(node_id.database));
                hash = hash_combine(hash, hash_str(payload.item_type));
                hash = hash_combine(hash, hash_str(payload.item_id));
                hash = hash_combine(hash, hash_str(payload.relationship_type));
                hash = hash_combine(hash, hash_str(payload.relationship_id));
            }
            break;
            }

            return hash;
        }
    };

    template <>
    struct formatter<storage::s_node_metadata> : std::formatter<std::string>
    {
        constexpr auto parse(auto &ctx)
        {
            return ctx.begin();
        }

        auto format(const storage::s_node_metadata &node_id, auto &ctx) const
        {
            auto out = ctx.out();
            switch (node_id.kind)
            {
            case storage::s_node_metadata::e_node_kind::root:
                out = std::format_to(out, "root");
                break;
            case storage::s_node_metadata::e_node_kind::database:
                out = std::format_to(out, "database[{}]", node_id.database);
                break;
            case storage::s_node_metadata::e_node_kind::item_root:
                out = std::format_to(out, "database[{}]/items", node_id.database);
                break;
            case storage::s_node_metadata::e_node_kind::item_type:
                out = std::format_to(out, "database[{}]/items/[{}]", node_id.database, std::get<storage::s_node_metadata::s_item_type_payload>(node_id.payload).item_type);
                break;
            case storage::s_node_metadata::e_node_kind::item:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_item_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]", node_id.database, payload.item_type, payload.item_id);
            }
            break;
            case storage::s_node_metadata::e_node_kind::property_dir:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_item_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/properties", node_id.database, payload.item_type, payload.item_id);
            }
            break;
            case storage::s_node_metadata::e_node_kind::property:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_property_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/properties/[{}]", node_id.database, payload.item_type, payload.item_id, payload.property_name);
            }
            break;
            case storage::s_node_metadata::e_node_kind::relationships_dir:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_item_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/relationships", node_id.database, payload.item_type, payload.item_id);
            }
            break;
            case storage::s_node_metadata::e_node_kind::relationship_type:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_relationship_payload>(node_id.payload);
                out = std::format_to(out, "database[{}]/items/[{}]/[{}]/relationships/[{}]", node_id.database, payload.item_type, payload.item_id, payload.relationship_type);
            }
            break;
            case storage::s_node_metadata::e_node_kind::relationship:
            {
                const auto &payload = std::get<storage::s_node_metadata::s_relationship_payload>(node_id.payload);
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

    auto s_node_metadata::to_node_id() const -> types::node_id_t
    {
        // Hash the metadata to produce a node ID. Collisions are possible but should be rare enough for practical purposes.
        std::hash<s_node_metadata> hasher;
        return types::node_id_t{ hasher(*this) };
    }
} // namespace storage::inline type
