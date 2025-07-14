/**
 * @file Common.h
 * @brief 内存池公共定义和工具类
 * @author ZP Memory Pool Team
 * @date 2025-07-11
 */

#ifndef COMMON_H
#define COMMON_H
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace SupperPool {

/** @brief 内存对齐字节数，通常为8字节 */
constexpr size_t ALIGNMENT = 8;

/** @brief 内存池管理的最大块大小，超过此大小直接使用系统分配 */
constexpr size_t MAX_BYTES = 256;

/** @brief 空闲链表数组大小，用于管理不同大小的内存块 */
constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;

/**
 * @brief 内存块头部信息结构体
 * 
 * 用于管理内存块的元数据信息，包括大小、使用状态和链表指针
 */
struct BlockHeader{
    size_t block_size;  /**< 内存块大小（字节） */
    bool in_use;        /**< 内存块是否正在使用 */
    BlockHeader* next;  /**< 指向下一个内存块的指针 */
};

/**
 * @brief 大小类管理工具类
 * 
 * 提供内存大小对齐和索引计算功能，用于内存池的大小分类管理
 */
class SizeClass{
public:
    /**
     * @brief 将字节数向上对齐到ALIGNMENT的倍数
     * @param bytes 原始字节数
     * @return size_t 对齐后的字节数
     * 
     * 使用位运算快速计算对齐值，确保返回值是ALIGNMENT的倍数
     */
    static size_t roundUp(size_t bytes)
    {
        return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }

    /**
     * @brief 根据字节数计算在空闲链表数组中的索引
     * @param bytes 内存块大小（字节）
     * @return size_t 在空闲链表数组中的索引位置
     * 
     * 将内存大小映射到对应的空闲链表索引，用于快速定位合适的内存块
     */
    static size_t getIndex(size_t bytes)
    {
        // makesure bytes at least ALIGNMENT
        bytes = std::max(bytes, ALIGNMENT);
        // 向上取整后 -1
        return (bytes + ALIGNMENT -1) / ALIGNMENT - 1;
    }
};

}   // namespace SupperPool

#endif // COMMON_H