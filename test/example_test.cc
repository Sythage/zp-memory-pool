#include <gtest/gtest.h>
#include "ZPmemoryPool.h"

using namespace ZPmemoryPool;

// Simple test for beginners
TEST(ExampleTest, SimpleMemoryPoolUsage) {
    // Initialize the memory pool system
    HashBucket::initMemoryPool();
    
    // Allocate memory for a simple integer
    void* ptr = HashBucket::useMemory(sizeof(int));
    ASSERT_NE(ptr, nullptr);
    
    // Use the memory
    int* int_ptr = static_cast<int*>(ptr);
    *int_ptr = 42;
    EXPECT_EQ(*int_ptr, 42);
    
    // Free the memory
    HashBucket::freeMemory(ptr, sizeof(int));
}

TEST(ExampleTest, TemplateNewDeleteUsage) {
    // Initialize the memory pool system
    HashBucket::initMemoryPool();
    
    // Create an object using template functions
    struct TestData {
        int id;
        double value;
        TestData(int i, double v) : id(i), value(v) {}
    };
    
    TestData* obj = newElement<TestData>(123, 45.67);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->id, 123);
    EXPECT_DOUBLE_EQ(obj->value, 45.67);
    
    // Clean up
    deleteElement(obj);
}

TEST(ExampleTest, DirectMemoryPoolUsage) {
    // Create a memory pool directly
    MemoryPool pool(4096);  // 4KB block size
    pool.init(64);          // 64-byte slots
    
    // Allocate some memory
    void* ptr1 = pool.Allocate();
    void* ptr2 = pool.Allocate();
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);  // Should be different pointers
    
    // Deallocate
    pool.Deallocate(ptr1);
    pool.Deallocate(ptr2);
    
    // Reallocate - should reuse freed memory
    void* ptr3 = pool.Allocate();
    ASSERT_NE(ptr3, nullptr);
    // ptr3 should be one of the previously allocated pointers
    EXPECT_TRUE(ptr3 == ptr1 || ptr3 == ptr2);
    
    pool.Deallocate(ptr3);
}
