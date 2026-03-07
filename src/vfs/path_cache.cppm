module;
#include <chrono>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
export module vfs:path_cache;

import :types;
import core;
import storage;

export namespace vfs::inline path_cache
{
    /**
     * @class s_path_key
     * @brief The key will be parent_node, node_name
     * This way, it will be easier to find entries
     */
    struct s_path_key
    {
        storage::types::node_id_t parent_id;
        std::string name;

        auto operator==(const s_path_key &) const -> bool = default;
    };

    /**
     * @class s_path_entry
     * @brief The node entry data. It is optional to encode both positive and negative hits in cache
     *
     */
    struct s_path_entry
    {
        std::optional<storage::types::node_id_t> node_id;
        std::chrono::steady_clock::time_point expire_time;
    };
} // namespace vfs::inline path_cache

export namespace std
{
    template <>
    struct hash<vfs::path_cache::s_path_key>
    {
        auto operator()(const vfs::path_cache::s_path_key &key) const noexcept -> std::size_t
        {
            std::size_t hash1 = std::hash<storage::types::node_id_t>{}(key.parent_id);
            std::size_t hash2 = std::hash<std::string>{}(key.name);
            return hash1 ^ hash2;
        }
    };
} // namespace std

export namespace vfs::inline path_cache
{
    class c_path_cache
    {
    public:
        auto get(const storage::types::node_id_t &parent_node, const std::string &name) -> core::result<std::optional<s_path_entry>>
        {
            core::assert(not name.empty(), "Name cannot be empty");
            {
                std::shared_lock read_lock(m_mutex);
                if (auto iter = m_cache.find({ parent_node, name }); iter != m_cache.end())
                {
                    if (clock::now() < iter->second.expire_time)
                    {
                        return iter->second;
                    }
                }
            }
            {
                std::unique_lock write_lock(m_mutex);
                m_cache.erase({ parent_node, name });
            }
            return std::nullopt;
        }

        enum class e_put_type : std::uint8_t
        {
            positive,
            negative
        };

        auto put(const storage::types::node_id_t &parent_id, const std::string &name, const std::optional<storage::types::node_id_t> &node_id, e_put_type type) -> core::status
        {
            using namespace std::literals::chrono_literals;
            switch (type)
            {
            case e_put_type::positive:
                return put(parent_id, name, node_id, 500ms);
            case e_put_type::negative:
                return put(parent_id, name, node_id, 200ms);
                break;
            }
            std::unreachable();
        }

        auto put(const storage::types::node_id_t &parent_id, const std::string &name, std::optional<storage::types::node_id_t> node_id, std::chrono::milliseconds ttl) -> core::status
        {
            core::assert(not name.empty(), "Path cannot be empty");
            core::assert(ttl.count() > 0, "TTL must be greater than zero");

            std::unique_lock write_lock(m_mutex);
            m_cache[{ parent_id, name }] = { .node_id = std::move(node_id), .expire_time = clock::now() + ttl };
            return {};
        }

        auto invalidate(storage::types::node_id_t parent_id, const std::string &name) -> core::status
        {
            core::assert(not name.empty(), "Name cannot be empty");
            std::unique_lock write_lock(m_mutex);
            m_cache.erase({ std::move(parent_id), name });
            return {};
        }

        auto invalidate_children(const storage::types::node_id_t &parent_id) -> core::status
        {
            std::unique_lock lock(m_mutex);

            for (auto iter = m_cache.begin(); iter != m_cache.end();)
            {
                if (iter->first.parent_id == parent_id)
                {
                    iter = m_cache.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }

            return {};
        }

        auto clear() -> core::status
        {
            std::unique_lock write_lock(m_mutex);
            m_cache.clear();
            return {};
        }

    private:
        using clock = std::chrono::steady_clock;

        mutable std::shared_mutex m_mutex;
        std::unordered_map<s_path_key, s_path_entry> m_cache;
    };
} // namespace vfs::inline path_cache
