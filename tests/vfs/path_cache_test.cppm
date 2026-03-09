module;
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <atomic>
#include <barrier>
#include <latch>
#include <random>
#include <thread>

export module vfs:path_cache_test;

import :path_cache;
import storage;
import core;

using namespace std::literals::chrono_literals;

// ============================================================================
// Unit Tests - Basic Operations
// ============================================================================

TEST_CASE("PathCache - Basic put and get operations", "[path_cache][unit]")
{
    vfs::c_path_cache cache;
    auto parent_id = storage::types::node_id_t{ 1 };
    auto node_id = storage::types::node_id_t{ 42 };
    std::string name = "test_file.txt";

    SECTION("Put positive entry and retrieve it")
    {
        auto status = cache.put(parent_id, name, node_id, vfs::c_path_cache::e_put_type::positive);
        REQUIRE(status);

        auto result = cache.get(parent_id, name);
        REQUIRE(result.has_value());
        REQUIRE(result.value().has_value());

        auto entry = result.value().value();
        REQUIRE(entry.node_id.has_value());
        REQUIRE(entry.node_id.value() == node_id);
    }

    SECTION("Put negative entry")
    {
        auto status = cache.put(parent_id, name, std::nullopt, vfs::c_path_cache::e_put_type::negative);
        REQUIRE(status);

        auto result = cache.get(parent_id, name);
        REQUIRE(result.has_value());
        REQUIRE(result.value().has_value());

        auto entry = result.value().value();
        REQUIRE_FALSE(entry.node_id.has_value());
    }

    SECTION("Get non-existent entry returns nullopt")
    {
        auto result = cache.get(parent_id, "nonexistent.txt");
        REQUIRE(result.has_value());
        REQUIRE_FALSE(result.value().has_value());
    }

    SECTION("Put with custom TTL")
    {
        auto status = cache.put(parent_id, name, node_id, 100ms);
        REQUIRE(status);

        auto result = cache.get(parent_id, name);
        REQUIRE(result.has_value());
        REQUIRE(result.value().has_value());
    }

    SECTION("Multiple different entries")
    {
        REQUIRE(cache.put(parent_id, "file1.txt", storage::types::node_id_t{ 100 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(parent_id, "file2.txt", storage::types::node_id_t{ 200 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(parent_id, "file3.txt", storage::types::node_id_t{ 300 }, vfs::c_path_cache::e_put_type::positive));

        auto result1 = cache.get(parent_id, "file1.txt");
        auto result2 = cache.get(parent_id, "file2.txt");
        auto result3 = cache.get(parent_id, "file3.txt");

        REQUIRE(result1.value().value().node_id.value() == storage::types::node_id_t{ 100 });
        REQUIRE(result2.value().value().node_id.value() == storage::types::node_id_t{ 200 });
        REQUIRE(result3.value().value().node_id.value() == storage::types::node_id_t{ 300 });
    }

    SECTION("Same name in different parents")
    {
        auto parent1 = storage::types::node_id_t{ 1 };
        auto parent2 = storage::types::node_id_t{ 2 };
        std::string common_name = "common.txt";

        REQUIRE(cache.put(parent1, common_name, storage::types::node_id_t{ 100 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(parent2, common_name, storage::types::node_id_t{ 200 }, vfs::c_path_cache::e_put_type::positive));

        auto result1 = cache.get(parent1, common_name);
        auto result2 = cache.get(parent2, common_name);

        REQUIRE(result1.value().value().node_id.value() == storage::types::node_id_t{ 100 });
        REQUIRE(result2.value().value().node_id.value() == storage::types::node_id_t{ 200 });
    }
}

TEST_CASE("PathCache - TTL expiration", "[path_cache][unit][ttl]")
{
    vfs::c_path_cache cache;
    auto parent_id = storage::types::node_id_t{ 1 };
    auto node_id = storage::types::node_id_t{ 42 };
    std::string name = "expiring_file.txt";

    SECTION("Entry expires after TTL")
    {
        REQUIRE(cache.put(parent_id, name, node_id, 50ms));

        // Should be available immediately
        auto result1 = cache.get(parent_id, name);
        REQUIRE(result1.has_value());
        REQUIRE(result1.value().has_value());

        // Wait for expiration
        std::this_thread::sleep_for(100ms);

        // Should be expired
        auto result2 = cache.get(parent_id, name);
        REQUIRE(result2.has_value());
        REQUIRE_FALSE(result2.value().has_value());
    }

    SECTION("Negative cache expires faster than positive")
    {
        auto positive_parent = storage::types::node_id_t{ 1 };
        auto negative_parent = storage::types::node_id_t{ 2 };

        REQUIRE(cache.put(positive_parent, "pos.txt", storage::types::node_id_t{ 1 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(negative_parent, "neg.txt", std::nullopt, vfs::c_path_cache::e_put_type::negative));

        std::this_thread::sleep_for(250ms);

        auto pos_result = cache.get(positive_parent, "pos.txt");
        auto neg_result = cache.get(negative_parent, "neg.txt");

        // Negative should be expired (200ms), positive should still exist (500ms)
        REQUIRE(pos_result.value().has_value());
        REQUIRE_FALSE(neg_result.value().has_value());
    }

    SECTION("Updating entry refreshes TTL")
    {
        REQUIRE(cache.put(parent_id, name, node_id, 100ms));
        std::this_thread::sleep_for(60ms);

        // Update the entry
        REQUIRE(cache.put(parent_id, name, storage::types::node_id_t{ 99 }, 100ms));
        std::this_thread::sleep_for(60ms);

        // Should still be available with new value
        auto result = cache.get(parent_id, name);
        REQUIRE(result.has_value());
        REQUIRE(result.value().has_value());
        REQUIRE(result.value().value().node_id.value() == storage::types::node_id_t{ 99 });
    }
}

TEST_CASE("PathCache - Invalidation operations", "[path_cache][unit][invalidation]")
{
    vfs::c_path_cache cache;

    SECTION("Invalidate single entry")
    {
        auto parent_id = storage::types::node_id_t{ 1 };
        REQUIRE(cache.put(parent_id, "file1.txt", storage::types::node_id_t{ 100 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(parent_id, "file2.txt", storage::types::node_id_t{ 200 }, vfs::c_path_cache::e_put_type::positive));

        auto status = cache.invalidate(parent_id, "file1.txt");
        REQUIRE(status);

        auto result1 = cache.get(parent_id, "file1.txt");
        auto result2 = cache.get(parent_id, "file2.txt");

        REQUIRE_FALSE(result1.value().has_value());
        REQUIRE(result2.value().has_value());
    }

    SECTION("Invalidate all children of a parent")
    {
        auto parent_id = storage::types::node_id_t{ 1 };
        auto other_parent = storage::types::node_id_t{ 2 };

        REQUIRE(cache.put(parent_id, "child1.txt", storage::types::node_id_t{ 100 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(parent_id, "child2.txt", storage::types::node_id_t{ 101 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(parent_id, "child3.txt", storage::types::node_id_t{ 102 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(other_parent, "other.txt", storage::types::node_id_t{ 200 }, vfs::c_path_cache::e_put_type::positive));

        auto status = cache.invalidate_children(parent_id);
        REQUIRE(status);

        REQUIRE_FALSE(cache.get(parent_id, "child1.txt").value().has_value());
        REQUIRE_FALSE(cache.get(parent_id, "child2.txt").value().has_value());
        REQUIRE_FALSE(cache.get(parent_id, "child3.txt").value().has_value());
        REQUIRE(cache.get(other_parent, "other.txt").value().has_value());
    }

    SECTION("Invalidate non-existent entry")
    {
        auto parent_id = storage::types::node_id_t{ 1 };
        auto status = cache.invalidate(parent_id, "nonexistent.txt");
        REQUIRE(status);
    }

    SECTION("Clear entire cache")
    {
        auto parent1 = storage::types::node_id_t{ 1 };
        auto parent2 = storage::types::node_id_t{ 2 };

        REQUIRE(cache.put(parent1, "file1.txt", storage::types::node_id_t{ 100 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(parent2, "file2.txt", storage::types::node_id_t{ 200 }, vfs::c_path_cache::e_put_type::positive));

        auto status = cache.clear();
        REQUIRE(status);

        REQUIRE_FALSE(cache.get(parent1, "file1.txt").value().has_value());
        REQUIRE_FALSE(cache.get(parent2, "file2.txt").value().has_value());
    }
}

// ============================================================================
// Multi-threaded Tests
// ============================================================================

TEST_CASE("PathCache - Concurrent reads", "[path_cache][multithread][concurrent]")
{
    vfs::c_path_cache cache;
    auto parent_id = storage::types::node_id_t{ 1 };

    // Populate cache
    for (int i = 0; i < 100; ++i)
    {
        REQUIRE(cache.put(parent_id, "file_" + std::to_string(i) + ".txt", storage::types::node_id_t{ static_cast<std::uint64_t>(i) }, vfs::c_path_cache::e_put_type::positive));
    }

    SECTION("Multiple threads reading same entries")
    {
        constexpr int num_threads = 10;
        constexpr int reads_per_thread = 1000;
        std::vector<std::jthread> threads;
        std::atomic<int> successful_reads{ 0 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&cache, parent_id, &successful_reads]() -> void
                                 {
                for (int i = 0; i < reads_per_thread; ++i)
                {
                    auto result = cache.get(parent_id, "file_50.txt");
                    if (result.has_value() && result.value().has_value())
                    {
                        successful_reads.fetch_add(1, std::memory_order_relaxed);
                    }
                } });
        }

        threads.clear(); // Wait for all threads
        REQUIRE(successful_reads.load() == num_threads * reads_per_thread);
    }

    SECTION("Multiple threads reading different entries")
    {
        constexpr int num_threads = 8;
        std::vector<std::jthread> threads;
        std::atomic<int> failed_reads{ 0 };

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&cache, parent_id, t, &failed_reads]() -> void
                                 {
                for (int i = 0; i < 100; ++i)
                {
                    int file_idx = (t * 100 + i) % 100;
                    auto result = cache.get(parent_id, "file_" + std::to_string(file_idx) + ".txt");
                    if (!result.has_value() || !result.value().has_value())
                    {
                        failed_reads.fetch_add(1, std::memory_order_relaxed);
                    }
                } });
        }

        threads.clear();
        REQUIRE(failed_reads.load() == 0);
    }
}

TEST_CASE("PathCache - Concurrent writes", "[path_cache][multithread][concurrent]")
{
    vfs::c_path_cache cache;
    auto parent_id = storage::types::node_id_t{ 1 };

    SECTION("Multiple threads writing different entries")
    {
        constexpr int num_threads = 10;
        constexpr int writes_per_thread = 100;
        std::vector<std::jthread> threads;

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&cache, parent_id, t]() -> void
                                 {
                for (int i = 0; i < writes_per_thread; ++i)
                {
                    std::string name = "file_" + std::to_string(t) + "_" + std::to_string(i) + ".txt";
REQUIRE(                    cache.put(parent_id, name, storage::types::node_id_t{static_cast<std::uint64_t>((t * 1000) + i)}, vfs::c_path_cache::e_put_type::positive));
                } });
        }

        threads.clear();

        // Verify all entries are accessible
        for (int t = 0; t < num_threads; ++t)
        {
            for (int i = 0; i < writes_per_thread; ++i)
            {
                std::string name = "file_" + std::to_string(t) + "_" + std::to_string(i) + ".txt";
                auto result = cache.get(parent_id, name);
                REQUIRE(result.has_value());
                REQUIRE(result.value().has_value());
            }
        }
    }

    SECTION("Multiple threads updating same entry")
    {
        constexpr int num_threads = 20;
        std::vector<std::jthread> threads;
        std::string shared_name = "shared_file.txt";

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&cache, parent_id, shared_name, t]() -> void
                                 {
                for (int i = 0; i < 50; ++i)
                {
REQUIRE(                    cache.put(parent_id, shared_name, storage::types::node_id_t{static_cast<std::uint64_t>((t * 100) + i)}, vfs::c_path_cache::e_put_type::positive));
                } });
        }

        threads.clear();

        // The entry should exist (last write wins)
        auto result = cache.get(parent_id, shared_name);
        REQUIRE(result.has_value());
        REQUIRE(result.value().has_value());
    }
}

TEST_CASE("PathCache - Concurrent mixed operations", "[path_cache][multithread][concurrent]")
{
    vfs::c_path_cache cache;
    auto parent_id = storage::types::node_id_t{ 1 };

    SECTION("Simultaneous reads, writes, and invalidations")
    {
        constexpr int num_reader_threads = 5;
        constexpr int num_writer_threads = 3;
        constexpr int num_invalidator_threads = 2;
        std::vector<std::jthread> threads;
        std::atomic<bool> stop_flag{ false };

        // Populate initial data
        for (int i = 0; i < 50; ++i)
        {
            REQUIRE(cache.put(parent_id, "file_" + std::to_string(i) + ".txt", storage::types::node_id_t{ static_cast<std::uint64_t>(i) }, vfs::c_path_cache::e_put_type::positive));
        }

        // Reader threads
        threads.reserve(num_reader_threads);
        for (int t = 0; t < num_reader_threads; ++t)
        {
            threads.emplace_back([&cache, parent_id, &stop_flag]() -> void
                                 {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, 49);
                
                while (!stop_flag.load(std::memory_order_relaxed))
                {
                    int idx = dist(rng);
                    cache.get(parent_id, "file_" + std::to_string(idx) + ".txt");
                } });
        }

        // Writer threads
        for (int t = 0; t < num_writer_threads; ++t)
        {
            threads.emplace_back([&cache, parent_id, &stop_flag, t]() -> void
                                 {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, 49);
                
                while (!stop_flag.load(std::memory_order_relaxed))
                {
                    int idx = dist(rng);
                    REQUIRE(cache.put(parent_id, "file_" + std::to_string(idx) + ".txt", storage::types::node_id_t{static_cast<std::uint64_t>((t * 100) + idx)}, vfs::c_path_cache::e_put_type::positive));
                } });
        }

        // Invalidator threads
        for (int t = 0; t < num_invalidator_threads; ++t)
        {
            threads.emplace_back([&cache, parent_id, &stop_flag]() -> void
                                 {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, 49);
                
                while (!stop_flag.load(std::memory_order_relaxed))
                {
                    int idx = dist(rng);
                    cache.invalidate(parent_id, "file_" + std::to_string(idx) + ".txt");
                } });
        }

        std::this_thread::sleep_for(200ms);
        stop_flag.store(true, std::memory_order_relaxed);
        threads.clear();

        // If we reach here without crashes, the test passes
        REQUIRE(true);
    }
}

TEST_CASE("PathCache - Race conditions with expiration", "[path_cache][multithread][ttl]")
{
    vfs::c_path_cache cache;
    auto parent_id = storage::types::node_id_t{ 1 };

    SECTION("Read during expiration cleanup")
    {
        constexpr int num_threads = 10;
        std::vector<std::jthread> threads;
        std::atomic<int> read_count{ 0 };

        // Insert entries with short TTL
        for (int i = 0; i < 20; ++i)
        {
            REQUIRE(cache.put(parent_id, "file_" + std::to_string(i) + ".txt", storage::types::node_id_t{ static_cast<std::uint64_t>(i) }, 50ms));
        }

        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&cache, parent_id, &read_count]() -> void
                                 {
                for (int i = 0; i < 100; ++i)
                {
                    auto result = cache.get(parent_id, "file_10.txt");
                    if (result.has_value())
                    {
                        read_count.fetch_add(1, std::memory_order_relaxed);
                    }
                    std::this_thread::sleep_for(1ms);
                } });
        }

        threads.clear();
        REQUIRE(read_count.load() > 0);
    }
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_CASE("PathCache - High volume operations", "[path_cache][stress]")
{
    vfs::c_path_cache cache;

    SECTION("Insert and retrieve large number of entries")
    {
        constexpr int num_entries = 10'000;
        auto parent_id = storage::types::node_id_t{ 1 };

        for (int i = 0; i < num_entries; ++i)
        {
            REQUIRE(cache.put(parent_id, "file_" + std::to_string(i) + ".txt", storage::types::node_id_t{ static_cast<std::uint64_t>(i) }, vfs::c_path_cache::e_put_type::positive));
        }

        int success_count = 0;
        for (int i = 0; i < num_entries; ++i)
        {
            auto result = cache.get(parent_id, "file_" + std::to_string(i) + ".txt");
            if (result.has_value() && result.value().has_value())
            {
                ++success_count;
            }
        }

        REQUIRE(success_count == num_entries);
    }

    SECTION("Rapid insertions and deletions")
    {
        auto parent_id = storage::types::node_id_t{ 1 };

        for (int cycle = 0; cycle < 100; ++cycle)
        {
            // Insert 100 entries
            for (int i = 0; i < 100; ++i)
            {
                REQUIRE(cache.put(parent_id, "file_" + std::to_string(i) + ".txt", storage::types::node_id_t{ static_cast<std::uint64_t>((cycle * 100) + i) }, vfs::c_path_cache::e_put_type::positive));
            }

            // Delete half
            for (int i = 0; i < 50; ++i)
            {
                cache.invalidate(parent_id, "file_" + std::to_string(i) + ".txt");
            }
        }

        // Check remaining entries
        int remaining = 0;
        for (int i = 0; i < 100; ++i)
        {
            auto result = cache.get(parent_id, "file_" + std::to_string(i) + ".txt");
            if (result.has_value() && result.value().has_value())
            {
                ++remaining;
            }
        }

        REQUIRE(remaining == 50);
    }
}

TEST_CASE("PathCache - Multi-threaded stress test", "[path_cache][stress][multithread]")
{
    vfs::c_path_cache cache;
    constexpr int num_threads = 16;
    constexpr int operations_per_thread = 1000;
    std::vector<std::jthread> threads;
    std::atomic<int> total_operations{ 0 };

    SECTION("High-concurrency mixed workload")
    {
        for (int t = 0; t < num_threads; ++t)
        {
            threads.emplace_back([&cache, t, &total_operations]() -> void
                                 {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> op_dist(0, 100);
                std::uniform_int_distribution<int> parent_dist(1, 10);
                std::uniform_int_distribution<int> file_dist(0, 99);

                for (int i = 0; i < operations_per_thread; ++i)
                {
                    auto parent_id = storage::types::node_id_t{static_cast<std::uint64_t>(parent_dist(rng))};
                    std::string name = "file_" + std::to_string(file_dist(rng)) + ".txt";
                    int op = op_dist(rng);

                    if (op < 60) // 60% reads
                    {
                        cache.get(parent_id, name);
                    }
                    else if (op < 90) // 30% writes
                    {
                        REQUIRE(cache.put(parent_id, name, storage::types::node_id_t{static_cast<std::uint64_t>((t * 10000) + i)}, vfs::c_path_cache::e_put_type::positive));
                    }
                    else if (op < 95) // 5% invalidate single
                    {
                        cache.invalidate(parent_id, name);
                    }
                    else // 5% invalidate children
                    {
                        cache.invalidate_children(parent_id);
                    }

                    total_operations.fetch_add(1, std::memory_order_relaxed);
                } });
        }

        threads.clear();
        REQUIRE(total_operations.load() == num_threads * operations_per_thread);
    }
}

TEST_CASE("PathCache - Memory pressure test", "[path_cache][stress][memory]")
{
    vfs::c_path_cache cache;

    SECTION("Large number of parents and children")
    {
        constexpr int num_parents = 100;
        constexpr int children_per_parent = 100;

        for (int p = 0; p < num_parents; ++p)
        {
            auto parent_id = storage::types::node_id_t{ static_cast<std::uint64_t>(p) };
            for (int c = 0; c < children_per_parent; ++c)
            {
                REQUIRE(cache.put(parent_id, "child_" + std::to_string(c) + ".txt", storage::types::node_id_t{ static_cast<std::uint64_t>((p * 1000) + c) }, vfs::c_path_cache::e_put_type::positive));
            }
        }

        // Verify random samples
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> parent_dist(0, num_parents - 1);
        std::uniform_int_distribution<int> child_dist(0, children_per_parent - 1);

        int verified = 0;
        for (int i = 0; i < 1000; ++i)
        {
            int p = parent_dist(rng);
            int c = child_dist(rng);
            auto parent_id = storage::types::node_id_t{ static_cast<std::uint64_t>(p) };
            auto result = cache.get(parent_id, "child_" + std::to_string(c) + ".txt");

            if (result.has_value() && result.value().has_value())
            {
                ++verified;
            }
        }

        REQUIRE(verified == 1000);
    }
}

// ============================================================================
// BDD-Style Scenario Tests
// ============================================================================

TEST_CASE("PathCache - File lookup scenario", "[path_cache][scenario][bdd]")
{
    GIVEN("An empty path cache")
    {
        vfs::c_path_cache cache;
        auto parent_id = storage::types::node_id_t{ 100 };
        std::string filename = "document.txt";
        auto node_id = storage::types::node_id_t{ 12345 };

        WHEN("A file lookup is cached as a positive hit")
        {
            REQUIRE(cache.put(parent_id, filename, node_id, vfs::c_path_cache::e_put_type::positive));

            THEN("The cached entry can be retrieved")
            {
                auto result = cache.get(parent_id, filename);
                REQUIRE(result.has_value());
                REQUIRE(result.value().has_value());
                REQUIRE(result.value().value().node_id.has_value());
                REQUIRE(result.value().value().node_id.value() == node_id);
            }

            AND_THEN("Other files are not affected")
            {
                auto other_result = cache.get(parent_id, "other.txt");
                REQUIRE(other_result.has_value());
                REQUIRE_FALSE(other_result.value().has_value());
            }
        }

        WHEN("A file lookup fails and is cached as negative")
        {
            REQUIRE(cache.put(parent_id, filename, std::nullopt, vfs::c_path_cache::e_put_type::negative));

            THEN("The negative cache entry can be retrieved")
            {
                auto result = cache.get(parent_id, filename);
                REQUIRE(result.has_value());
                REQUIRE(result.value().has_value());
                REQUIRE_FALSE(result.value().value().node_id.has_value());
            }
        }
    }
}

TEST_CASE("PathCache - File modification scenario", "[path_cache][scenario][bdd]")
{
    GIVEN("A cache with an existing file entry")
    {
        vfs::c_path_cache cache;
        auto parent_id = storage::types::node_id_t{ 1 };
        std::string filename = "config.json";
        auto original_node = storage::types::node_id_t{ 500 };

        REQUIRE(cache.put(parent_id, filename, original_node, vfs::c_path_cache::e_put_type::positive));

        WHEN("The file is modified and cache is updated")
        {
            auto new_node = storage::types::node_id_t{ 600 };
            REQUIRE(cache.put(parent_id, filename, new_node, vfs::c_path_cache::e_put_type::positive));

            THEN("The cache returns the updated node ID")
            {
                auto result = cache.get(parent_id, filename);
                REQUIRE(result.has_value());
                REQUIRE(result.value().has_value());
                REQUIRE(result.value().value().node_id.value() == new_node);
            }
        }

        WHEN("The file is deleted and cache is invalidated")
        {
            cache.invalidate(parent_id, filename);

            THEN("The cache no longer contains the entry")
            {
                auto result = cache.get(parent_id, filename);
                REQUIRE(result.has_value());
                REQUIRE_FALSE(result.value().has_value());
            }
        }
    }
}

TEST_CASE("PathCache - Directory listing scenario", "[path_cache][scenario][bdd]")
{
    GIVEN("A cache with multiple files in a directory")
    {
        vfs::c_path_cache cache;
        auto dir_id = storage::types::node_id_t{ 42 };

        REQUIRE(cache.put(dir_id, "file1.txt", storage::types::node_id_t{ 101 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(dir_id, "file2.txt", storage::types::node_id_t{ 102 }, vfs::c_path_cache::e_put_type::positive));
        REQUIRE(cache.put(dir_id, "file3.txt", storage::types::node_id_t{ 103 }, vfs::c_path_cache::e_put_type::positive));

        WHEN("The directory is modified")
        {
            cache.invalidate_children(dir_id);

            THEN("All child entries are removed from cache")
            {
                REQUIRE_FALSE(cache.get(dir_id, "file1.txt").value().has_value());
                REQUIRE_FALSE(cache.get(dir_id, "file2.txt").value().has_value());
                REQUIRE_FALSE(cache.get(dir_id, "file3.txt").value().has_value());
            }
        }

        WHEN("Only one file is removed")
        {
            cache.invalidate(dir_id, "file2.txt");

            THEN("Other files remain cached")
            {
                REQUIRE(cache.get(dir_id, "file1.txt").value().has_value());
                REQUIRE_FALSE(cache.get(dir_id, "file2.txt").value().has_value());
                REQUIRE(cache.get(dir_id, "file3.txt").value().has_value());
            }
        }
    }
}

TEST_CASE("PathCache - Cache expiration scenario", "[path_cache][scenario][bdd][ttl]")
{
    GIVEN("A cache with time-limited entries")
    {
        vfs::c_path_cache cache;
        auto parent_id = storage::types::node_id_t{ 1 };

        WHEN("A file is cached with a short TTL")
        {
            REQUIRE(cache.put(parent_id, "temporary.txt", storage::types::node_id_t{ 999 }, 100ms));

            THEN("It is available immediately")
            {
                auto result = cache.get(parent_id, "temporary.txt");
                REQUIRE(result.has_value());
                REQUIRE(result.value().has_value());
            }

            AND_WHEN("Time passes beyond TTL")
            {
                std::this_thread::sleep_for(150ms);

                THEN("The entry is no longer available")
                {
                    auto result = cache.get(parent_id, "temporary.txt");
                    REQUIRE(result.has_value());
                    REQUIRE_FALSE(result.value().has_value());
                }
            }
        }
    }
}

TEST_CASE("PathCache - Concurrent access scenario", "[path_cache][scenario][bdd][multithread]")
{
    GIVEN("A cache accessed by multiple threads")
    {
        vfs::c_path_cache cache;
        auto parent_id = storage::types::node_id_t{ 1 };
        std::string shared_file = "shared.txt";

        WHEN("Multiple threads read the same cached entry")
        {
            REQUIRE(cache.put(parent_id, shared_file, storage::types::node_id_t{ 777 }, vfs::c_path_cache::e_put_type::positive));

            constexpr int num_threads = 10;
            constexpr int reads_per_thread = 100;
            std::vector<std::jthread> threads;
            std::atomic<int> successful_reads{ 0 };

            threads.reserve(num_threads);
            for (int i = 0; i < num_threads; ++i)
            {
                threads.emplace_back([&]() -> void
                                     {
                    for (int j = 0; j < reads_per_thread; ++j)
                    {
                        auto result = cache.get(parent_id, shared_file);
                        if (result.has_value() && result.value().has_value())
                        {
                            successful_reads.fetch_add(1, std::memory_order_relaxed);
                        }
                    } });
            }

            threads.clear();

            THEN("All reads succeed without data corruption")
            {
                REQUIRE(successful_reads.load() == num_threads * reads_per_thread);
            }
        }

        WHEN("Multiple threads write to different entries")
        {
            constexpr int num_threads = 8;
            std::vector<std::jthread> threads;

            threads.reserve(num_threads);
            for (int i = 0; i < num_threads; ++i)
            {
                threads.emplace_back([&, i]() -> void
                                     {
                    std::string filename = "file_" + std::to_string(i) + ".txt";
                    cache.put(parent_id, filename, storage::types::node_id_t{static_cast<std::uint64_t>(i * 100)}, vfs::c_path_cache::e_put_type::positive); });
            }

            threads.clear();

            THEN("All entries are correctly stored")
            {
                for (int i = 0; i < num_threads; ++i)
                {
                    std::string filename = "file_" + std::to_string(i) + ".txt";
                    auto result = cache.get(parent_id, filename);
                    REQUIRE(result.has_value());
                    REQUIRE(result.value().has_value());
                    REQUIRE(result.value().value().node_id.value() == storage::types::node_id_t{ static_cast<std::uint64_t>(i * 100) });
                }
            }
        }
    }
}

TEST_CASE("PathCache - Cache coherency scenario", "[path_cache][scenario][bdd]")
{
    GIVEN("A hierarchical file system structure")
    {
        vfs::c_path_cache cache;
        auto root_id = storage::types::node_id_t{ 1 };
        auto dir1_id = storage::types::node_id_t{ 10 };
        auto dir2_id = storage::types::node_id_t{ 20 };

        // Setup: /dir1/file.txt and /dir2/file.txt
        cache.put(root_id, "dir1", dir1_id, vfs::c_path_cache::e_put_type::positive);
        cache.put(root_id, "dir2", dir2_id, vfs::c_path_cache::e_put_type::positive);
        cache.put(dir1_id, "file.txt", storage::types::node_id_t{ 100 }, vfs::c_path_cache::e_put_type::positive);
        cache.put(dir2_id, "file.txt", storage::types::node_id_t{ 200 }, vfs::c_path_cache::e_put_type::positive);

        WHEN("Files with same name in different directories are accessed")
        {
            auto result1 = cache.get(dir1_id, "file.txt");
            auto result2 = cache.get(dir2_id, "file.txt");

            THEN("Each returns the correct distinct node")
            {
                REQUIRE(result1.value().value().node_id.value() == storage::types::node_id_t{ 100 });
                REQUIRE(result2.value().value().node_id.value() == storage::types::node_id_t{ 200 });
            }
        }

        WHEN("One directory is cleared")
        {
            cache.invalidate_children(dir1_id);

            THEN("Only that directory's cache is affected")
            {
                REQUIRE_FALSE(cache.get(dir1_id, "file.txt").value().has_value());
                REQUIRE(cache.get(dir2_id, "file.txt").value().has_value());
                REQUIRE(cache.get(root_id, "dir1").value().has_value());
                REQUIRE(cache.get(root_id, "dir2").value().has_value());
            }
        }
    }
}

TEST_CASE("PathCache - Stress test with barrier synchronization", "[path_cache][stress][multithread]")
{
    GIVEN("Multiple threads ready to hammer the cache simultaneously")
    {
        vfs::c_path_cache cache;
        constexpr int num_threads = 12;
        std::barrier sync_point(num_threads);
        std::vector<std::jthread> threads;
        std::atomic<int> total_ops{ 0 };

        WHEN("All threads start at the same time")
        {
            for (int t = 0; t < num_threads; ++t)
            {
                threads.emplace_back([&, t]() -> void
                                     {
                    sync_point.arrive_and_wait(); // Synchronized start

                    auto parent_id = storage::types::node_id_t{static_cast<std::uint64_t>(t % 3)};
                    
                    for (int i = 0; i < 500; ++i)
                    {
                        std::string name = "file_" + std::to_string(i % 50) + ".txt";
                        
                        // Mix of operations
                        if (i % 3 == 0)
                        {
                            cache.put(parent_id, name,
                                     storage::types::node_id_t{static_cast<std::uint64_t>((t * 1000) + i)},
                                     vfs::c_path_cache::e_put_type::positive);
                        }
                        else if (i % 3 == 1)
                        {
                            cache.get(parent_id, name);
                        }
                        else
                        {
                            cache.invalidate(parent_id, name);
                        }
                        
                        total_ops.fetch_add(1, std::memory_order_relaxed);
                    } });
            }

            threads.clear();

            THEN("All operations complete without deadlock or corruption")
            {
                REQUIRE(total_ops.load() == num_threads * 500);
            }
        }
    }
}

TEST_CASE("PathCache - Edge cases", "[path_cache][edge_cases]")
{
    vfs::c_path_cache cache;

    SECTION("Very long file names")
    {
        auto parent_id = storage::types::node_id_t{ 1 };
        std::string long_name(1000, 'x');
        long_name += ".txt";

        cache.put(parent_id, long_name, storage::types::node_id_t{ 42 }, vfs::c_path_cache::e_put_type::positive);
        auto result = cache.get(parent_id, long_name);

        REQUIRE(result.has_value());
        REQUIRE(result.value().has_value());
    }

    SECTION("Special characters in file names")
    {
        auto parent_id = storage::types::node_id_t{ 1 };
        std::vector<std::string> special_names = {
            "file with spaces.txt",
            "file-with-dashes.txt",
            "file_with_underscores.txt",
            "file.multiple.dots.txt",
            "CaseSensitive.TXT"
        };

        for (const auto &name : special_names)
        {
            cache.put(parent_id, name, storage::types::node_id_t{ 1 }, vfs::c_path_cache::e_put_type::positive);
            auto result = cache.get(parent_id, name);
            REQUIRE(result.value().has_value());
        }
    }

    SECTION("Maximum node ID values")
    {
        auto parent_id = storage::types::node_id_t{ std::numeric_limits<std::uint64_t>::max() };
        auto node_id = storage::types::node_id_t{ std::numeric_limits<std::uint64_t>::max() };

        cache.put(parent_id, "test.txt", node_id, vfs::c_path_cache::e_put_type::positive);
        auto result = cache.get(parent_id, "test.txt");

        REQUIRE(result.has_value());
        REQUIRE(result.value().has_value());
        REQUIRE(result.value().value().node_id.value() == node_id);
    }

    SECTION("Rapid clear operations")
    {
        auto parent_id = storage::types::node_id_t{ 1 };

        for (int cycle = 0; cycle < 100; ++cycle)
        {
            cache.put(parent_id, "test.txt", storage::types::node_id_t{ static_cast<std::uint64_t>(cycle) },
                      vfs::c_path_cache::e_put_type::positive);
            cache.clear();
        }

        auto result = cache.get(parent_id, "test.txt");
        REQUIRE_FALSE(result.value().has_value());
    }
}

TEST_CASE("PathCache - Latch-based coordination test", "[path_cache][multithread][coordination]")
{
    GIVEN("A cache with threads coordinated by latches")
    {
        vfs::c_path_cache cache;
        constexpr int num_writers = 5;
        constexpr int num_readers = 10;
        std::latch start_latch(num_writers + num_readers);
        std::latch write_complete_latch(num_writers);
        std::vector<std::jthread> threads;
        auto parent_id = storage::types::node_id_t{ 1 };

        WHEN("Writers populate cache before readers access it")
        {
            // Writer threads
            for (int w = 0; w < num_writers; ++w)
            {
                threads.emplace_back([&, w]() -> void
                                     {
                    start_latch.arrive_and_wait();
                    
                    for (int i = 0; i < 20; ++i)
                    {
                        std::string name = "file_" + std::to_string((w * 20) + i) + ".txt";
                        cache.put(parent_id, name,
                                 storage::types::node_id_t{static_cast<std::uint64_t>((w * 20) + i)},
                                 vfs::c_path_cache::e_put_type::positive);
                    }
                    
                    write_complete_latch.count_down(); });
            }

            // Reader threads
            std::atomic<int> read_successes{ 0 };
            for (int r = 0; r < num_readers; ++r)
            {
                threads.emplace_back([&]() -> void
                                     {
                    start_latch.arrive_and_wait();
                    write_complete_latch.wait(); // Wait for all writes
                    
                    for (int i = 0; i < 100; ++i)
                    {
                        std::string name = "file_" + std::to_string(i) + ".txt";
                        auto result = cache.get(parent_id, name);
                        if (result.has_value() && result.value().has_value())
                        {
                            read_successes.fetch_add(1, std::memory_order_relaxed);
                        }
                    } });
            }

            threads.clear();

            THEN("All readers see the written data")
            {
                REQUIRE(read_successes.load() == num_readers * 100);
            }
        }
    }
}
