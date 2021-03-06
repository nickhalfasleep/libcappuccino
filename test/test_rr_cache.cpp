#include "catch.hpp"
#include <cappuccino/cappuccino.hpp>

using namespace cappuccino;

TEST_CASE("RR example")
{
    // Create a cache with 2 items.
    rr_cache<uint64_t, std::string> cache{2};

    // Insert hello and world.
    cache.insert(1, "Hello");
    cache.insert(2, "World");

    {
        // Grab them
        auto hello = cache.find(1);
        auto world = cache.find(2);

        REQUIRE(hello.has_value());
        REQUIRE(world.has_value());

        REQUIRE(hello.value() == "Hello");
        REQUIRE(world.value() == "World");
    }

    // Insert hola, this will replace "Hello" or "World", we don't know!
    cache.insert(3, "Hola");

    {
        auto hola  = cache.find(3); // This will be in the cache.
        auto hello = cache.find(1); // This might be in the cache?
        auto world = cache.find(2); // This might be in the cache?

        REQUIRE(hola.has_value());
        REQUIRE(hola.value() == "Hola");

        size_t count{0};
        if (hello.has_value())
        {
            ++count;
        }
        if (world.has_value())
        {
            ++count;
        }

        // One of hello or world will be randomly evicted, which we don't know.
        REQUIRE(count == 1);
    }
}

TEST_CASE("Rr Find doesn't exist")
{
    rr_cache<uint64_t, std::string> cache{4};
    REQUIRE_FALSE(cache.find(100).has_value());
}

TEST_CASE("Rr Insert Only")
{
    rr_cache<uint64_t, std::string> cache{4};

    REQUIRE(cache.insert(1, "test", allow::insert));
    auto value = cache.find(1);
    REQUIRE(value.has_value());
    REQUIRE(value.value() == "test");

    REQUIRE_FALSE(cache.insert(1, "test2", allow::insert));
    value = cache.find(1);
    REQUIRE(value.has_value());
    REQUIRE(value.value() == "test");
}

TEST_CASE("Rr Update Only")
{
    rr_cache<uint64_t, std::string> cache{4};

    REQUIRE_FALSE(cache.insert(1, "test", allow::update));
    auto value = cache.find(1);
    REQUIRE_FALSE(value.has_value());
}

TEST_CASE("Rr Insert Or Update")
{
    rr_cache<uint64_t, std::string> cache{4};

    REQUIRE(cache.insert(1, "test"));
    auto value = cache.find(1);
    REQUIRE(value.has_value());
    REQUIRE(value.value() == "test");

    REQUIRE(cache.insert(1, "test2"));
    value = cache.find(1);
    REQUIRE(value.has_value());
    REQUIRE(value.value() == "test2");
}

TEST_CASE("Rr insert_range Insert Only")
{
    rr_cache<uint64_t, std::string> cache{4};

    {
        std::vector<std::pair<uint64_t, std::string>> inserts{{1, "test1"}, {2, "test2"}, {3, "test3"}};

        auto inserted = cache.insert_range(std::move(inserts), allow::insert);
        REQUIRE(inserted == 3);
    }

    REQUIRE(cache.size() == 3);

    REQUIRE(cache.find(2).has_value());
    REQUIRE(cache.find(2).value() == "test2");
    REQUIRE(cache.find(1).has_value());
    REQUIRE(cache.find(1).value() == "test1");
    REQUIRE(cache.find(3).has_value());
    REQUIRE(cache.find(3).value() == "test3");

    {
        std::vector<std::pair<uint64_t, std::string>> inserts{
            {1, "test1"},
            {2, "test2"},
            {3, "test3"},
            {4, "test4"}, // new
            {5, "test5"}, // new
        };

        auto inserted = cache.insert_range(std::move(inserts), allow::insert);
        REQUIRE(inserted == 2);
    }

    REQUIRE(cache.size() == 4);
    // Non deterministic which values are still in the cache, but should be size 4.

    size_t count{0};
    count += cache.find(1).has_value() ? 1 : 0;
    count += cache.find(2).has_value() ? 1 : 0;
    count += cache.find(3).has_value() ? 1 : 0;
    count += cache.find(4).has_value() ? 1 : 0;
    count += cache.find(5).has_value() ? 1 : 0;
    REQUIRE(count == 4);
}

TEST_CASE("Rr insert_range Update Only")
{
    rr_cache<uint64_t, std::string> cache{4};

    {
        std::vector<std::pair<uint64_t, std::string>> inserts{{1, "test1"}, {2, "test2"}, {3, "test3"}};

        auto inserted = cache.insert_range(std::move(inserts), allow::update);
        REQUIRE(inserted == 0);
    }

    REQUIRE(cache.size() == 0);
    REQUIRE_FALSE(cache.find(1).has_value());
    REQUIRE_FALSE(cache.find(2).has_value());
    REQUIRE_FALSE(cache.find(3).has_value());
}

TEST_CASE("Rr insert_range Insert Or Update")
{
    rr_cache<uint64_t, std::string> cache{4};

    {
        std::vector<std::pair<uint64_t, std::string>> inserts{{1, "test1"}, {2, "test2"}, {3, "test3"}};

        auto inserted = cache.insert_range(std::move(inserts));
        REQUIRE(inserted == 3);
    }

    REQUIRE(cache.size() == 3);
    REQUIRE(cache.find(1).has_value());
    REQUIRE(cache.find(1).value() == "test1");
    REQUIRE(cache.find(2).has_value());
    REQUIRE(cache.find(2).value() == "test2");
    REQUIRE(cache.find(3).has_value());
    REQUIRE(cache.find(3).value() == "test3");

    {
        std::vector<std::pair<uint64_t, std::string>> inserts{
            {2, "test2"}, // make 2 Rr
            {1, "test1"},
            {3, "test3"},
            {4, "test4"}, // new
            {5, "test5"}, // new
        };

        auto inserted = cache.insert_range(std::move(inserts));
        REQUIRE(inserted == 5);
    }

    REQUIRE(cache.size() == 4);
    // Non deterministic which values are still in the cache, but should be size 4.

    size_t count{0};
    count += cache.find(1).has_value() ? 1 : 0;
    count += cache.find(2).has_value() ? 1 : 0;
    count += cache.find(3).has_value() ? 1 : 0;
    count += cache.find(4).has_value() ? 1 : 0;
    count += cache.find(5).has_value() ? 1 : 0;
    REQUIRE(count == 4);
}

TEST_CASE("Rr erase")
{
    rr_cache<uint64_t, std::string> cache{4};

    REQUIRE(cache.insert(1, "test", allow::insert));
    auto value = cache.find(1);
    REQUIRE(value.has_value());
    REQUIRE(value.value() == "test");
    REQUIRE(cache.size() == 1);

    cache.erase(1);
    value = cache.find(1);
    REQUIRE_FALSE(value.has_value());
    REQUIRE(cache.size() == 0);
    REQUIRE(cache.empty());

    REQUIRE_FALSE(cache.erase(200));
}

TEST_CASE("Rr erase_range")
{
    rr_cache<uint64_t, std::string> cache{4};

    {
        std::vector<std::pair<uint64_t, std::string>> inserts{{1, "test1"}, {2, "test2"}, {3, "test3"}};

        auto inserted = cache.insert_range(std::move(inserts));
        REQUIRE(inserted == 3);
    }

    REQUIRE(cache.size() == 3);
    REQUIRE(cache.find(1).has_value());
    REQUIRE(cache.find(2).has_value());
    REQUIRE(cache.find(3).has_value());

    {
        std::vector<uint64_t> delete_keys{1, 3, 4, 5};

        auto deleted = cache.erase_range(delete_keys);
        REQUIRE(deleted == 2);
    }

    REQUIRE(cache.size() == 1);
    REQUIRE_FALSE(cache.find(1).has_value());
    REQUIRE(cache.find(2).has_value());
    REQUIRE(cache.find(2).value() == "test2");
    REQUIRE_FALSE(cache.find(3).has_value());
    REQUIRE_FALSE(cache.find(4).has_value());
    REQUIRE_FALSE(cache.find(5).has_value());
}

TEST_CASE("Rr find_range")
{
    rr_cache<uint64_t, std::string> cache{4};

    {
        std::vector<std::pair<uint64_t, std::string>> inserts{{1, "test1"}, {2, "test2"}, {3, "test3"}};

        auto inserted = cache.insert_range(std::move(inserts));
        REQUIRE(inserted == 3);
    }

    // Make sure all inserted keys exists via find range.
    {
        std::vector<uint64_t> keys{1, 2, 3};
        auto                  items = cache.find_range(keys);

        REQUIRE(items[0].first == 1);
        REQUIRE(items[0].second.has_value());
        REQUIRE(items[0].second.value() == "test1");
        REQUIRE(items[1].first == 2);
        REQUIRE(items[1].second.has_value());
        REQUIRE(items[1].second.value() == "test2");
        REQUIRE(items[2].first == 3);
        REQUIRE(items[2].second.has_value());
        REQUIRE(items[2].second.value() == "test3");
    }

    // Make sure keys not inserted are not found by find range.
    {
        std::vector<uint64_t> keys{1, 3, 4, 5};
        auto                  items = cache.find_range(keys);

        REQUIRE(items[0].first == 1);
        REQUIRE(items[0].second.has_value());
        REQUIRE(items[0].second.value() == "test1");
        REQUIRE(items[1].first == 3);
        REQUIRE(items[1].second.has_value());
        REQUIRE(items[1].second.value() == "test3");
        REQUIRE(items[2].first == 4);
        REQUIRE_FALSE(items[2].second.has_value());
        REQUIRE(items[3].first == 5);
        REQUIRE_FALSE(items[3].second.has_value());
    }
}

TEST_CASE("Rr find_range_fill")
{
    rr_cache<uint64_t, std::string> cache{4};

    {
        std::vector<std::pair<uint64_t, std::string>> inserts{{1, "test1"}, {2, "test2"}, {3, "test3"}};

        auto inserted = cache.insert_range(std::move(inserts));
        REQUIRE(inserted == 3);
    }

    // Make sure all inserted keys exists via find range.
    {
        std::vector<std::pair<uint64_t, std::optional<std::string>>> items{
            {1, std::nullopt},
            {2, std::nullopt},
            {3, std::nullopt},
        };
        cache.find_range_fill(items);

        REQUIRE(items[0].first == 1);
        REQUIRE(items[0].second.has_value());
        REQUIRE(items[0].second.value() == "test1");
        REQUIRE(items[1].first == 2);
        REQUIRE(items[1].second.has_value());
        REQUIRE(items[1].second.value() == "test2");
        REQUIRE(items[2].first == 3);
        REQUIRE(items[2].second.has_value());
        REQUIRE(items[2].second.value() == "test3");
    }

    // Make sure keys not inserted are not found by find range.
    {
        std::vector<std::pair<uint64_t, std::optional<std::string>>> items{
            {1, std::nullopt},
            {3, std::nullopt},
            {4, std::nullopt},
            {5, std::nullopt},
        };
        cache.find_range_fill(items);

        REQUIRE(items[0].first == 1);
        REQUIRE(items[0].second.has_value());
        REQUIRE(items[0].second.value() == "test1");
        REQUIRE(items[1].first == 3);
        REQUIRE(items[1].second.has_value());
        REQUIRE(items[1].second.value() == "test3");
        REQUIRE(items[2].first == 4);
        REQUIRE_FALSE(items[2].second.has_value());
        REQUIRE(items[3].first == 5);
        REQUIRE_FALSE(items[3].second.has_value());
    }
}

TEST_CASE("Rr empty")
{
    rr_cache<uint64_t, std::string> cache{4};

    REQUIRE(cache.empty());
    REQUIRE(cache.insert(1, "test", allow::insert));
    REQUIRE_FALSE(cache.empty());
    REQUIRE(cache.erase(1));
    REQUIRE(cache.empty());
}

TEST_CASE("Rr size + capacity")
{
    rr_cache<uint64_t, std::string> cache{4};

    REQUIRE(cache.capacity() == 4);

    REQUIRE(cache.insert(1, "test1"));
    REQUIRE(cache.size() == 1);

    REQUIRE(cache.insert(2, "test2"));
    REQUIRE(cache.size() == 2);

    REQUIRE(cache.insert(3, "test3"));
    REQUIRE(cache.size() == 3);

    REQUIRE(cache.insert(4, "test4"));
    REQUIRE(cache.size() == 4);

    REQUIRE(cache.insert(5, "test5"));
    REQUIRE(cache.size() == 4);

    REQUIRE(cache.insert(6, "test6"));
    REQUIRE(cache.size() == 4);

    REQUIRE(cache.capacity() == 4);
}