#include "ZPmemoryPool.h"
#include <cassert>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <iostream>

namespace ZPmemoryPool {

MemoryPool::MemoryPool(size_t block_size)
: block_size_(block_size), slot_size_(0), first_block_(nullptr), 
  current_slot_(nullptr), free_list_(nullptr), last_slot_(nullptr)
{};

MemoryPool::~MemoryPool(){
    // delete the continuous block
    Slot* cur = first_block_;
    while (cur) {
        Slot* next = cur->next;
        // 等同于 free(reinterpret_cast《void*》（first_block_）)
        // 转化为void指针，因为void 类型不需要调用析构函数，只释放空间
        operator delete(reinterpret_cast<void*>(cur)); // 这一句有点复杂
        cur = next;
    }
};

void MemoryPool::init(size_t size){
    assert(size>0);
    slot_size_ = size;
    first_block_ = nullptr;
    current_slot_ = nullptr;
    free_list_ = nullptr;
    last_slot_ = nullptr;
}

void* MemoryPool::Allocate(){
    // 优先使用空闲链表中的内存槽
    if(free_list_ != nullptr){
        {
            std::lock_guard<std::mutex> lock(mutex_for_free_list_);
            if(free_list_ != nullptr){
                Slot* temp = free_list_;
                free_list_ = free_list_->next;
                return temp;
            }
        }
    }

    Slot* temp;
    {
        std::lock_guard<std::mutex> lock(mutex_for_block_);
        if(current_slot_ == nullptr || current_slot_ > last_slot_){
            // 当前memory block is 不可用，开辟新一块
            AllocateNewBlock();
        }

        temp = current_slot_;
        // Move to next slot
        current_slot_ = reinterpret_cast<Slot*>(reinterpret_cast<char*>(current_slot_) + slot_size_);
    }
    return temp;
}

void MemoryPool::Deallocate(void* ptr){
    if(ptr)
    {
        // hui shou memory, which is inserted free list by head insert method
        std::lock_guard<std::mutex> lock(mutex_for_free_list_);
        reinterpret_cast<Slot*>(ptr)->next = free_list_;
        free_list_ = reinterpret_cast<Slot*>(ptr);
    }
}

void MemoryPool::AllocateNewBlock()
{
    // std::cout << "申请一块内存，slotsize： "<< slot_size_ << std::endl;
    // head insert new memory block
    void* new_block = operator new(block_size_);
    reinterpret_cast<Slot*>(new_block)->next = first_block_;
    first_block_ = reinterpret_cast<Slot*>(new_block);

    char* body = reinterpret_cast<char*>(new_block) + sizeof(Slot*);
    size_t padding_size = PadPointer(body, slot_size_);
    
    // Set current_slot_ to the beginning of usable memory (after padding)
    current_slot_ = reinterpret_cast<Slot*>(body + padding_size);
    
    // Set last_slot_ to the last possible slot in this block
    char* block_end = reinterpret_cast<char*>(new_block) + block_size_;
    last_slot_ = reinterpret_cast<Slot*>(block_end - slot_size_);
}

size_t MemoryPool::PadPointer(char* p, size_t align)
{// 让pointer 对齐到槽大小的数倍
    // align  == slot_size
    return (align - reinterpret_cast<size_t>(p)) % align;
    // 一个槽包括了一个指针
}

void HashBucket::initMemoryPool(){
    for(int i = 0; i < MEMORY_POOL_NUM; i++){
        getMemoryPool(i).init((i+1) * SLOT_BASE_SIZE);
        // 0-->8;1-->16;...8-->64... 
    }
}

// 单例模式
MemoryPool& HashBucket::getMemoryPool(int index){
    if(index < 0 || index >= MEMORY_POOL_NUM)
    {
        throw std::out_of_range("MemoryPool index out of range");
    }
    static MemoryPool pools[MEMORY_POOL_NUM];
    return pools[index];
}



} // ZPmemoryPoll