#include "../include/ZPmemoryPool.h"
#include <cassert>
#include <cstddef>
#include <mutex>

namespace ZPmemoryPool {

MemoryPool::MemoryPool(size_t block_size)
: block_size_(block_size)
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
        if(current_slot_ >= last_slot_){
            // 当前memory block is 不可用，开辟新一块
            AllocateNewBlock();
        }

        temp = current_slot_;
        // 这里不能直接current_slot_ += slot_size_ 因为current_slot_是slot* 类型，所以需要除以slotsize再加一
        current_slot_ += slot_size_ / sizeof(Slot);
    }
    return temp;
}

void* MemoryPool::Deallocate(void* ptr){
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

    char* body = reinterpret_cast<char*>(new_block)+sizeof(Slot*);
    size_t padding_size = PadPointer(body, slot_size_);
    current_slot_ = reinterpret_cast<Slot*>(reinterpret_cast<size_t>(new_block)+block_size_-slot_size_+1);

    free_list_ = nullptr;
}



} // ZPmemoryPoll