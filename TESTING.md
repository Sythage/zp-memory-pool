# ZP Memory Pool - Testing Guide

这是一个 C++ 内存池项目，使用 Google Test (gtest) 进行单元测试。

## 项目结构

```
.
├── version1/
│   ├── ZPmemoryPool.h      # 内存池头文件
│   └── ZPmemoryPool.cc     # 内存池实现
├── test/
│   ├── test_main.cc        # 完整的测试套件
│   └── example_test.cc     # 简单的示例测试
├── CMakeLists.txt          # CMake 配置
├── vcpkg.json             # 依赖管理
└── README.md              # 本文档
```

## 构建和运行测试

### 前提条件

1. 安装 CMake (3.28+)
2. 安装 vcpkg 包管理器
3. 设置环境变量 `VCPKG_ROOT` 指向 vcpkg 安装目录

### 构建步骤

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# 构建项目
cmake --build .
```

### 运行测试

```bash
# 运行完整测试套件
./tests

# 运行简单示例测试
./example_tests
```

## 测试内容

### 基础测试 (example_test.cc)

1. **SimpleMemoryPoolUsage**: 测试 HashBucket 的基本内存分配和释放
2. **TemplateNewDeleteUsage**: 测试模板函数 `newElement()` 和 `deleteElement()`
3. **DirectMemoryPoolUsage**: 测试直接使用 MemoryPool 类

### 完整测试套件 (test_main.cc)

1. **BasicAllocation**: 基本的内存分配测试
2. **MultipleAllocations**: 多次内存分配测试
3. **AllocationDeallocationCycle**: 分配-释放循环测试
4. **HashBucketBasicUsage**: HashBucket 基本功能测试
5. **HashBucketLargeAllocation**: 大块内存分配测试
6. **HashBucketZeroSize**: 零大小分配测试
7. **NewDeleteElement**: 对象创建和销毁测试
8. **MultipleNewDeleteElements**: 批量对象创建和销毁测试
9. **ConcurrentAllocation**: 多线程并发分配测试
10. **ConcurrentHashBucket**: 多线程 HashBucket 测试
11. **PerformanceComparison**: 性能对比测试

## 如何编写新的测试

### 基本测试模板

```cpp
#include <gtest/gtest.h>
#include "ZPmemoryPool.h"

using namespace ZPmemoryPool;

TEST(YourTestSuite, YourTestCase) {
    // 初始化内存池系统
    HashBucket::initMemoryPool();
    
    // 你的测试代码
    void* ptr = HashBucket::useMemory(64);
    ASSERT_NE(ptr, nullptr);
    
    // 清理
    HashBucket::freeMemory(ptr, 64);
}
```

### 常用断言

- `ASSERT_EQ(a, b)`: a 等于 b
- `ASSERT_NE(a, b)`: a 不等于 b
- `ASSERT_TRUE(condition)`: 条件为真
- `ASSERT_FALSE(condition)`: 条件为假
- `EXPECT_*()`: 与 ASSERT 类似，但测试失败时继续执行

### 测试类模板

```cpp
class YourTestClass : public ::testing::Test {
protected:
    void SetUp() override {
        // 在每个测试前运行
        HashBucket::initMemoryPool();
    }
    
    void TearDown() override {
        // 在每个测试后运行（如果需要清理）
    }
};

TEST_F(YourTestClass, TestMethod) {
    // 使用 TEST_F 来使用测试类
}
```

## 内存池 API 使用指南

### 1. HashBucket（推荐使用）

```cpp
// 初始化内存池系统
HashBucket::initMemoryPool();

// 分配内存
void* ptr = HashBucket::useMemory(size);

// 释放内存
HashBucket::freeMemory(ptr, size);
```

### 2. 模板函数（面向对象）

```cpp
// 创建对象
MyClass* obj = newElement<MyClass>(constructor_args...);

// 销毁对象
deleteElement(obj);
```

### 3. 直接使用 MemoryPool

```cpp
MemoryPool pool(4096);  // 块大小
pool.init(64);          // 槽大小

void* ptr = pool.Allocate();
pool.Deallocate(ptr);
```

## VS Code 集成

项目已配置了 VS Code 任务：

- `Ctrl+Shift+P` → "Tasks: Run Task" → "Run Tests"

## 性能特点

- 内存池通常比标准 `new`/`delete` 更快
- 减少内存碎片
- 线程安全
- 支持不同大小的内存分配

## 故障排除

如果测试失败，检查以下项目：

1. 确保 vcpkg 正确安装并配置
2. 检查 CMake 版本是否符合要求
3. 确保正确调用了 `HashBucket::initMemoryPool()`
4. 检查内存分配大小是否在支持的范围内 (<=512 字节)
