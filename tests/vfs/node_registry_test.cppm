module;
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>
#include <vector>

export module vfs:node_registry_test;

import :node_registry;
import storage;
import core;

using namespace std::literals::chrono_literals;

// ============================================================================
// Unit Tests - Basic Operations
// ============================================================================

TEST_CASE("NodeRegistry - Basic get and get_or_create operations", "[node_registry][unit]")
{
    vfs::c_node_registry registry;

    SECTION("Get non-existent node returns empty")
    {
        auto node_id = storage::types::node_id_t{ 1 };
        auto result = registry.get(node_id);
        REQUIRE(result.has_value());
        REQUIRE_FALSE(result.value()); // Should be empty optional
    }

    SECTION("Get_or_create creates new node")
    {
        auto node_id = storage::types::node_id_t{ 42 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 1 },
            .size = 1024,
            .version = 1,
            .mode = 0644,
            .uid = 1000,
            .gid = 1000,
            .node_type = vfs::s_node_metadata::e_node_type::file,
            .extended_attributes = {},
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());

        auto record = result.value();
        REQUIRE(record != nullptr);
        REQUIRE(record->metadata.node_id == node_id);
        REQUIRE(record->metadata.size == 1024);
        REQUIRE(record->metadata.mode == 0644);
        REQUIRE(record->metadata_valid == true);
        REQUIRE(record->content_valid == false);
        REQUIRE(record->generation == 0);
        REQUIRE(record->dirty == false);
    }

    SECTION("Get returns previously created node")
    {
        auto node_id = storage::types::node_id_t{ 100 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 1 },
            .size = 2048
        };

        auto create_result = registry.get_or_create(metadata);
        REQUIRE(create_result.has_value());

        auto get_result = registry.get(node_id);
        REQUIRE(get_result.has_value());
        REQUIRE(get_result.value() != nullptr);
        REQUIRE(get_result.value()->metadata.node_id == node_id);
        REQUIRE(get_result.value()->metadata.size == 2048);
    }

    SECTION("Get_or_create returns existing node without modification")
    {
        auto node_id = storage::types::node_id_t{ 200 };
        vfs::s_node_metadata metadata1{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 1 },
            .size = 1000,
            .version = 1
        };

        auto result1 = registry.get_or_create(metadata1);
        REQUIRE(result1.has_value());

        // Modify the record
        result1.value()->generation = 5;
        result1.value()->dirty = true;

        // Try to create again with different metadata
        vfs::s_node_metadata metadata2{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 2 },
            .size = 2000,
            .version = 2
        };

        auto result2 = registry.get_or_create(metadata2);
        REQUIRE(result2.has_value());

        // Should be the same object
        REQUIRE(result2.value() == result1.value());
        // Should have original values, not new metadata
        REQUIRE(result2.value()->metadata.size == 1000);
        REQUIRE(result2.value()->metadata.version == 1);
        REQUIRE(result2.value()->generation == 5); // Still modified
        REQUIRE(result2.value()->dirty == true);
    }
}

TEST_CASE("NodeRegistry - Node metadata types", "[node_registry][unit]")
{
    vfs::c_node_registry registry;

    SECTION("Create file node")
    {
        auto node_id = storage::types::node_id_t{ 1 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 0 },
            .size = 512,
            .node_type = vfs::s_node_metadata::e_node_type::file,
            .extended_attributes = {}
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());
        REQUIRE(result.value()->metadata.node_type == vfs::s_node_metadata::e_node_type::file);
    }

    SECTION("Create directory node")
    {
        auto node_id = storage::types::node_id_t{ 2 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 0 },
            .node_type = vfs::s_node_metadata::e_node_type::directory,
            .extended_attributes = {}
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());
        REQUIRE(result.value()->metadata.node_type == vfs::s_node_metadata::e_node_type::directory);
    }

    SECTION("Create symlink node")
    {
        auto node_id = storage::types::node_id_t{ 3 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 0 },
            .node_type = vfs::s_node_metadata::e_node_type::symlink,
            .extended_attributes = {}
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());
        REQUIRE(result.value()->metadata.node_type == vfs::s_node_metadata::e_node_type::symlink);
    }
}

TEST_CASE("NodeRegistry - Node content variants", "[node_registry][unit]")
{
    vfs::c_node_registry registry;

    SECTION("File blob content")
    {
        auto node_id = storage::types::node_id_t{ 10 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 1 }
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());

        // Set file blob content
        std::vector<std::byte> data{ std::byte{ 0x48 }, std::byte{ 0x65 }, std::byte{ 0x6C }, std::byte{ 0x6C }, std::byte{ 0x6F } };
        result.value()->content = vfs::s_file_blob{ data };
        result.value()->content_valid = true;

        // Verify
        REQUIRE(std::holds_alternative<vfs::s_file_blob>(result.value()->content));
        auto &blob = std::get<vfs::s_file_blob>(result.value()->content);
        REQUIRE(blob.data.size() == 5);
    }

    SECTION("Directory listing content")
    {
        auto node_id = storage::types::node_id_t{ 20 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 1 }
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());

        // Set directory listing
        vfs::s_directory_listing listing;
        listing.entries.push_back({ "file1.txt", storage::types::node_id_t{ 100 } });
        listing.entries.push_back({ "file2.txt", storage::types::node_id_t{ 101 } });
        result.value()->content = listing;
        result.value()->content_valid = true;

        // Verify
        REQUIRE(std::holds_alternative<vfs::s_directory_listing>(result.value()->content));
        auto &dir_list = std::get<vfs::s_directory_listing>(result.value()->content);
        REQUIRE(dir_list.entries.size() == 2);
        REQUIRE(dir_list.entries[0].name == "file1.txt");
    }

    SECTION("Symlink target content")
    {
        auto node_id = storage::types::node_id_t{ 30 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 1 }
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());

        // Set symlink target
        result.value()->content = vfs::s_symlink_target{ "/path/to/target" };
        result.value()->content_valid = true;

        // Verify
        REQUIRE(std::holds_alternative<vfs::s_symlink_target>(result.value()->content));
        auto &symlink = std::get<vfs::s_symlink_target>(result.value()->content);
        REQUIRE(symlink.target_path == "/path/to/target");
    }
}

TEST_CASE("NodeRegistry - Node removal", "[node_registry][unit]")
{
    vfs::c_node_registry registry;

    SECTION("Remove existing node")
    {
        auto node_id = storage::types::node_id_t{ 100 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 1 }
        };

        REQUIRE(registry.get_or_create(metadata).has_value());

        auto status = registry.remove(node_id);
        REQUIRE(status);

        auto get_result = registry.get(node_id);
        REQUIRE(get_result.has_value());
        REQUIRE_FALSE(get_result.value()); // Should be empty
    }

    SECTION("Remove non-existent node")
    {
        auto node_id = storage::types::node_id_t{ 999 };
        auto status = registry.remove(node_id);
        REQUIRE(status); // Should succeed (no-op)
    }

    SECTION("Remove multiple nodes")
    {
        for (std::uint64_t i = 0; i < 10; ++i)
        {
            auto node_id = storage::types::node_id_t{ i };
            vfs::s_node_metadata metadata{
                .node_id = node_id,
                .parent_id = storage::types::node_id_t{ 0 }
            };
            REQUIRE(registry.get_or_create(metadata).has_value());
        }

        for (std::uint64_t i = 0; i < 10; i += 2)
        {
            REQUIRE(registry.remove(storage::types::node_id_t{ i }));
        }

        // Verify even nodes are removed, odd nodes remain
        for (std::uint64_t i = 0; i < 10; ++i)
        {
            auto result = registry.get(storage::types::node_id_t{ i });
            if (i % 2 == 0)
            {
                REQUIRE_FALSE(result.value());
            }
            else
            {
                REQUIRE(result.value() != nullptr);
            }
        }
    }
}

TEST_CASE("NodeRegistry - Clear all nodes", "[node_registry][unit]")
{
    vfs::c_node_registry registry;

    SECTION("Clear empty registry")
    {
        auto status = registry.clear();
        REQUIRE(status);
    }

    SECTION("Clear populated registry")
    {
        // Add multiple nodes
        for (std::uint64_t i = 0; i < 100; ++i)
        {
            vfs::s_node_metadata metadata{
                .node_id = storage::types::node_id_t{ i },
                .parent_id = storage::types::node_id_t{ i / 10 }
            };
            REQUIRE(registry.get_or_create(metadata).has_value());
        }

        auto status = registry.clear();
        REQUIRE(status);

        // Verify all nodes are gone
        for (std::uint64_t i = 0; i < 100; ++i)
        {
            auto result = registry.get(storage::types::node_id_t{ i });
            REQUIRE(result.has_value());
            REQUIRE_FALSE(result.value());
        }
    }
}

TEST_CASE("NodeRegistry - Generation tracking", "[node_registry][unit]")
{
    vfs::c_node_registry registry;

    SECTION("Initial generation is 0")
    {
        auto node_id = storage::types::node_id_t{ 1 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 0 }
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());
        REQUIRE(result.value()->generation == 0);
    }

    SECTION("Generation can be incremented")
    {
        auto node_id = storage::types::node_id_t{ 1 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 0 }
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());

        result.value()->generation++;
        REQUIRE(result.value()->generation == 1);

        result.value()->generation++;
        REQUIRE(result.value()->generation == 2);
    }
}

TEST_CASE("NodeRegistry - Dirty flag tracking", "[node_registry][unit]")
{
    vfs::c_node_registry registry;

    SECTION("Initial dirty flag is false")
    {
        auto node_id = storage::types::node_id_t{ 1 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 0 }
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());
        REQUIRE(result.value()->dirty == false);
    }

    SECTION("Dirty flag can be set and cleared")
    {
        auto node_id = storage::types::node_id_t{ 1 };
        vfs::s_node_metadata metadata{
            .node_id = node_id,
            .parent_id = storage::types::node_id_t{ 0 }
        };

        auto result = registry.get_or_create(metadata);
        REQUIRE(result.has_value());

        result.value()->dirty = true;
        REQUIRE(result.value()->dirty == true);

        result.value()->dirty = false;
        REQUIRE(result.value()->dirty == false);
    }
}

// ============================================================================
// Multi-threaded Tests
// ============================================================================

TEST_CASE("NodeRegistry - Concurrent reads", "[node_registry][multithread]")
{
    vfs::c_node_registry registry;

    // Pre-populate registry
    for (std::uint64_t i = 0; i < 100; ++i)
    {
        vfs::s_node_metadata metadata{
            .node_id = storage::types::node_id_t{ i },
            .parent_id = storage::types::node_id_t{ i / 10 },
            .size = i * 100
        };
        REQUIRE(registry.get_or_create(metadata).has_value());
    }

    SECTION("Multiple threads reading same nodes")
    {
        constexpr int num_threads = 10;
        std::vector<std::jthread> threads;
        std::atomic<int> successful_reads{ 0 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&]() -> void
                                 {
                for (std::uint64_t i = 0; i < 100; ++i)
                {
                    auto result = registry.get(storage::types::node_id_t{ i });
                    if (result.has_value() && result.value() && result.value()->metadata.size == i * 100)
                    {
                        successful_reads.fetch_add(1, std::memory_order_relaxed);
                    }
                } });
        }

        threads.clear();
        REQUIRE(successful_reads.load() == num_threads * 100);
    }

    SECTION("Multiple threads reading different nodes")
    {
        constexpr int num_threads = 8;
        std::vector<std::jthread> threads;
        std::atomic<int> failed_reads{ 0 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&, thread_id = t]() -> void
                                 {
                for (int i = 0; i < 100; ++i)
                {
                    std::uint64_t node_idx = (thread_id * 10 + i) % 100;
                    auto result = registry.get(storage::types::node_id_t{ node_idx });
                    if (!result.has_value() || !result.value())
                    {
                        failed_reads.fetch_add(1, std::memory_order_relaxed);
                    }
                } });
        }

        threads.clear();
        REQUIRE(failed_reads.load() == 0);
    }
}

TEST_CASE("NodeRegistry - Concurrent writes", "[node_registry][multithread]")
{
    vfs::c_node_registry registry;

    SECTION("Multiple threads creating different nodes")
    {
        constexpr int num_threads = 10;
        constexpr int nodes_per_thread = 50;
        std::vector<std::jthread> threads;

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&, thread_id = t]() -> void
                                 {
                for (int i = 0; i < nodes_per_thread; ++i)
                {
                    std::uint64_t node_idx = (thread_id * nodes_per_thread) + i;
                    vfs::s_node_metadata metadata{
                        .node_id = storage::types::node_id_t{ node_idx },
                        .parent_id = storage::types::node_id_t{ 0 },
                        .size = node_idx
                    };
                    registry.get_or_create(metadata);
                } });
        }

        threads.clear();

        // Verify all nodes exist
        for (int t = 0; t < num_threads; ++t)
        {
            for (int i = 0; i < nodes_per_thread; ++i)
            {
                std::uint64_t node_idx = (t * nodes_per_thread) + i;
                auto result = registry.get(storage::types::node_id_t{ node_idx });
                REQUIRE(result.has_value());
                REQUIRE(result.value() != nullptr);
                REQUIRE(result.value()->metadata.size == node_idx);
            }
        }
    }

    SECTION("Multiple threads trying to create same node (race)")
    {
        constexpr int num_threads = 20;
        std::vector<std::jthread> threads;
        auto shared_node_id = storage::types::node_id_t{ 12345 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&, thread_id = t]() -> void
                                 {
                vfs::s_node_metadata metadata{
                    .node_id = shared_node_id,
                    .parent_id = storage::types::node_id_t{ static_cast<std::uint64_t>(thread_id) },
                    .size = static_cast<std::uint64_t>(thread_id * 100)
                };
                registry.get_or_create(metadata); });
        }

        threads.clear();

        // The node should exist (first writer wins)
        auto result = registry.get(shared_node_id);
        REQUIRE(result.has_value());
        REQUIRE(result.value() != nullptr);
    }
}

TEST_CASE("NodeRegistry - Concurrent mixed operations", "[node_registry][multithread]")
{
    vfs::c_node_registry registry;

    SECTION("Simultaneous reads, writes, and removals")
    {
        constexpr int num_reader_threads = 5;
        constexpr int num_writer_threads = 5;
        constexpr int num_deleter_threads = 2;
        std::vector<std::jthread> threads;

        // Pre-populate some nodes
        for (std::uint64_t i = 0; i < 200; ++i)
        {
            vfs::s_node_metadata metadata{
                .node_id = storage::types::node_id_t{ i },
                .parent_id = storage::types::node_id_t{ 0 }
            };
            registry.get_or_create(metadata);
        }

        threads.reserve(num_reader_threads + num_writer_threads + num_deleter_threads);

        // Reader threads
        for (int t = 0; t < num_reader_threads; ++t)
        {
            threads.emplace_back([&]() -> void
                                 {
                for (int i = 0; i < 500; ++i)
                {
                    auto result = registry.get(storage::types::node_id_t{ static_cast<std::uint64_t>(i % 200) });
                    // Just reading, result may or may not be valid due to deletions
                } });
        }

        // Writer threads
        for (int t = 0; t < num_writer_threads; ++t)
        {
            threads.emplace_back([&, thread_id = t]() -> void
                                 {
                for (int i = 0; i < 100; ++i)
                {
                    std::uint64_t node_idx = 200 + (thread_id * 100) + i;
                    vfs::s_node_metadata metadata{
                        .node_id = storage::types::node_id_t{ node_idx },
                        .parent_id = storage::types::node_id_t{ 0 }
                    };
                    registry.get_or_create(metadata);
                } });
        }

        // Deleter threads
        for (int t = 0; t < num_deleter_threads; ++t)
        {
            threads.emplace_back([&]() -> void
                                 {
                for (int i = 0; i < 50; ++i)
                {
                    registry.remove(storage::types::node_id_t{ static_cast<std::uint64_t>(i * 2) });
                    std::this_thread::sleep_for(1ms);
                } });
        }

        threads.clear();
        // Test completes without deadlock or crash
        REQUIRE(true);
    }
}

TEST_CASE("NodeRegistry - Concurrent metadata modifications", "[node_registry][multithread]")
{
    vfs::c_node_registry registry;
    auto node_id = storage::types::node_id_t{ 1 };

    vfs::s_node_metadata metadata{
        .node_id = node_id,
        .parent_id = storage::types::node_id_t{ 0 }
    };
    auto record = registry.get_or_create(metadata);
    REQUIRE(record.has_value());

    SECTION("Multiple threads modifying generation and dirty flags")
    {
        constexpr int num_threads = 10;
        std::vector<std::jthread> threads;

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&]() -> void
                                 {
                for (int i = 0; i < 1000; ++i)
                {
                    std::scoped_lock lock(record.value()->mutex);
                    record.value()->generation++;
                    record.value()->dirty = !record.value()->dirty;
                } });
        }

        threads.clear();

        // Generation should have been incremented 10000 times
        REQUIRE(record.value()->generation == 10000);
    }
}
