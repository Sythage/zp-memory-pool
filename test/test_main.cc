#include <gtest/gtest.h>
#include "ZPmemoryPool.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>

using namespace ZPmemoryPool;

class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize HashBucket memory pools
        HashBucket::initMemoryPool();
    }
};

// Test basic MemoryPool functionality
TEST_F(MemoryPoolTest, BasicAllocation) {
    MemoryPool pool(4096);
    pool.init(64);
    
    void* ptr = pool.Allocate();
    ASSERT_NE(ptr, nullptr);
    
    pool.Deallocate(ptr);
}

TEST_F(MemoryPoolTest, MultipleAllocations) {
    MemoryPool pool(4096);
    pool.init(32);
    
    std::vector<void*> ptrs;
    
    // Allocate multiple blocks
    for(int i = 0; i < 100; ++i) {
        void* ptr = pool.Allocate();
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    // Deallocate all blocks
    for(void* ptr : ptrs) {
        pool.Deallocate(ptr);
    }
}

TEST_F(MemoryPoolTest, AllocationDeallocationCycle) {
    MemoryPool pool(4096);
    pool.init(16);
    
    // Test allocation and deallocation cycle
    for(int cycle = 0; cycle < 10; ++cycle) {
        std::vector<void*> ptrs;
        
        // Allocate
        for(int i = 0; i < 50; ++i) {
            void* ptr = pool.Allocate();
            ASSERT_NE(ptr, nullptr);
            ptrs.push_back(ptr);
        }
        
        // Deallocate
        for(void* ptr : ptrs) {
            pool.Deallocate(ptr);
        }
    }
}

// Test HashBucket functionality
TEST_F(MemoryPoolTest, HashBucketBasicUsage) {
    void* ptr8 = HashBucket::useMemory(8);
    ASSERT_NE(ptr8, nullptr);
    HashBucket::freeMemory(ptr8, 8);
    
    void* ptr64 = HashBucket::useMemory(64);
    ASSERT_NE(ptr64, nullptr);
    HashBucket::freeMemory(ptr64, 64);
    
    void* ptr256 = HashBucket::useMemory(256);
    ASSERT_NE(ptr256, nullptr);
    HashBucket::freeMemory(ptr256, 256);
}

TEST_F(MemoryPoolTest, HashBucketLargeAllocation) {
    // Test allocation larger than MAX_SLOT_SIZE
    void* ptr = HashBucket::useMemory(1024);
    ASSERT_NE(ptr, nullptr);
    HashBucket::freeMemory(ptr, 1024);
}

TEST_F(MemoryPoolTest, HashBucketZeroSize) {
    void* ptr = HashBucket::useMemory(0);
    ASSERT_EQ(ptr, nullptr);
}

// Test newElement and deleteElement
struct TestObject {
    int value;
    double data;
    
    TestObject(int v, double d) : value(v), data(d) {}
};

TEST_F(MemoryPoolTest, NewDeleteElement) {
    TestObject* obj = newElement<TestObject>(42, 3.14);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->value, 42);
    EXPECT_DOUBLE_EQ(obj->data, 3.14);
    
    deleteElement(obj);
}

TEST_F(MemoryPoolTest, MultipleNewDeleteElements) {
    std::vector<TestObject*> objects;
    
    // Create multiple objects
    for(int i = 0; i < 100; ++i) {
        TestObject* obj = newElement<TestObject>(i, i * 2.5);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(obj->value, i);
        EXPECT_DOUBLE_EQ(obj->data, i * 2.5);
        objects.push_back(obj);
    }
    
    // Delete all objects
    for(TestObject* obj : objects) {
        deleteElement(obj);
    }
}

// Thread safety tests
TEST_F(MemoryPoolTest, ConcurrentAllocation) {
    MemoryPool pool(8192);
    pool.init(32);
    
    const int num_threads = 4;
    const int allocations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    
    for(int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&pool, &success_count, allocations_per_thread]() {
            std::vector<void*> local_ptrs;
            
            // Allocate
            for(int i = 0; i < allocations_per_thread; ++i) {
                void* ptr = pool.Allocate();
                if(ptr != nullptr) {
                    local_ptrs.push_back(ptr);
                    success_count++;
                }
            }
            
            // Deallocate
            for(void* ptr : local_ptrs) {
                pool.Deallocate(ptr);
            }
        });
    }
    
    for(auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count.load(), num_threads * allocations_per_thread);
}

TEST_F(MemoryPoolTest, ConcurrentHashBucket) {
    const int num_threads = 4;
    const int allocations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    
    for(int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&success_count, allocations_per_thread]() {
            std::vector<std::pair<void*, size_t>> local_ptrs;
            
            // Allocate different sizes
            for(int i = 0; i < allocations_per_thread; ++i) {
                size_t size = 8 + (i % 10) * 8; // 8, 16, 24, ..., 88
                void* ptr = HashBucket::useMemory(size);
                if(ptr != nullptr) {
                    local_ptrs.push_back({ptr, size});
                    success_count++;
                }
            }
            
            // Deallocate
            for(auto& pair : local_ptrs) {
                HashBucket::freeMemory(pair.first, pair.second);
            }
        });
    }
    
    for(auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count.load(), num_threads * allocations_per_thread);
}

// Performance test
TEST_F(MemoryPoolTest, PerformanceComparison) {
    const int num_allocations = 10000;
    
    // Test memory pool performance
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<void*> pool_ptrs;
    for(int i = 0; i < num_allocations; ++i) {
        void* ptr = HashBucket::useMemory(64);
        pool_ptrs.push_back(ptr);
    }
    
    for(void* ptr : pool_ptrs) {
        HashBucket::freeMemory(ptr, 64);
    }
    
    auto pool_time = std::chrono::high_resolution_clock::now() - start;
    
    // Test standard allocation performance
    start = std::chrono::high_resolution_clock::now();
    
    std::vector<void*> std_ptrs;
    for(int i = 0; i < num_allocations; ++i) {
        void* ptr = operator new(64);
        std_ptrs.push_back(ptr);
    }
    
    for(void* ptr : std_ptrs) {
        operator delete(ptr);
    }
    
    auto std_time = std::chrono::high_resolution_clock::now() - start;
    
    // Memory pool should be faster or at least comparable
    std::cout << "Memory pool time: " << std::chrono::duration_cast<std::chrono::microseconds>(pool_time).count() << "us" << std::endl;
    std::cout << "Standard allocation time: " << std::chrono::duration_cast<std::chrono::microseconds>(std_time).count() << "us" << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}