/**
 * @file ThreadCache.h
 * @brief 线程本地缓存实现
 * @author ZP Memory Pool Team
 * @date 2025-07-11
 */

#ifndef THREAD_CACHE_H
#define THREAD_CACHE_H
#include "Common.h"
#include <array>
#include <cstddef>

namespace SupperPool {

/**
 * @brief 线程本地缓存类
 * 
 * 为每个线程提供独立的内存缓存，减少线程间竞争，提高内存分配效率。
 * 使用thread_local单例模式确保每个线程有唯一的ThreadCache实例。
 */
class ThreadCache
{
public:
    /**
     * @brief 获取线程本地ThreadCache实例
     * @return ThreadCache* 当前线程的ThreadCache实例指针
     * 
     * 使用thread_local关键字实现线程本地单例模式，
     * 确保每个线程都有自己独立的ThreadCache实例
     */
    static ThreadCache* getInstance()
    {
        static thread_local ThreadCache instance;
        return &instance;
    }

    /**
     * @brief 分配指定大小的内存
     * @param size 需要分配的内存大小（字节）
     * @return void* 分配的内存指针，失败时返回nullptr
     * 
     * 优先从线程本地缓存分配，如果缓存不足则从中心缓存获取
     */
    void* allocate(size_t size);
    
    /**
     * @brief 释放指定大小的内存
     * @param ptr 要释放的内存指针
     * @param size 内存块大小（字节）
     * 
     * 将内存归还到线程本地缓存，如果缓存过多则归还给中心缓存
     */
    void deallocate(void* ptr, size_t size);

private:
    /**
     * @brief 私有构造函数，防止外部直接创建实例
     */
    ThreadCache() = default;
    
    /**
     * @brief 从中心缓存获取内存块
     * @param index 空闲链表数组索引
     * @return void* 获取的内存块指针
     * 
     * 当线程本地缓存不足时，批量从中心缓存获取内存块
     */
    void* fetchFromCentralCache(size_t index);
    
    /**
     * @brief 归还内存块到中心缓存
     * @param start 要归还的内存块起始指针
     * @param size 内存块大小（字节）
     * 
     * 当线程本地缓存过多时，将多余的内存块归还给中心缓存
     */
    void returnTocentralCache(void* start, size_t size);
    
    /**
     * @brief 计算批量获取内存块的数量
     * @param size 内存块大小（字节）
     * @return size_t 批量获取的内存块数量
     * 
     * 根据内存块大小动态调整批量获取数量，平衡性能和内存使用
     */
    size_t getBatchNum(size_t size);
    
    /**
     * @brief 判断是否需要归还内存给中心缓存
     * @param index 空闲链表数组索引
     * @return bool true表示需要归还，false表示不需要
     * 
     * 根据当前缓存大小和阈值判断是否需要向中心缓存归还内存
     */
    bool sholdReturnToCentralCache(size_t index);

private:
    /**
     * @brief 线程本地空闲链表数组
     * 
     * 每个索引对应不同大小的内存块链表，
     * 索引通过SizeClass::getIndex()计算得出
     */
    std::array<void*, FREE_LIST_SIZE> free_list_;
    
    /**
     * @brief 空闲链表大小统计数组
     * 
     * 记录每个空闲链表中当前缓存的内存块数量，
     * 用于判断是否需要从中心缓存获取或归还内存
     */
    std::array<size_t, FREE_LIST_SIZE> free_list_size_;

};

} // namespace SupperPool

#endif // THREAD_CACHE_H