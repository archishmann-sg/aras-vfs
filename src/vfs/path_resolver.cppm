module;
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>
export module vfs:path_resolver;

import :types;
import :path_cache;
import :node_registry;

import storage;
import core;

export namespace vfs::inline path_resolver
{
    struct s_resolve_request
    {
        storage::types::node_id_t start_node;
        std::string_view path;
        std::size_t symlink_resolution_depth = 16;
        bool follow_symlinks = true;
    };

    class c_path_resolver
    {
    public:
        c_path_resolver(c_path_cache &path_cache, c_node_registry &node_registry, storage::c_storage &storage) noexcept
            : m_path_cache(path_cache),
              m_node_registry(node_registry),
              m_storage(storage)
        {
        }

        // No copy or move. Contains reference members.
        c_path_resolver(const c_path_resolver &) = delete;
        auto operator=(const c_path_resolver &) -> c_path_resolver & = delete;
        c_path_resolver(c_path_resolver &&) = delete;
        auto operator=(c_path_resolver &&) -> c_path_resolver & = delete;

        auto resolve(const s_resolve_request &request) noexcept -> core::result<storage::types::node_id_t>
        {
            static auto split_path = [](std::string_view path) -> std::vector<std::string_view>
            {
                return path
                       | std::views::split('/')
                       | std::views::transform([](auto rng) -> auto
                                               { return std::string_view{ rng.begin(), rng.end() }; })
                       | std::views::filter([](std::string_view str_v) -> bool
                                            { return not str_v.empty(); })
                       | std::ranges::to<std::vector>();
            };

            storage::types::node_id_t current_node = request.start_node;

            auto path_components = split_path(request.path);

            for (auto name : path_components)
            {
                if (name == ".")
                {
                    continue;
                }
                if (name == "..")
                {
                    auto parent = m_node_registry.get(current_node)
                                      .and_then([](const std::shared_ptr<s_node_record> &record) -> core::result<storage::types::node_id_t>
                                                {
                                                    if (record->metadata_valid) 
                                                    {
                                                        return record->metadata.parent_id;
                                                    } 
                                                    return core::failure(core::e_domain::vfs, e_err_code::unknown_error, "Node metadata invalid!"); });

                    if (not parent)
                    {
                        core::s_error &err = parent.error();
                        if (err.code.is<e_err_code>() and err.code.as<e_err_code>() == e_err_code::unknown_error)
                        {
                            // TODO: Think what to do here
                        }
                    }

                    current_node = *parent;
                    continue;
                }

                auto next = resolve_component(current_node, name);

                if (not next)
                {
                    // Propagate the error to caller, and stop descending further
                    return next;
                }

                current_node = *next;
            }

            return current_node;
        }

    private:
        c_path_cache &m_path_cache;
        c_node_registry &m_node_registry;
        storage::c_storage &m_storage;

        auto resolve_component(const storage::types::node_id_t &parent, std::string_view name) noexcept -> core::result<storage::types::node_id_t>
        {
            core::result<std::optional<s_path_entry>> cache_result = m_path_cache.get(parent, std::string(name));
            if (cache_result)
            {

                std::optional<s_path_entry> &cache_entry = *cache_result;
                if (cache_entry)
                {
                    // Cache hit
                    auto node_opt = cache_entry->node_id;
                    if (node_opt)
                    {
                        // Positive
                        return *node_opt;
                    }

                    // Negative cache entry
                    return core::failure(core::e_domain::vfs, e_err_code::file_not_found, std::format("File/Directory not Found: {}", name));
                }
            }
            else
            {
                // TODO: Log path_cache failure
            }
            // Cache miss
            // Perform lookup

            auto storage_lookup_result = m_storage.lookup_child(parent, name);

            if (not storage_lookup_result)
            {
                return m_path_cache.put(parent, std::string(name), std::nullopt, c_path_cache::e_put_type::negative)
                    .and_then([&name](auto) -> core::result<storage::types::node_id_t>
                              { return core::failure(core::e_domain::vfs, e_err_code::file_not_found, std::format("File/Directory not Found: {}", name)); });
            }

            storage::types::node_id_t &node_id = (*storage_lookup_result).id;
            return m_path_cache.put(parent, std::string(name), node_id, c_path_cache::e_put_type::positive)
                .and_then([&node_id](auto) -> core::result<storage::types::node_id_t>
                          { return node_id; });
        }
    };
} // namespace vfs::inline path_resolver
