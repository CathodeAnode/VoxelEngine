#include <gtest/gtest.h>

#include <random>

#include "screen.h"
#include "gpu_hashmap_allocator.h"

class GPULockFreeHashMapTest : public ::testing::Test 
{
protected:
    static Screen* screen;

    static void SetUpTestSuite() 
    {
        screen = new Screen(800, 600, "Test");
        ASSERT_TRUE(screen->init()) << "Screen initialization failed";
    }

    static void TearDownTestSuite() 
    {
        if (screen) {
            screen->close();
            delete screen;
            screen = nullptr;
        }
    }
};

Screen* GPULockFreeHashMapTest::screen = nullptr;

TEST_F(GPULockFreeHashMapTest, CreateTable)
{
    GPULockFreeHashMap<int, int> map;
    EXPECT_TRUE(map.Create(128));
}

TEST_F(GPULockFreeHashMapTest, InsertAndFind)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(128));

    EXPECT_TRUE(map.Insert(1, 42));

    int value = 0;
    EXPECT_TRUE(map.Find(1, value));
    EXPECT_EQ(value, 42);
}

TEST_F(GPULockFreeHashMapTest, ContainsKey)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(128));

    EXPECT_FALSE(map.Contains(5));

    map.Insert(5, 99);

    EXPECT_TRUE(map.Contains(5));
}

TEST_F(GPULockFreeHashMapTest, FindNonExistentKey)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(64));

    int value;
    EXPECT_FALSE(map.Find(999, value));
}

TEST_F(GPULockFreeHashMapTest, EraseKey)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(128));

    map.Insert(7, 77);

    EXPECT_TRUE(map.Contains(7));

    EXPECT_TRUE(map.Erase(7));

    EXPECT_FALSE(map.Contains(7));

    int value;
    EXPECT_FALSE(map.Find(7, value));
}

TEST_F(GPULockFreeHashMapTest, InsertMultipleKeys)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(256));

    for (int i = 0; i < 100; ++i)
        EXPECT_TRUE(map.Insert(i, i * 10));

    for (int i = 0; i < 100; ++i)
    {
        int value = 0;
        EXPECT_TRUE(map.Find(i, value));
        EXPECT_EQ(value, i * 10);
    }
}

TEST_F(GPULockFreeHashMapTest, InsertEraseInsert)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(64));

    EXPECT_TRUE(map.Insert(1, 10));
    EXPECT_TRUE(map.Erase(1));
    EXPECT_TRUE(map.Insert(1, 20));

    int value;
    EXPECT_TRUE(map.Find(1, value));
    EXPECT_EQ(value, 20);
}

TEST_F(GPULockFreeHashMapTest, DuplicateInsert)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(128));

    EXPECT_TRUE(map.Insert(10, 1));

    EXPECT_FALSE(map.Insert(10, 2));

    int value = 0;
    EXPECT_TRUE(map.Find(10, value));

    EXPECT_EQ(value, 2);
}

TEST_F(GPULockFreeHashMapTest, MultiThreadedInsert)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(2048));

    const int threadCount = 8;
    const int insertsPerThread = 500;

    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        threads.emplace_back([&, t]()
            {
                for (int i = 0; i < insertsPerThread; ++i)
                {
                    int key = t * insertsPerThread + i;
                    map.Insert(key, key * 10);
                }
            });
    }

    for (auto& th : threads)
        th.join();

    for (int i = 0; i < threadCount * insertsPerThread; ++i)
    {
        int value = 0;
        EXPECT_TRUE(map.Find(i, value));
        EXPECT_EQ(value, i * 10);
    }
}

TEST_F(GPULockFreeHashMapTest, MultiThreadedInsertAndFind)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(4096));

    const int threadCount = 8;
    const int ops = 1000;

    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        threads.emplace_back([&, t]()
            {
                for (int i = 0; i < ops; ++i)
                {
                    int key = (t * ops) + i;
                    map.Insert(key, key);

                    int value;
                    map.Find(key, value);
                }
            });
    }

    for (auto& th : threads)
        th.join();

    for (int i = 0; i < threadCount * ops; ++i)
    {
        int value;
        EXPECT_TRUE(map.Find(i, value));
        EXPECT_EQ(value, i);
    }
}

TEST_F(GPULockFreeHashMapTest, MultiThreadedDuplicateInsert)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(512));

    const int threadCount = 16;

    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        threads.emplace_back([&, t]()
            {
                for (int i = 0; i < 100; ++i)
                {
                    map.Insert(42, t); // all threads target same key
                }
            });
    }

    for (auto& th : threads)
        th.join();

    int value;
    EXPECT_TRUE(map.Find(42, value));
}

TEST_F(GPULockFreeHashMapTest, HashCollisions)
{
    GPULockFreeHashMap<int, int> map;

    ASSERT_TRUE(map.Create(8)); // tiny table => heavy collisions

    const int count = 50;

    for (int i = 0; i < count; ++i)
        EXPECT_TRUE(map.Insert(i, i * 5));

    for (int i = 0; i < count; ++i)
    {
        int value = 0;
        EXPECT_TRUE(map.Find(i, value));
        EXPECT_EQ(value, i * 5);
    }
}

TEST_F(GPULockFreeHashMapTest, ForcedCollisionKeys)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(64));

    const int stride = 64;

    for (int i = 0; i < 50; ++i)
    {
        int key = i * stride; // hashes to same bucket
        EXPECT_TRUE(map.Insert(key, i));
    }

    for (int i = 0; i < 50; ++i)
    {
        int key = i * stride;
        int value = 0;

        EXPECT_TRUE(map.Find(key, value));
        EXPECT_EQ(value, i);
    }
}

TEST_F(GPULockFreeHashMapTest, StressTest)
{
    GPULockFreeHashMap<int, int> map;
    ASSERT_TRUE(map.Create(16384));

    const int threadCount = std::thread::hardware_concurrency();
    const int operations = 20000;
    const int keySpace = 2000;

    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        threads.emplace_back([&, t]()
            {
                std::mt19937 rng(t);
                std::uniform_int_distribution<int> dist(0, keySpace);

                for (int i = 0; i < operations; ++i)
                {
                    int key = dist(rng);

                    map.Insert(key, key * 2);

                    int value;
                    if (map.Find(key, value))
                        EXPECT_EQ(value, key * 2);

                    if (i % 5 == 0)
                        map.Erase(key);
                }
            });
    }

    for (auto& th : threads)
        th.join();

    // final consistency check
    for (int k = 0; k <= keySpace; ++k)
    {
        int value;
        if (map.Find(k, value))
            EXPECT_EQ(value, k * 2);
    }
}
