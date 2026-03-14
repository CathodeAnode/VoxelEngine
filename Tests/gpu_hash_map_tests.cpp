#include <gtest/gtest.h>
#include "screen.h"
#include "gpu_hash_maps.h"

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
