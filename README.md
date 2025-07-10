# zp-memory-pool
使用c++实现memory pool，供学习练习使用
## 项目开发环境搭建
[参考该项目的现代C++开发环境](https://github.com/rutura/cpp23m)

#   项目分析
## 1.为啥子要memory pool？
```C++
// 传统方式的问题
for(int i = 0; i < 10000; ++i) {
    MyObject* obj = new MyObject();  // 每次都要向操作系统申请内存
    // ... 使用对象
    delete obj;                      // 每次都要归还给操作系统
}
```
**question：**
+ **内存开销大**：每次 new/delete 都要系统调用
+ **内存碎片**： 频繁分配释放导致内存碎片化（内碎片，一分配块内未被实际使用的部分；外碎片，系统中有足够的空闲内存，但是这些空闲不连续，无法满足一个较大的分配请求）
+ **不确定性**： 分配时间不可预测 

内存池就是通过与预先分配（new）一大块内存作为pool，然后新对象就使用pool中的内存。
```
内存池中的 Block 链表：
┌─────────────────────────────────────────────────────────┐
│                    Memory Pool                          │
│                   (slot_size=64)                       │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  Block 3 (最新分配的)                                   │
│  ┌───────┬─────────┬─────────┬─────────┬─────────┐      │
│  │Header │ Slot1   │ Slot2   │ Slot3   │  ...    │      │
│  │next=B2│ (64B)   │ (64B)   │ (64B)   │         │      │
│  └───────┴─────────┴─────────┴─────────┴─────────┘      │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  Block 2                                                │
│  ┌───────┬─────────┬─────────┬─────────┬─────────┐      │
│  │Header │ Slot1   │ Slot2   │ Slot3   │  ...    │      │
│  │next=B1│ (64B)   │ (64B)   │ (64B)   │         │      │
│  └───────┴─────────┴─────────┴─────────┴─────────┘      │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  Block 1 (最初分配的)                                   │
│  ┌───────┬─────────┬─────────┬─────────┬─────────┐      │
│  │Header │ Slot1   │ Slot2   │ Slot3   │  ...    │      │
│  │next=  │ (64B)   │ (64B)   │ (64B)   │         │      │
│  │NULL   │         │         │         │         │      │
│  └───────┴─────────┴─────────┴─────────┴─────────┘      │
└─────────────────────────────────────────────────────────┘
```

## 2.核心datastructure
### 2.1 Slot
```C++
struct Slot{
    Slot* next;/// pointer to the next free slot
}
```
```bash
    内存布局示意：
┌─────────────┬─────────────┬─────────────┐
│   Slot 1    │   Slot 2    │   Slot 3    │
│ next = &S2  │ next = &S3  │ next = NULL │
└─────────────┴─────────────┴─────────────┘
      ↓             ↓             ↓
   free_list     (空闲链表)
```
**为啥子这么设计？**
+ 零额外开销：利用未分配的内存空间存储链表指针
+ **O（1）分配** ：直接从链表头取出即可
+ **O（1）释放**： 直接插入链表头即可

### 2.2 内存块管理
```C++
private:
    size_t          block_size_;            // 每个大块的大小(如4KB)
    size_t          slot_size_;             // 每个槽的大小(如64字节)
    Slot*           first_block_;           // 指向第一个内存块
    Slot*           current_slot_;          // 当前可用槽的位置
    Slot*           free_list_;             // 已释放但可重用的槽链表
    Slot*           last_slot_;             // 当前块的最后一个槽
```
```bash
Memory Block (4096 bytes):
┌────────────┬─────────────────────────────────────────────┐
│Block Header│              Slots Area                     │
│(next ptr)  │  [Slot1][Slot2][Slot3]...[SlotN]          │
└────────────┴─────────────────────────────────────────────┘
             ↑                              ↑
        current_slot                   last_slot
```

## 3. 核心算法
### 3.1 内存分配

```c++
void* MemoryPool::Allocate(){
    // 第一优先级：重用已释放的内存
    if(free_list_ != nullptr){
        {
            std::lock_guard<std::mutex> lock(mutex_for_free_list_);
            if(free_list_ != nullptr){  // 双重检查锁定模式
                Slot* temp = free_list_;
                free_list_ = free_list_->next;  // 链表头出队
                return temp;
            }
        }
    }

    // 第二优先级：从当前块分配新槽
    Slot* temp;
    {
        std::lock_guard<std::mutex> lock(mutex_for_block_);
        if(current_slot_ == nullptr || current_slot_ > last_slot_){
            AllocateNewBlock();  // 当前块用完，分配新块
        }

        temp = current_slot_;
        // 指针算术：移动到下一个槽位置
        current_slot_ = reinterpret_cast<Slot*>(
            reinterpret_cast<char*>(current_slot_) + slot_size_
        );
    }
    return temp;
}
```
+ **优先重用**： 先检查已释放的内存，减少浪费
+ **双重检查锁**： 保证线程安全的同时提高性能
+ **指针算术** ： 精确计算下一个槽的位置   
### 3.2新块分配算法
```C++
void MemoryPool::AllocateNewBlock()
{
    // 分配一大块原始内存
    void* new_block = operator new(block_size_);
    
    // 建立块链表（用于析构时释放）
    reinterpret_cast<Slot*>(new_block)->next = first_block_;
    first_block_ = reinterpret_cast<Slot*>(new_block);

    // 计算可用内存区域
    char* body = reinterpret_cast<char*>(new_block) + sizeof(Slot*);
    size_t padding_size = PadPointer(body, slot_size_);
    
    // 设置当前槽和最后槽的位置
    current_slot_ = reinterpret_cast<Slot*>(body + padding_size);
    char* block_end = reinterpret_cast<char*>(new_block) + block_size_;
    last_slot_ = reinterpret_cast<Slot*>(block_end - slot_size_);
}
```

1. **块链表管理**：每个block的开头存储指向下一个block的指针
2. **内存对齐**： 确保槽的起始地址满足对齐要求
3. **边界计算**： 精确计算最后一个有效slot的位置。  
### 3.3 内存对齐算法
```cPP
size_t MemoryPool::PadPointer(char* p, size_t align)
{
    // 计算需要填充多少字节来达到对齐
    return (align - reinterpret_cast<size_t>(p)) % align;
}
```
```h
// 不对齐的内存访问（可能导致性能问题或崩溃）
struct Data {
    double value;  // 需要8字节对齐
};

// 如果指针不是8的倍数，访问可能很慢或出错
```
## 4.高级设计：HashBucket 系统
### 4.1 多池管理

```CPP
class HashBucket {
    static MemoryPool& getMemoryPool(int index){
        static MemoryPool pools[MEMORY_POOL_NUM];  // 64个不同大小的池
        return pools[index];
    }
    
    static void* useMemory(size_t size){
        if(size > MAX_SLOT_SIZE)
            return operator new(size);  // 超大内存用标准分配

        // 根据大小选择合适的池：8,16,24,32...512字节
        return getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).Allocate();
    }
};
```

```bash
Pool 0: 8字节槽   -> 适合小对象
Pool 1: 16字节槽  -> 适合稍大对象  
Pool 2: 24字节槽  -> ...
...
Pool 63: 512字节槽 -> 适合大对象
> 512字节: 使用标准 new/delete
```

### 4.2 模板接口设计
```C++
template<typename T, typename... Args>
T* newElement(Args&&... args){
    T* p = nullptr;
    // 根据类型大小自动选择合适的池
    if((p = reinterpret_cast<T*>(HashBucket::useMemory(sizeof(T)))) != nullptr){
        // placement new: 在已分配内存上构造对象
        new(p) T(std::forward<Args>(args)...);
    }
    return p;
}
```
**why use placement new?**
```CPP
// 传统方式：分配+构造一体化
MyClass* obj = new MyClass(arg1, arg2);

// 内存池方式：分离分配和构造
void* memory = pool.Allocate();           // 只分配内存
MyClass* obj = new(memory) MyClass(arg1, arg2);  // 在指定位置构造
```

## 5. 线程安全设计
### 5.1 双锁策略
```CPP
std::mutex mutex_for_free_list_;   // 保护空闲链表
std::mutex mutex_for_block_;       // 保护块分配
```
**为啥子用两个🔒？**
1. 减少锁竞争： 不同操作用不同锁
2. 提高并发性：一个线程在释放内存时，另一个可以分配新block
3. 避免死锁： 锁的粒度更细

### 5.2 双重检查锁定模式
```CPP
if(free_list_ != nullptr){  // 第一次检查（无锁）
    {
        std::lock_guard<std::mutex> lock(mutex_for_free_list_);
        if(free_list_ != nullptr){  // 第二次检查（有锁）
            // 实际操作
        }
    }
}
```
**为什么这样做？**

1. 避免不必要的加锁开销
2. 在高并发场景下显著提升性能





## 项目：基于哈希映射的多种定长内存分配器
![](https://cdn.nlark.com/yuque/0/2024/jpeg/39027506/1732612548134-15b20832-d60d-4cdc-8990-cc508ad4a1c7.jpeg?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_25%2Ctext_5Luj56CB6ZqP5oOz5b2V55-l6K-G5pif55CD%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10)


![](https://cdn.nlark.com/yuque/0/2024/jpeg/39027506/1732623935791-c3136b91-afc4-4e2a-9838-4c7f6056dbdb.jpeg?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_34%2Ctext_5Luj56CB6ZqP5oOz5b2V55-l6K-G5pif55CD%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10)

通过hashbucket可以找到存储不同大小数据的内存池，一共设计了从8～512一共64种类型的内存池。不过，每个pool自身的size是一致的。   
但是并不是OS直接为pool事先分配好固定内存，而是需要的时候才分配，释放对象后不被os回收，而是加入到freeslot 链表，下一个对象就不需要调用new 分配了。

1. MemoryPool 和 HashBucket 不关心类型，只关心大小。在newElement和deleteElement处是模板函数，会传入类型。   
2. 内存对齐的意义：   
    + 提高访问效率：cpu缓存一般是固定大小的block（64，128），数据对齐则可以完全放入缓冲block，提高缓存命中率
    + 保证兼容性： C++标准库的数据结构通常要求对齐，例如std::allocator分配的内存默认按最大对齐要求对齐。对齐内存可以确保代码在不同架构下正确运行，避免潜在问题。

