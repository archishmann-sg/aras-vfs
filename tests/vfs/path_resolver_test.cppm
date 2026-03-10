module;
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <atomic>
#include <expected>
#include <string>
#include <thread>
#include <vector>

export module vfs:path_resolver_test;

import :path_resolver;
import :path_cache;
import :node_registry;
import storage;
import core;

using namespace std::literals::chrono_literals;

// ============================================================================
// Mock Storage Implementation (Storage layer not implemented yet)
// ============================================================================

namespace
{
    enum class e_mock_error : std::uint8_t
    {
        not_implemented = 1
    };

    class c_mock_storage : public storage::c_storage
    {
    public:
        // Mock implementation - storage not yet implemented
        auto lookup_child([[maybe_unused]] const storage::types::node_id_t &parent, [[maybe_unused]] std::string_view name) noexcept
            -> core::result<storage::s_storage_entry>
        {
            // For testing, we'll simulate some basic lookups
            // This will be replaced when actual storage is implemented
            return core::failure(core::e_domain::storage, e_mock_error::not_implemented, "Storage not implemented");
        }
    };
} // anonymous namespace

// ============================================================================
// Unit Tests - Basic Path Resolution
// ============================================================================

TEST_CASE("PathResolver - Constructor and basic setup", "[path_resolver][unit]")
{
    vfs::c_path_cache cache;
    vfs::c_node_registry registry;
    c_mock_storage storage;

    SECTION("Create path resolver")
    {
        vfs::c_path_resolver resolver(cache, registry, storage);
        // If constructor succeeds, test passes
        REQUIRE(true);
    }
}

TEST_CASE("PathResolver - Path splitting and component handling", "[path_resolver][unit]")
{
    vfs::c_path_cache cache;
    vfs::c_node_registry registry;
    c_mock_storage storage;
    vfs::c_path_resolver resolver(cache, registry, storage);

    auto root_id = storage::types::node_id_t{ 1 };

    SECTION("Empty path resolves to start node")
    {
        vfs::s_resolve_request request{
            .start_node = root_id,
            .path = "",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == root_id);
    }

    SECTION("Single dot (.) stays at current node")
    {
        vfs::s_resolve_request request{
            .start_node = root_id,
            .path = ".",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == root_id);
    }

    SECTION("Multiple dots in path")
    {
        vfs::s_resolve_request request{
            .start_node = root_id,
            .path = "./././.",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == root_id);
    }
}

TEST_CASE("PathResolver - Parent directory navigation", "[path_resolver][unit]")
{
    vfs::c_path_cache cache;
    vfs::c_node_registry registry;
    c_mock_storage storage;
    vfs::c_path_resolver resolver(cache, registry, storage);

    SECTION("Double dot (..) requires valid node metadata")
    {
        auto child_id = storage::types::node_id_t{ 100 };
        auto parent_id = storage::types::node_id_t{ 1 };

        // Create a node record with valid metadata
        vfs::s_node_metadata metadata{
            .node_id = child_id,
            .parent_id = parent_id,
            .size = 0,
            .version = 0,
            .created_at = std::chrono::system_clock::now(),
            .modified_at = std::chrono::system_clock::now(),
            .mode = 0644,
            .uid = 1000,
            .gid = 1000,
            .node_type = vfs::s_node_metadata::e_node_type::file,
            .extended_attributes = {}
        };

        auto record = registry.get_or_create(metadata);
        REQUIRE(record.has_value());

        vfs::s_resolve_request request{
            .start_node = child_id,
            .path = "..",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == parent_id);
    }

    SECTION("Parent navigation with invalid metadata fails")
    {
        auto child_id = storage::types::node_id_t{ 100 };

        // Create node without valid metadata
        vfs::s_node_metadata metadata{
            .node_id = child_id,
            .parent_id = storage::types::node_id_t{ 1 },
            .size = 0,
            .version = 0,
            .created_at = std::chrono::system_clock::now(),
            .modified_at = std::chrono::system_clock::now(),
            .mode = 0644,
            .uid = 1000,
            .gid = 1000,
            .node_type = vfs::s_node_metadata::e_node_type::file,
            .extended_attributes = {}
        };

        auto record = registry.get_or_create(metadata);
        REQUIRE(record.has_value());

        // Invalidate metadata
        record.value()->metadata_valid = false;

        vfs::s_resolve_request request{
            .start_node = child_id,
            .path = "..",
            .follow_symlinks = true
        };

        REQUIRE_FALSE(resolver.resolve(request).has_value());
    }
}

TEST_CASE("PathResolver - Cache interaction", "[path_resolver][unit][cache]")
{
    vfs::c_path_cache cache;
    vfs::c_node_registry registry;
    c_mock_storage storage;
    vfs::c_path_resolver resolver(cache, registry, storage);

    auto parent_id = storage::types::node_id_t{ 1 };
    auto child_id = storage::types::node_id_t{ 100 };

    SECTION("Resolve uses positive cache entries")
    {
        // Pre-populate cache
        REQUIRE(cache.put(parent_id, "test.txt", child_id, vfs::c_path_cache::e_put_type::positive));

        vfs::s_resolve_request request{
            .start_node = parent_id,
            .path = "test.txt",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == child_id);
    }

    SECTION("Negative cache entry results in file not found")
    {
        // Pre-populate with negative cache
        REQUIRE(cache.put(parent_id, "missing.txt", std::nullopt, vfs::c_path_cache::e_put_type::negative));

        vfs::s_resolve_request request{
            .start_node = parent_id,
            .path = "missing.txt",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Multi-component path with cached entries")
    {
        auto dir1_id = storage::types::node_id_t{ 10 };
        auto dir2_id = storage::types::node_id_t{ 20 };
        auto file_id = storage::types::node_id_t{ 30 };

        REQUIRE(cache.put(parent_id, "dir1", dir1_id, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(dir1_id, "dir2", dir2_id, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(dir2_id, "file.txt", file_id, vfs::c_path_cache::e_put_type::positive));

        vfs::s_resolve_request request{
            .start_node = parent_id,
            .path = "dir1/dir2/file.txt",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == file_id);
    }
}

TEST_CASE("PathResolver - Complex paths", "[path_resolver][unit]")
{
    vfs::c_path_cache cache;
    vfs::c_node_registry registry;
    c_mock_storage storage;
    vfs::c_path_resolver resolver(cache, registry, storage);

    SECTION("Path with . and .. components")
    {
        auto root_id = storage::types::node_id_t{ 1 };
        auto dir_id = storage::types::node_id_t{ 10 };
        auto file_id = storage::types::node_id_t{ 20 };

        // Setup registry with metadata
        vfs::s_node_metadata dir_meta{
            .node_id = dir_id,
            .parent_id = root_id,
            .size = 0,
            .version = 0,
            .created_at = std::chrono::system_clock::now(),
            .modified_at = std::chrono::system_clock::now(),
            .mode = 0755,
            .uid = 1000,
            .gid = 1000,
            .node_type = vfs::s_node_metadata::e_node_type::directory,
            .extended_attributes = {}
        };
        REQUIRE(registry.get_or_create(dir_meta).has_value());

        // Setup cache
        REQUIRE(cache.put(root_id, "dir", dir_id, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(dir_id, "file.txt", file_id, vfs::c_path_cache::e_put_type::positive));

        vfs::s_resolve_request request{
            .start_node = root_id,
            .path = "./dir/./file.txt",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == file_id);
    }

    SECTION("Absolute-style paths (multiple leading slashes)")
    {
        auto root_id = storage::types::node_id_t{ 1 };
        auto file_id = storage::types::node_id_t{ 10 };

        REQUIRE(cache.put(root_id, "file.txt", file_id, vfs::c_path_cache::e_put_type::positive));

        vfs::s_resolve_request request{
            .start_node = root_id,
            .path = "///file.txt",
            .follow_symlinks = true
        };

        auto result = resolver.resolve(request);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == file_id);
    }
}

// ============================================================================
// Multi-threaded Tests
// ============================================================================

TEST_CASE("PathResolver - Concurrent path resolution", "[path_resolver][multithread]")
{
    vfs::c_path_cache cache;
    vfs::c_node_registry registry;
    c_mock_storage storage;
    vfs::c_path_resolver resolver(cache, registry, storage);

    auto root_id = storage::types::node_id_t{ 1 };

    // Pre-populate paths
    for (int i = 0; i < 50; ++i)
    {
        auto node_id = storage::types::node_id_t{ static_cast<std::uint64_t>(100 + i) };
        REQUIRE(cache.put(root_id, std::format("file_{}.txt", i), node_id, vfs::c_path_cache::e_put_type::positive));
    }

    SECTION("Multiple threads resolving different paths")
    {
        constexpr int num_threads = 8;
        std::vector<std::jthread> threads;
        std::atomic<int> successful_resolves{ 0 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&, thread_id = t]()
                                 {
                for (int i = 0; i < 100; ++i)
                {
                    int file_idx = (thread_id * 5 + i) % 50;
                    std::string path = std::format("file_{}.txt", file_idx);
                    vfs::s_resolve_request request{
                        .start_node = root_id,
                        .path = path,
                        .follow_symlinks = true
                    };

                    auto result = resolver.resolve(request);
                    if (result.has_value())
                    {
                        successful_resolves.fetch_add(1, std::memory_order_relaxed);
                    }
                } });
        }

        threads.clear();
        REQUIRE(successful_resolves.load() == num_threads * 100);
    }

    SECTION("Multiple threads resolving same path")
    {
        auto file_id = storage::types::node_id_t{ 999 };
        REQUIRE(cache.put(root_id, "shared.txt", file_id, vfs::c_path_cache::e_put_type::positive));

        constexpr int num_threads = 10;
        std::vector<std::jthread> threads;
        std::atomic<int> successful_resolves{ 0 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&]()
                                 {
                for (int i = 0; i < 100; ++i)
                {
                    vfs::s_resolve_request request{
                        .start_node = root_id,
                        .path = "shared.txt",
                        .follow_symlinks = true
                    };

                    auto result = resolver.resolve(request);
                    if (result.has_value() && result.value() == file_id)
                    {
                        successful_resolves.fetch_add(1, std::memory_order_relaxed);
                    }
                } });
        }

        threads.clear();
        REQUIRE(successful_resolves.load() == num_threads * 100);
    }
}
