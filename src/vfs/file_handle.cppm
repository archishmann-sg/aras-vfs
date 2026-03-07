module;
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <variant>
#include <vector>
export module vfs:file_handle;

import :types;
import :node_registry;
import storage;
import core;

export namespace vfs::inline file_handle
{
    namespace tags
    {
        struct open_mask_t;
        struct access_mask_t;
    } // namespace tags

    struct s_file_handle
    {
        types::file_handle_id_t handle_id;

        std::shared_ptr<node_registry::s_node_record> node;

        std::uint64_t offset{ 0 };

        core::c_strong_type<std::uint32_t, tags::open_mask_t> open_mask;
        core::c_strong_type<std::uint32_t, tags::access_mask_t> access_mask;

        std::uint64_t open_generation{ 0 };

        std::mutex handle_lock;

        bool dirty{ false };
        bool append{ false };
        bool readable{ false };
        bool writeable{ false };
    };

    struct s_directory_entry
    {
        std::string name;
        storage::types::node_id_t node_id;
        node_registry::s_node_metadata::e_node_type node_type;
        std::uint64_t size{ 0 };
    };

    struct s_directory_handle
    {
        types::file_handle_id_t handle_id;

        std::shared_ptr<node_registry::s_node_record> node;

        std::size_t cursor{ 0 };
        std::vector<s_directory_entry> snapshot;

        std::mutex handle_lock;
    };

    using s_handle = std::variant<s_file_handle, s_directory_handle>;

    class c_handle_table
    {
    public:
        auto create_file_handle(std::shared_ptr<node_registry::s_node_record> node, core::c_strong_type<std::uint32_t, tags::open_mask_t> open_mask, core::c_strong_type<std::uint32_t, tags::access_mask_t> access_mask) -> core::result<types::file_handle_id_t>
        {
            const auto handle_id = m_next_handle_id.fetch_add(1, std::memory_order_relaxed);
            auto h_id = types::file_handle_id_t(handle_id);

            auto handle = std::make_unique<s_handle>(std::in_place_type<s_file_handle>);
            auto &file_handle = std::get<s_file_handle>(*handle);

            file_handle.handle_id = h_id;
            file_handle.node = std::move(node);
            file_handle.open_mask = open_mask;
            file_handle.access_mask = access_mask;
            file_handle.open_generation = file_handle.node->generation;
            {
                std::scoped_lock lock(m_mutex);
                m_handles.emplace(types::file_handle_id_t(handle_id), std::move(handle));
            }
            return h_id;
        }

        auto create_dir_handle(std::shared_ptr<node_registry::s_node_record> node) -> core::result<types::file_handle_id_t>
        {
            const auto handle_id = m_next_handle_id.fetch_add(1, std::memory_order_relaxed);
            auto h_id = types::file_handle_id_t(handle_id);

            auto handle = std::make_unique<s_handle>(std::in_place_type<s_directory_handle>);
            auto &dir_handle = std::get<s_directory_handle>(*handle);
            dir_handle.handle_id = h_id;
            dir_handle.node = std::move(node);

            {
                std::scoped_lock lock(m_mutex);
                m_handles.emplace(types::file_handle_id_t(handle_id), std::move(handle));
            }
            return h_id;
        }

        auto get(types::file_handle_id_t handle_id) -> core::result<std::shared_ptr<s_handle>>
        {
            std::shared_lock lock(m_mutex);
            auto iter = m_handles.find(handle_id);
            if (iter != m_handles.end())
            {
                auto handle = iter->second;
                return handle;
            }
            return {};
        }

        auto close(types::file_handle_id_t handle_id) -> core::status
        {
            std::unique_lock lock(m_mutex);
            auto iter = m_handles.find(handle_id);
            if (iter != m_handles.end())
            {
                m_handles.erase(iter);
            }
            return {};
        }

        auto clear() -> core::status
        {
            std::unique_lock lock(m_mutex);
            m_handles.clear();
            return {};
        }

    private:
        using open_mask_t = core::c_strong_type<std::uint32_t, tags::open_mask_t>;
        using access_mask_t = core::c_strong_type<std::uint32_t, tags::access_mask_t>;

        std::unordered_map<types::file_handle_id_t, std::shared_ptr<s_handle>> m_handles;
        std::atomic<std::uint64_t> m_next_handle_id{ 1 };
        std::shared_mutex m_mutex;
    };
} // namespace vfs::inline file_handle
