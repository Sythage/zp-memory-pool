#include "../include/ThreadCache.h"
#include "../include/Common.h"
#include "Common.h"
#include <cstddef>


namespace SupperPool {

void* ThreadCache::allocate(size_t size)
{
    //处理0大小的分配请求
    if(size == 0){
        size = ALIGNMENT; // 至少分配一个对齐大小
    }

    if(size > MAX_BYTES)
    {
        // 大对象直接从系统分配
        return malloc(size);
    }

    size_t index = SizeClass::getIndex(size);

    // 更新对应自由链表的长度计数
    free_list_size_[index]--;

    // 检查线程本地自由链表
    // 如果 free_list_[index] 不为空,表示该链表中有可用内存块
    if(void* ptr = free_list_[index])
    {   // 将free_list_[index] 指向内存块的下一个内存块地址（取决于内存块的实现）
        free_list_[index] = *reinterpret_cast<void**>(ptr);
    }
    // 如果线程本地自由链表为空，则从中心缓存获取一批内存
    return fetchFromCentralCache(index);
}

void* ThreadCache::fetchFromCentralCache(size_t index)
{
    // fetch get memory from centrol cache 
    void* start = CentralCache::getInstance().fetchRange(index);
    if(!start) return nullptr;

    // 取一个返回，其余放入自由链表
    void* result = start;
    free_list_[index] = *reinterpret_cast<void**>(start);

    // update free list size
    size_t batch_num = 0;
    void* current = start; // 从start 开始遍历

    // 计算从中心缓存获取的内存块数量
    while(current != nullptr)
    {
        batch_num++;
        current = *reinterpret_cast<void**>(current); // 遍历下一个内存块
    }

    // 更新free_list_size_；增加获取的内存块数量
    free_list_size_[index] += batch_num;

    return result;
}

bool ThreadCache::sholdReturnToCentralCache(size_t index)
{
    // 设定阈值，例如：当自由链表的大小超过一定数量时
    size_t threshold = 256;
    return (free_list_size_[index] > threshold);
}

void ThreadCache::deallocate(void* ptr, size_t size)
{
    if(size > MAX_BYTES)
    {
        free(ptr);
        return;
    }

    size_t index = SizeClass::getIndex(size);

    // 插入到线程本地自由链表
    *reinterpret_cast<void**>(ptr) = free_list_[index];
    free_list_[index] = ptr;

    // 更新对应自由链表大小
    free_list_size_[index]++; // 增加对应大小类的自由链表大小

    // 判断是否需要将一部分内存回收给中心缓存
    if(sholdReturnToCentralCache(index))
    {
        returnTocentralCache(free_list_[index], size);
    }
}

void ThreadCache::returnTocentralCache(void* start, size_t size)
{
    // 根据大小计算对应的索引
    size_t index = SizeClass::getIndex(size);

    // 获取对齐后的实际块大小
    size_t aligned_size = SizeClass::roundUp(size);

    // 计算要归还内存块数量
    size_t batch_num = free_list_size_[index];
    if(batch_num <= 1) return; // 如果只有一个block，则不返回

    // 保留一部分在ThreadCache中（比如保留1/4）
    size_t keep_num = std::max(batch_num / 4, size_t(1));
    size_t return_num = batch_num - keep_num;

    // 将内存块串成链表
    char* current = static_cast<char*>(start);
    // 使用对齐以后的大小分割点
    char* split_node = current;
    for(size_t i = 0; i < keep_num - 1; ++i)
    {
        split_node = reinterpret_cast<char*>(*reinterpret_cast<void**>(split_node));
        if(split_node == nullptr)
        {
            // 如果链表提前结束，更新实际的返回数量
            return_num = batch_num - (i + 1);
            break;
        }
    }

    if(split_node != nullptr)
    {
        // 将要返回的部分和要保留的部分断开
        void* next_node = *reinterpret_cast<void**>(split_node);
        *reinterpret_cast<void**>(split_node) = nullptr;// break the connection

        // 更新ThreadCache 的空闲链表
        free_list_[index] = start;

        // 更新自由链表大小
        free_list_size_[index] = keep_num;

        // 将剩余部分返回给CentralCache
        if(return_num > 0 && next_node != nullptr)
        {
            CentralCache::getInstance().rerurnRange(next_node, return_num * aligned_size, index);
        }
    }
}

}// namespace SupperPool

