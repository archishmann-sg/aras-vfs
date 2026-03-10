module;
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <atomic>
#include <barrier>
#include <latch>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>
#include <vector>

export module vfs:file_handle_test;

import :file_handle;
import :node_registry;
import storage;
import core;

using namespace std::literals::chrono_literals;

// ============================================================================
// Unit Tests - File Handle Creation
// ============================================================================

TEST_CASE("HandleTable - Create file handles", "[file_handle][unit]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    // Create a node
    auto node_id = storage::types::node_id_t{ 42 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 1 },
        .size = 1024,
        .version = 0,
        .created_at = std::chrono::system_clock::now(),
        .modified_at = std::chrono::system_clock::now(),
        .mode = 0644,
        .uid = 1000,
        .gid = 1000,
        .node_type = vfs::s_node_metadata::e_node_type::file,
        .extended_attributes = {}
    };
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Create a basic file handle")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        REQUIRE(result.has_value());

        auto handle_id = result.value();
        REQUIRE(handle_id.get() > 0);
    }

    SECTION("Create multiple file handles")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto result1 = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        auto result2 = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        auto result3 = handle_table.create_file_handle(node.value(), open_mask, access_mask);

        REQUIRE(result1.has_value());
        REQUIRE(result2.has_value());
        REQUIRE(result3.has_value());

        // Each handle should have a unique ID
        REQUIRE(result1.value() != result2.value());
        REQUIRE(result2.value() != result3.value());
        REQUIRE(result1.value() != result3.value());
    }

    SECTION("Multiple handles can reference the same node")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto result1 = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        auto result2 = handle_table.create_file_handle(node.value(), open_mask, access_mask);

        REQUIRE(result1.has_value());
        REQUIRE(result2.has_value());

        auto handle1 = handle_table.get(result1.value());
        auto handle2 = handle_table.get(result2.value());

        REQUIRE(handle1.has_value());
        REQUIRE(handle2.has_value());

        // Different handles but same node
        auto &fh1 = std::get<vfs::s_file_handle>(*handle1.value());
        auto &fh2 = std::get<vfs::s_file_handle>(*handle2.value());

        REQUIRE(fh1.handle_id != fh2.handle_id);
        REQUIRE(fh1.node == fh2.node);
    }
}

TEST_CASE("HandleTable - Create directory handles", "[file_handle][unit]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    // Create a directory node
    auto node_id = storage::types::node_id_t{ 100 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 1 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Create a directory handle")
    {
        auto result = handle_table.create_dir_handle(node.value());
        REQUIRE(result.has_value());

        auto handle_id = result.value();
        REQUIRE(handle_id.get() > 0);
    }

    SECTION("Directory handle contains node reference")
    {
        auto result = handle_table.create_dir_handle(node.value());
        REQUIRE(result.has_value());

        auto handle = handle_table.get(result.value());
        REQUIRE(handle.has_value());

        auto &dir_handle = std::get<vfs::s_directory_handle>(*handle.value());
        REQUIRE(dir_handle.node == node.value());
        REQUIRE(dir_handle.cursor == 0);
    }
}

TEST_CASE("HandleTable - Get handles", "[file_handle][unit]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Get existing file handle")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto create_result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        REQUIRE(handle.has_value());
        REQUIRE(std::holds_alternative<vfs::s_file_handle>(*handle.value()));

        auto &file_handle = std::get<vfs::s_file_handle>(*handle.value());
        REQUIRE(file_handle.handle_id == create_result.value());
        REQUIRE(file_handle.node == node.value());
        REQUIRE(file_handle.open_mask == open_mask);
        REQUIRE(file_handle.access_mask == access_mask);
        REQUIRE(file_handle.offset == 0);
        REQUIRE(file_handle.open_generation == 0);
        REQUIRE(file_handle.dirty == false);
    }

    SECTION("Get existing directory handle")
    {
        auto create_result = handle_table.create_dir_handle(node.value());
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        REQUIRE(handle.has_value());
        REQUIRE(std::holds_alternative<vfs::s_directory_handle>(*handle.value()));

        auto &dir_handle = std::get<vfs::s_directory_handle>(*handle.value());
        REQUIRE(dir_handle.handle_id == create_result.value());
        REQUIRE(dir_handle.node == node.value());
    }

    SECTION("Get non-existent handle returns empty")
    {
        auto fake_id = vfs::types::file_handle_id_t{ 99999 };
        auto result = handle_table.get(fake_id);
        REQUIRE(result.has_value());
        REQUIRE_FALSE(result.value());
    }
}

TEST_CASE("HandleTable - Close handles", "[file_handle][unit]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Close existing handle")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto create_result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        REQUIRE(create_result.has_value());

        auto status = handle_table.close(create_result.value());
        REQUIRE(status);

        // Handle should no longer exist
        auto get_result = handle_table.get(create_result.value());
        REQUIRE(get_result.has_value());
        REQUIRE_FALSE(get_result.value());
    }

    SECTION("Close non-existent handle")
    {
        auto fake_id = vfs::types::file_handle_id_t{ 88888 };
        auto status = handle_table.close(fake_id);
        REQUIRE(status); // Should succeed (no-op)
    }

    SECTION("Close multiple handles")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        std::vector<vfs::types::file_handle_id_t> handles;
        for (int i = 0; i < 10; ++i)
        {
            auto result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
            REQUIRE(result.has_value());
            handles.push_back(result.value());
        }

        // Close all handles
        for (auto handle_id : handles)
        {
            REQUIRE(handle_table.close(handle_id));
        }

        // Verify all are gone
        for (auto handle_id : handles)
        {
            auto result = handle_table.get(handle_id);
            REQUIRE_FALSE(result.value());
        }
    }
}

TEST_CASE("HandleTable - Clear all handles", "[file_handle][unit]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Clear empty handle table")
    {
        auto status = handle_table.clear();
        REQUIRE(status);
    }

    SECTION("Clear populated handle table")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        std::vector<vfs::types::file_handle_id_t> handles;
        for (int i = 0; i < 50; ++i)
        {
            auto result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
            REQUIRE(result.has_value());
            handles.push_back(result.value());
        }

        auto status = handle_table.clear();
        REQUIRE(status);

        // Verify all are gone
        for (auto handle_id : handles)
        {
            auto result = handle_table.get(handle_id);
            REQUIRE_FALSE(result.value());
        }
    }
}

TEST_CASE("FileHandle - Generation tracking", "[file_handle][unit][generation]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node_record = registry.get_or_create(metadata);
    REQUIRE(node_record.has_value());

    SECTION("Handle captures node generation at open time")
    {
        node_record.value()->generation = 5;

        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto create_result = handle_table.create_file_handle(node_record.value(), open_mask, access_mask);
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        REQUIRE(handle.has_value());

        auto &file_handle = std::get<vfs::s_file_handle>(*handle.value());
        REQUIRE(file_handle.open_generation == 5);
    }

    SECTION("Handle generation vs node generation (ahead/behind detection)")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        // Node at generation 0
        node_record.value()->generation = 0;

        auto create_result = handle_table.create_file_handle(node_record.value(), open_mask, access_mask);
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        REQUIRE(handle.has_value());
        auto &file_handle = std::get<vfs::s_file_handle>(*handle.value());

        REQUIRE(file_handle.open_generation == 0);

        // Simulate node being updated
        node_record.value()->generation = 10;

        // Handle is now behind
        REQUIRE(file_handle.open_generation < file_handle.node->generation);
    }

    SECTION("Dirty handle with generation tracking")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        node_record.value()->generation = 3;

        auto create_result = handle_table.create_file_handle(node_record.value(), open_mask, access_mask);
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        auto &file_handle = std::get<vfs::s_file_handle>(*handle.value());

        REQUIRE(file_handle.dirty == false);

        // Mark as dirty (simulating a write)
        file_handle.dirty = true;
        REQUIRE(file_handle.dirty == true);

        // Dirty handle is ahead of node
        REQUIRE(file_handle.dirty);
        REQUIRE(file_handle.open_generation == 3);
    }
}

TEST_CASE("FileHandle - Offset tracking", "[file_handle][unit][offset]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Initial offset is 0")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto create_result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        auto &file_handle = std::get<vfs::s_file_handle>(*handle.value());

        REQUIRE(file_handle.offset == 0);
    }

    SECTION("Offset can be modified")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto create_result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        auto &file_handle = std::get<vfs::s_file_handle>(*handle.value());

        file_handle.offset = 1024;
        REQUIRE(file_handle.offset == 1024);

        file_handle.offset += 512;
        REQUIRE(file_handle.offset == 1536);
    }
}

TEST_CASE("DirectoryHandle - Snapshot and cursor", "[file_handle][unit][directory]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Initial cursor is 0")
    {
        auto create_result = handle_table.create_dir_handle(node.value());
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        auto &dir_handle = std::get<vfs::s_directory_handle>(*handle.value());

        REQUIRE(dir_handle.cursor == 0);
    }

    SECTION("Snapshot can be populated")
    {
        auto create_result = handle_table.create_dir_handle(node.value());
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        auto &dir_handle = std::get<vfs::s_directory_handle>(*handle.value());

        // Add entries to snapshot
        dir_handle.snapshot.push_back({ "file1.txt",
                                        storage::types::node_id_t{ 100 },
                                        vfs::s_node_metadata::e_node_type::file,
                                        1024 });
        dir_handle.snapshot.push_back({ "file2.txt",
                                        storage::types::node_id_t{ 101 },
                                        vfs::s_node_metadata::e_node_type::file,
                                        2048 });

        REQUIRE(dir_handle.snapshot.size() == 2);
        REQUIRE(dir_handle.snapshot[0].name == "file1.txt");
        REQUIRE(dir_handle.snapshot[1].size == 2048);
    }

    SECTION("Cursor can be advanced")
    {
        auto create_result = handle_table.create_dir_handle(node.value());
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        auto &dir_handle = std::get<vfs::s_directory_handle>(*handle.value());

        dir_handle.cursor = 5;
        REQUIRE(dir_handle.cursor == 5);

        dir_handle.cursor++;
        REQUIRE(dir_handle.cursor == 6);
    }
}

// ============================================================================
// Multi-threaded Tests
// ============================================================================

TEST_CASE("HandleTable - Concurrent handle creation", "[file_handle][multithread]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Multiple threads creating file handles")
    {
        constexpr int num_threads = 10;
        constexpr int handles_per_thread = 100;
        std::vector<std::jthread> threads;
        std::atomic<int> created_count{ 0 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&]()
                                 {
                auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
                auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

                for (int i = 0; i < handles_per_thread; ++i)
                {
                    auto result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
                    if (result.has_value())
                    {
                        created_count.fetch_add(1, std::memory_order_relaxed);
                    }
                } });
        }

        threads.clear();
        REQUIRE(created_count.load() == num_threads * handles_per_thread);
    }

    SECTION("Multiple threads creating directory handles")
    {
        constexpr int num_threads = 8;
        std::vector<std::jthread> threads;
        std::atomic<int> created_count{ 0 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&]()
                                 {
                for (int i = 0; i < 50; ++i)
                {
                    auto result = handle_table.create_dir_handle(node.value());
                    if (result.has_value())
                    {
                        created_count.fetch_add(1, std::memory_order_relaxed);
                    }
                } });
        }

        threads.clear();
        REQUIRE(created_count.load() == num_threads * 50);
    }
}

TEST_CASE("HandleTable - Concurrent mixed operations", "[file_handle][multithread]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Simultaneous create, get, and close operations")
    {
        constexpr int num_creator_threads = 5;
        constexpr int num_reader_threads = 5;
        constexpr int num_closer_threads = 3;
        std::vector<std::jthread> threads;
        std::vector<vfs::types::file_handle_id_t> shared_handles;
        std::mutex handles_mutex;

        // Pre-create some handles
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        for (int i = 0; i < 50; ++i)
        {
            auto result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
            if (result.has_value())
            {
                shared_handles.push_back(result.value());
            }
        }

        threads.reserve(num_creator_threads + num_reader_threads + num_closer_threads);

        // Creator threads
        for (int t = 0; t < num_creator_threads; ++t)
        {
            threads.emplace_back([&]()
                                 {
                for (int i = 0; i < 50; ++i)
                {
                    auto result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
                    if (result.has_value())
                    {
                        std::scoped_lock lock(handles_mutex);
                        shared_handles.push_back(result.value());
                    }
                } });
        }

        // Reader threads
        for (int t = 0; t < num_reader_threads; ++t)
        {
            threads.emplace_back([&]()
                                 {
                for (int i = 0; i < 200; ++i)
                {
                    vfs::types::file_handle_id_t handle_id;
                    {
                        std::scoped_lock lock(handles_mutex);
                        if (!shared_handles.empty())
                        {
                            handle_id = shared_handles[static_cast<std::size_t>(i) % shared_handles.size()];
                        }
                        else
                        {
                            continue;
                        }
                    }
                    auto result = handle_table.get(handle_id);
                    // Result may or may not be valid due to concurrent closes
                } });
        }

        // Closer threads
        for (int t = 0; t < num_closer_threads; ++t)
        {
            threads.emplace_back([&]()
                                 {
                for (int i = 0; i < 30; ++i)
                {
                    vfs::types::file_handle_id_t handle_id;
                    {
                        std::scoped_lock lock(handles_mutex);
                        if (shared_handles.size() > 10)
                        {
                            handle_id = shared_handles.back();
                            shared_handles.pop_back();
                        }
                        else
                        {
                            continue;
                        }
                    }
                    handle_table.close(handle_id);
                    std::this_thread::sleep_for(1ms);
                } });
        }

        threads.clear();
        // Test completes without deadlock or crash
        REQUIRE(true);
    }
}

TEST_CASE("FileHandle - Concurrent offset modifications", "[file_handle][multithread]")
{
    vfs::c_handle_table handle_table;
    vfs::c_node_registry registry;

    auto node_id = storage::types::node_id_t{ 1 };
    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 },
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
    auto node = registry.get_or_create(metadata);
    REQUIRE(node.has_value());

    SECTION("Multiple threads modifying same handle offset")
    {
        auto open_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::open_mask_t>{ 0x01 };
        auto access_mask = core::c_strong_type<std::uint32_t, vfs::file_handle::tags::access_mask_t>{ 0x02 };

        auto create_result = handle_table.create_file_handle(node.value(), open_mask, access_mask);
        REQUIRE(create_result.has_value());

        auto handle = handle_table.get(create_result.value());
        REQUIRE(handle.has_value());
        auto handle_ptr = handle.value();

        constexpr int num_threads = 10;
        std::vector<std::jthread> threads;

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&]()
                                 {
                auto& file_handle = std::get<vfs::s_file_handle>(*handle_ptr);
                for (int i = 0; i < 1000; ++i)
                {
                    std::scoped_lock lock(file_handle.handle_lock);
                    file_handle.offset++;
                } });
        }

        threads.clear();

        auto &file_handle = std::get<vfs::s_file_handle>(*handle_ptr);
        REQUIRE(file_handle.offset == 10000);
    }
}