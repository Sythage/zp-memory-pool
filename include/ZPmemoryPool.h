/**
 * @file ZPmemoryPool.h
 * @brief  Memory Pool implementation
 * @author pan
 * @date 2025-07-09
 * @version 1.0
 */

#ifndef ZP_MEMOEY_POOPL_H
#define ZP_MEMOEY_POOPL_H

#include <cstddef>
#include <mutex>

/**
 * @namespace ZPmemoryPool
 * @brief  Memory Pool namespace containing all memory pool related classes and functions
 */
namespace ZPmemoryPool {

/// @brief Number of memory pools
#define MEMORY_POOL_NUM 64
/// @brief Base size for memory slots (in bytes)
#define SLOT_BASE_SIZE 8
/// @brief Maximum size for memory slots (in bytes)
#define MAX_SLOT_SIZE 512

/**
 * @struct Slot
 * @brief A memory slot structure for linked list management
 * 
 * This structure represents a single slot in the memory pool's free list.
 * It uses intrusive linked list approach where each slot contains a pointer
 * to the next available slot.
 */
struct Slot{
    Slot* next; ///< Pointer to the next free slot
};

/**
 * @class MemoryPool
 * @brief A thread-safe memory pool implementation for efficient memory allocation
 * 
 * This class provides a memory pool that pre-allocates large blocks of memory
 * and divides them into smaller slots of fixed size. It supports efficient
 * allocation and deallocation operations with O(1) time complexity.
 * 
 * Features:
 * - Thread-safe operations using mutexes
 * - Efficient memory reuse through free list management
 * - Automatic block allocation when needed
 * - Configurable block and slot sizes
 * 
 * @note This implementation is designed for scenarios where frequent
 *       allocations and deallocations of same-sized objects occur.
 */
class MemoryPool{
public:
    /**
     * @brief Constructor that initializes the memory pool
     * @param block_size_ The size of each memory block in bytes (default: 4096)
     * 
     * Creates a new memory pool with the specified block size.
     * The block size determines how much memory is allocated at once
     * when the pool needs to expand.
     */
    MemoryPool(size_t block_size_ = 4096);
    
    /**
     * @brief Destructor that cleans up all allocated memory
     * 
     * Releases all memory blocks allocated by this memory pool.
     * Ensures proper cleanup to prevent memory leaks.
     */
    ~MemoryPool();

    /**
     * @brief Initialize the memory pool with a specific slot size
     * @param slot_size The size of each slot in bytes
     * 
     * This method must be called before using Allocate() or Deallocate().
     * It sets up the internal structure based on the desired slot size.
     */
    void init(size_t slot_size);

    /**
     * @brief Allocate a memory slot from the pool
     * @return Pointer to the allocated memory slot, or nullptr if allocation fails
     * 
     * Returns a pointer to a memory slot of the size specified in init().
     * If no free slots are available, a new memory block will be allocated.
     * 
     * @note This method is thread-safe
     * @warning The returned pointer must be deallocated using Deallocate()
     */
    void* Allocate();
    
    /**
     * @brief Deallocate a memory slot back to the pool
     * @param ptr Pointer to the memory slot to be deallocated
     * @return Pointer to the deallocated slot (for chaining operations)
     * 
     * Returns the memory slot to the free list for future reuse.
     * The slot becomes available for subsequent Allocate() calls.
     * 
     * @note This method is thread-safe
     * @warning The pointer must have been obtained from this pool's Allocate() method
     * @warning Do not use the pointer after calling this method
     */
    void* Deallocate(void* ptr);
private:
    /**
     * @brief Allocate a new memory block when the current block is exhausted
     * 
     * This method is called internally when there are no more free slots
     * available in the current memory block. It allocates a new block
     * and divides it into slots.
     * 
     * @note This method is not thread-safe and should be called with proper locking
     */
    void AllocateNewBlock();
    
    /**
     * @brief Calculate the padding needed for pointer alignment
     * @param p Pointer to be aligned
     * @param align Alignment requirement in bytes
     * @return Number of bytes needed for padding
     * 
     * Calculates how many bytes are needed to align a pointer to the
     * specified boundary. This ensures proper memory alignment for
     * optimal performance and compatibility.
     */
    size_t PadPointer(char* p, size_t align);

private:
    size_t          block_size_;            // 内存块大小
    size_t          slot_size_;             // 槽大小
    Slot*           first_block_;            // 指向内存池管理的首个实际内存块
    Slot*           current_slot_;          // 指向当前未被使用的slot
    Slot*           free_list_;             // 指向空闲的槽（被使用后又被释放的slot）
    Slot*           last_slot_;             // 作为当前内存块中最后能够存放元素的位置表示（超过该位置需要申请新的block）
    std::mutex      mutex_for_free_list_;   // 保证free_list_ 在多线程中的原子性
    std::mutex      mutex_for_block_;       // 保证多线程情况下避免不必要的重复开辟内存导致的浪费行为

};



}

#endif
