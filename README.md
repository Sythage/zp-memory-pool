# zp-memory-pool
使用c++实现memory pool，供学习练习使用


## 项目开发环境搭建
[参考该项目的现代C++开发环境](https://github.com/rutura/cpp23m)

## 项目：基于哈希映射的多种定长内存分配器
![](https://cdn.nlark.com/yuque/0/2024/jpeg/39027506/1732612548134-15b20832-d60d-4cdc-8990-cc508ad4a1c7.jpeg?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_25%2Ctext_5Luj56CB6ZqP5oOz5b2V55-l6K-G5pif55CD%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10)


![](https://cdn.nlark.com/yuque/0/2024/jpeg/39027506/1732623935791-c3136b91-afc4-4e2a-9838-4c7f6056dbdb.jpeg?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_34%2Ctext_5Luj56CB6ZqP5oOz5b2V55-l6K-G5pif55CD%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10)

通过hashbucket可以找到存储不同大小数据的内存池，一共设计了从8～512一共64种类型的内存池。不过，每个pool自身的size是一致的。   
但是并不是OS直接为pool事先分配好固定内存，而是需要的时候才分配，释放对象后不被os回收，而是加入到freeslot 链表，下一个对象就不需要调用new 分配了。

1. MemoryPool 和 HashBucket 不关心类型，只关心大小。在newElement和deleteElement处是模板函数，会传入类型。   
2. 内存对齐的意义：   
    + 提高访问效率：cpu缓存一般是固定大小的block（64，128），数据对齐则可以完全放入缓冲block，提高缓存命中率
    + 保证兼容性： C++标准库的数据结构通常要求对齐，例如std::allocator分配的内存默认按最大对齐要求对齐。对齐内存可以确保代码在不同架构下正确运行，避免潜在问题。