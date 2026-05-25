#include <gtest/gtest.h>

#include <random>

#include "screen.h"
#include "gpu_cache_allocator.h"
#include "gpu_cache_policy.h"

#include "Helpers/gpu_cache_inspector.h"

class GPUPagedCacheTests : public ::testing::Test
{
protected:
    static Screen* screen;

    using Cache = GPUPagedCache<uint32_t, int, FIFOPolicy>;
    using Inspector = GPUPagedCacheInspector<Cache>;

    static constexpr size_t PAGE_SIZE = 5;
    static constexpr uint32_t PAGE_COUNT = 10;

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

Screen* GPUPagedCacheTests::screen = nullptr;

TEST_F(GPUPagedCacheTests, CreateCache)
{
    Cache cache;
    const size_t pageSize = 5;
    const size_t pageCount = 10;
    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, pageSize, pageCount));
}

TEST_F(GPUPagedCacheTests, CreateWithZeroPageCount)
{
    Cache cache;
    const size_t pageSize = 5;
    const uint32_t pageCount = 0;

    ASSERT_FALSE(cache.Create(GL_SHADER_STORAGE_BUFFER, pageSize, pageCount));
    ASSERT_EQ(cache.GetName(), 0u);
}

TEST_F(GPUPagedCacheTests, CreateTwice)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 10));

    const GLuint firstName = cache.GetName();
    ASSERT_NE(firstName, 0u);

    ASSERT_FALSE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 10));

    // Ensure original allocation is still valid.
    ASSERT_EQ(cache.GetName(), firstName);
}

TEST_F(GPUPagedCacheTests, DestroyEmptyCache)
{
    Cache cache;

    ASSERT_NO_THROW(cache.Destroy());

    ASSERT_EQ(cache.GetName(), 0u);
}

TEST_F(GPUPagedCacheTests, DestroyPopulatedCache)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 10));

    constexpr uint32_t objectID = 1;
    const int data[] = { 1, 2, 3, 4, 5 };

    cache.AllocateObject(objectID, data, std::size(data));

    ASSERT_TRUE(cache.Has(objectID));
    ASSERT_NE(cache.GetName(), 0u);

    ASSERT_NO_THROW(cache.Destroy());

    ASSERT_EQ(cache.GetName(), 0u);
    ASSERT_DEBUG_DEATH(cache.Has(objectID), "table != nullptr");
}

TEST_F(GPUPagedCacheTests, DeallocateObjectBasic)
{
    Cache cache;

    ASSERT_TRUE(
        cache.Create(GL_SHADER_STORAGE_BUFFER, PAGE_SIZE, PAGE_COUNT)
    );

    constexpr uint32_t objectID = 1;
    std::vector<int> data = { 1, 2, 3, 4, 5};
    cache.AllocateObject(objectID, data.data(), data.size());

    ASSERT_TRUE(cache.Has(objectID));

    const unsigned int freePagesBefore = Inspector::GetFreePagesCount(cache);

    cache.DeallocateObject(objectID);
    EXPECT_FALSE(cache.Has(objectID));

    const unsigned int freePagesAfter = Inspector::GetFreePagesCount(cache);

    EXPECT_GT(freePagesAfter, freePagesBefore);

    auto readback = Inspector::ReadObjectData(cache, objectID);

    EXPECT_TRUE(readback.empty());

    cache.Destroy();
}

TEST_F(GPUPagedCacheTests, DeallocateObjectNonExistingObject)
{
    Cache cache;

    ASSERT_TRUE(
        cache.Create(GL_SHADER_STORAGE_BUFFER, PAGE_SIZE, PAGE_COUNT)
    );

    const unsigned int freePagesBefore = Inspector::GetFreePagesCount(cache);

    EXPECT_NO_THROW(cache.DeallocateObject(9999));

    const unsigned int freePagesAfter = Inspector::GetFreePagesCount(cache);

    EXPECT_EQ(freePagesBefore, freePagesAfter);

    cache.Destroy();
}


TEST_F(GPUPagedCacheTests, DeallocateObjectEmptyObject)
{
    Cache cache;

    ASSERT_TRUE(
        cache.Create(GL_SHADER_STORAGE_BUFFER, PAGE_SIZE, PAGE_COUNT)
    );

    constexpr uint32_t objectID = 7;

    cache.AllocateObject(objectID, nullptr, 0);

    ASSERT_TRUE(cache.Has(objectID));

    const unsigned int freePagesBefore = Inspector::GetFreePagesCount(cache);

    EXPECT_NO_THROW(cache.DeallocateObject(objectID));

    EXPECT_FALSE(cache.Has(objectID));

    const unsigned int freePagesAfter = Inspector::GetFreePagesCount(cache);

    EXPECT_GT(freePagesAfter, freePagesBefore);

    cache.Destroy();
}

TEST_F(GPUPagedCacheTests, DeallocateObjectMultipleTimes)
{
    Cache cache;

    ASSERT_TRUE(
        cache.Create(GL_SHADER_STORAGE_BUFFER, PAGE_SIZE, PAGE_COUNT)
    );

    constexpr uint32_t objectID = 42;

    std::vector<int> data = { 10, 20, 30, 40, 50 };

    cache.AllocateObject(objectID, data.data(), data.size());

    ASSERT_TRUE(cache.Has(objectID));

    cache.DeallocateObject(objectID);

    EXPECT_FALSE(cache.Has(objectID));

    const unsigned int freePagesAfterFirst =
        Inspector::GetFreePagesCount(cache);

    EXPECT_NO_THROW(cache.DeallocateObject(objectID));

    const unsigned int freePagesAfterSecond =
        Inspector::GetFreePagesCount(cache);

    EXPECT_EQ(freePagesAfterFirst, freePagesAfterSecond);

    cache.Destroy();
}

TEST_F(GPUPagedCacheTests, DeallocateObjectFragmentedAllocation)
{
    Cache cache;

    ASSERT_TRUE(
        cache.Create(GL_SHADER_STORAGE_BUFFER, PAGE_SIZE, PAGE_COUNT)
    );

    constexpr uint32_t objA = 1;
    constexpr uint32_t objB = 2;
    constexpr uint32_t objC = 3;

    std::vector<int> dataA = { 1, 2, 3, 4, 5 };
    std::vector<int> dataB = { 10, 11, 12, 13 };
    std::vector<int> dataC = { 20, 21, 22, 23 };

    cache.AllocateObject(objA, dataA.data(), dataA.size());
    cache.AllocateObject(objB, dataB.data(), dataB.size());
    cache.AllocateObject(objC, dataC.data(), dataC.size());

    ASSERT_TRUE(cache.Has(objA));
    ASSERT_TRUE(cache.Has(objB));
    ASSERT_TRUE(cache.Has(objC));

    // Create fragmentation
    cache.DeallocateObject(objB);

    EXPECT_FALSE(cache.Has(objB));
    EXPECT_TRUE(cache.Has(objA));
    EXPECT_TRUE(cache.Has(objC));

    auto readA = Inspector::ReadObjectData(cache, objA);
    auto readC = Inspector::ReadObjectData(cache, objC);

    EXPECT_EQ(readA, dataA);
    EXPECT_EQ(readC, dataC);

    // Ensure fragmented pages are reusable
    constexpr uint32_t objD = 4;

    std::vector<int> dataD = { 100, 101, 102, 103, 104, 105 };

    EXPECT_NO_THROW(
        cache.AllocateObject(objD, dataD.data(), dataD.size())
    );

    EXPECT_TRUE(cache.Has(objD));

    auto readD = Inspector::ReadObjectData(cache, objD);

    EXPECT_EQ(readD, dataD);

    // deallocate fragmentented object
    cache.DeallocateObject(objD);

    auto freePages = Inspector::GetFreePagesCount(cache);

    EXPECT_EQ(freePages, 8);

    cache.Destroy();
}

TEST_F(GPUPagedCacheTests, AllocateObjectSinglePage)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 10));

    constexpr uint32_t objectID = 1;

    const std::vector<int> data = { 1, 2, 3 };

    cache.AllocateObject(objectID, data.data(), data.size());

    ASSERT_TRUE(cache.Has(objectID));

    const auto ranges = cache.GetObjectBufferRanges(objectID);
    ASSERT_EQ(ranges.size(), 1u);

    std::vector<int> result = Inspector::ReadObjectData(cache, objectID);

    ASSERT_EQ(result.size(), data.size());

    for (size_t i = 0; i < data.size(); ++i)
    {
        ASSERT_EQ(result[i], data[i])
            << "Mismatch at index " << i;
    }
}

TEST_F(GPUPagedCacheTests, AllocateObjectExactPageBoundary)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 10));

    constexpr uint32_t objectID = 2;

    const std::vector<int> data = { 1, 2, 3, 4, 5 };

    cache.AllocateObject(objectID, data.data(), data.size());

    ASSERT_TRUE(cache.Has(objectID));

    const auto ranges = cache.GetObjectBufferRanges(objectID);
    ASSERT_EQ(ranges.size(), 1u);

    auto result = Inspector::ReadObjectData(cache, objectID);

    ASSERT_EQ(result, data);
}

TEST_F(GPUPagedCacheTests, AllocateObjectOverMultiplePagesContiguous)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 10));

    constexpr uint32_t objectID = 3;

    std::vector<int> data(12, 42);

    cache.AllocateObject(objectID, data.data(), data.size());

    ASSERT_TRUE(cache.Has(objectID));

    const auto ranges = cache.GetObjectBufferRanges(objectID);

    // 12 elements with page size 5 => 3 pages
    ASSERT_EQ(ranges.size(), 1u);
}

TEST_F(GPUPagedCacheTests, AllocateObjectOverMultiplePagesFragmented)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 6));

    // Create fragmentation.
    cache.AllocateObject(1, std::array<int, 5>{1, 2, 3, 4, 5}.data(), 5);
    cache.AllocateObject(2, std::array<int, 5>{1, 2, 3, 4, 5}.data(), 5);
    cache.AllocateObject(3, std::array<int, 5>{1, 2, 3, 4, 5}.data(), 5);

    cache.DeallocateObject(2);

    // Allocate object requiring multiple pages.
    std::vector<int> largeData(7, 7);

    cache.AllocateObject(4, largeData.data(), largeData.size());

    ASSERT_TRUE(cache.Has(4));

    const auto ranges = cache.GetObjectBufferRanges(4);

    ASSERT_EQ(ranges.size(), 2u);
}

TEST_F(GPUPagedCacheTests, AllocateObjectWithNoElements)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 10));

    constexpr uint32_t objectID = 5;

    cache.AllocateObject(objectID, nullptr, 0);

    ASSERT_TRUE(cache.Has(objectID));
}

TEST_F(GPUPagedCacheTests, AllocateObjectCausesSingleEviction)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 2));

    const int pageData[] = { 1, 2, 3, 4, 5 };

    cache.AllocateObject(1, pageData, 5);
    cache.AllocateObject(2, pageData, 5);

    ASSERT_TRUE(cache.Has(1));
    ASSERT_TRUE(cache.Has(2));

    // Requires eviction.
    cache.AllocateObject(3, pageData, 5);

    ASSERT_TRUE(cache.Has(3));

    const bool obj1Alive = cache.Has(1);
    const bool obj2Alive = cache.Has(2);

    // Exactly one prior object should have been evicted.
    ASSERT_NE(obj1Alive, obj2Alive);
}

TEST_F(GPUPagedCacheTests, AllocateObjectCausesMultipleEvictions)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 3));

    const int pageData[] = { 1, 2, 3, 4, 5 };

    cache.AllocateObject(1, pageData, 5);
    cache.AllocateObject(2, pageData, 5);
    cache.AllocateObject(3, pageData, 5);

    ASSERT_TRUE(cache.Has(1));
    ASSERT_TRUE(cache.Has(2));
    ASSERT_TRUE(cache.Has(3));

    // Requires 2 pages.
    std::vector<int> largeData(10, 9);

    cache.AllocateObject(4, largeData.data(), largeData.size());

    ASSERT_TRUE(cache.Has(4));

    size_t survivors = 0;

    survivors += cache.Has(1) ? 1 : 0;
    survivors += cache.Has(2) ? 1 : 0;
    survivors += cache.Has(3) ? 1 : 0;

    // Only one old object should remain.
    ASSERT_EQ(survivors, 2u);
}

TEST_F(GPUPagedCacheTests, AllocateObjectAfterObjectDeallocationReusesFreedPages)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 2));

    const int pageData[] = { 1, 2, 3, 4, 5 };

    cache.AllocateObject(1, pageData, 5);
    cache.AllocateObject(2, pageData, 5);

    cache.DeallocateObject(1);

    ASSERT_FALSE(cache.Has(1));
    ASSERT_TRUE(cache.Has(2));

    // Should reuse freed page without eviction.
    cache.AllocateObject(3, pageData, 5);

    ASSERT_TRUE(cache.Has(2));
    ASSERT_TRUE(cache.Has(3));
}

TEST_F(GPUPagedCacheTests, AllocateObjectAfterClearObject)
{
    Cache cache;

    ASSERT_TRUE(cache.Create(GL_SHADER_STORAGE_BUFFER, 5, 2));

    const int initialData[] = { 1, 2, 3, 4, 5 };

    cache.AllocateObject(1, initialData, 5);

    ASSERT_TRUE(cache.Has(1));

    cache.ClearObject(1);

    ASSERT_TRUE(cache.Has(1));

    const int newData[] = { 9, 8, 7 };

    cache.AllocateObject(1, newData, 3);

    ASSERT_TRUE(cache.Has(1));

    const auto ranges = cache.GetObjectBufferRanges(1);

    ASSERT_EQ(ranges.size(), 1u);
}


// AllocatePages
// - AllocatePages single page
// - AllocatePages exact remaining capacity
// - AllocatePages exceed cache page count
// - AllocatePages with 0
// - AllocatePages causes single eviction
// - AllocatePages causes multiple evictions
// - AllocatePages eviction order follows policy
// - AllocatePages fragmented allocation
// - AllocatePages contiguous allocation
// - AllocatePages after deallocation reuses pages
// - AllocatePages append to existing allocation


// PushBackToObject
// - PushBackToObject existing object
// - PushBackToObject non-existing object
// - PushBackToObject into partially filled page
// - PushBackToObject filling page exactly
// - PushBackToObject causing new page allocation
// - PushBackToObject causing single eviction
// - PushBackToObject after ClearObject
// - PushBackToObject repeated append stress test
// - PushBackToObject fragmented page growth


// ClearObject
// - ClearObject basic
// - ClearObject empty object
// - ClearObject non-existing object

// MoveObject
// - MoveObject basic
// - MoveObject non-existing source
// - MoveObject onto existing destination


// Swap
// - Swap single-page objects
// - Swap multi-page objects
// - Swap non-existing object
// - Swap both non-existing objects
// - Swap object with itself


// GetObjectBufferRanges
// - GetObjectBufferRanges empty object
// - GetObjectBufferRanges non-existing object
// - GetObjectBufferRanges single page
// - GetObjectBufferRanges consecutive pages
// - GetObjectBufferRanges fragmented pages
// - GetObjectBufferRanges mixed contiguous and fragmented pages


// Eviction Clock Policy Behavior


// Stress / Randomized Tests
// - Random allocate/deallocate operations
// - Random push back operations
// - Random move/swap operations
// - Random eviction scenarios
// - Long-running allocation stress test
// - Full cache churn stress test
// - Repeated fragmentation/reuse stress test
// - Large object churn stress test
