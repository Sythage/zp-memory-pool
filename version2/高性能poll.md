![](https://cdn.nlark.com/yuque/0/2025/png/39027506/1737645723531-894026d7-2d33-455c-afbf-bf6015175533.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_15%2Ctext_5Luj56CB6ZqP5oOz5b2V55-l6K-G5pif55CD%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10)
**内存池架构图**，三个层级，内存池通过分层缓存架构来管理内存，旨在优化**内存分配和释放**的性能，特别是在多线程环境下。

1. ThreadCache（线程本地缓存）
+ 每个线程独立的内存缓存
+ 无锁操作，快速分配和释放
+ 减少线程间竞争，提高并发性
2. CentralCache 中心缓存
+ 管理多个线程共享的内存块
+ 通过自旋锁保护，确保线程安全
+ 批量从pagecache获取内存，分配给ThreadCache
3. PageCache 页缓存
+ 从操作系统获取大块内存
+ 将大块内存切分为小块，供CentralCache使用
+ 负责内存的回收和再利用


