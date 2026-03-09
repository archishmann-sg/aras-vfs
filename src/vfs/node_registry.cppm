module;
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <variant>
export module vfs:node_registry;

import core;
import storage;

export namespace vfs::inline node_registry
{
    struct s_node_metadata
    {
        storage::types::node_id_t node_id;
        storage::types::node_id_t parent_id;

        std::uint64_t size{};
        std::uint64_t version{};

        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point modified_at;

        std::uint32_t mode{};
        std::uint32_t uid{};
        std::uint32_t gid{};

        enum class e_node_type : std::uint8_t
        {
            file,
            directory,
            symlink,
        };
        e_node_type node_type{ e_node_type::file };

        std::unordered_map<std::string, std::string> extended_attributes;
    };

    struct s_file_blob
    {
        std::vector<std::byte> data;
    };
    struct s_property_table
    {
        std::unordered_map<std::string, std::string> properties;
    };
    struct s_directory_listing
    {
        struct s_entry
        {
            std::string name;
            storage::types::node_id_t node_id;
        };
        std::vector<s_entry> entries;
    };
    struct s_symlink_target
    {
        std::string target_path;
    };
    using s_node_content = std::variant<std::monostate, s_file_blob, s_property_table, s_directory_listing, s_symlink_target>;

    struct s_node_record
    {
        std::mutex mutex;
        std::condition_variable cv;

        std::uint64_t generation{};

        s_node_metadata metadata;
        s_node_content content;

        bool metadata_valid{};
        bool content_valid{};
        bool loading{};
        bool dirty{};
    };

    class c_node_registry
    {
    public:
        auto get(const storage::types::node_id_t &node_id) noexcept -> core::result<std::shared_ptr<s_node_record>>
        {
            std::shared_lock lock(m_mutex);
            auto iter = m_cache.find(node_id);
            if (iter != m_cache.end())
            {
                return iter->second;
            }
            return {};
        }

        auto get_or_create(s_node_metadata metadata) noexcept -> core::result<std::shared_ptr<s_node_record>>
        {
            std::unique_lock lock(m_mutex);
            auto iter = m_cache.find(metadata.node_id);
            if (iter != m_cache.end())
            {
                return iter->second;
            }

            auto record = std::make_shared<s_node_record>();
            record->metadata = std::move(metadata);
            record->generation = 0;
            record->dirty = false;
            record->metadata_valid = true;
            record->content_valid = false;
            record->loading = false;
            m_cache[record->metadata.node_id] = record;
            return record;
        }

        auto remove(const storage::types::node_id_t &node_id) noexcept -> core::status
        {
            std::unique_lock lock(m_mutex);
            auto iter = m_cache.find(node_id);
            if (iter != m_cache.end())
            {
                m_cache.erase(iter);
            }
            return {};
        }

        auto clear() noexcept -> core::status
        {
            std::unique_lock lock(m_mutex);
            m_cache.clear();
            return {};
        }

    private:
        std::unordered_map<storage::types::node_id_t, std::shared_ptr<s_node_record>> m_cache;
        mutable std::shared_mutex m_mutex;
    };
} // namespace vfs::inline node_registry
