# Composite Pattern / 组合模式

**分类**: 结构型模式 (Structural)

## 解决的问题

当系统存在"部分-整体"的树形层次结构时，如果客户端用 if/else
区分叶子节点和容器节点，代码复杂且难以扩展。

组合模式让**叶子和容器实现同一接口**，客户端无需区分即可统一操作。

## 示例说明

本目录中的 `composite.cpp` 以**文件系统**为例：

- `FileSystemNode` — 统一接口（Component）
- `File` — 叶子节点（没有子节点）
- `Directory` — 组合节点（包含子文件和子文件夹）
- `getSize()` 和 `print()` 递归操作整棵树，客户端不区分文件和文件夹

## 编译运行

```bash
g++ -std=c++17 -o composite composite.cpp && ./composite
```

## 要点

- 叶子和容器共享接口 → 客户端代码统一，无 if/else
- 递归组合 → `Directory.getSize()` 自动累加所有子节点
- 典型应用：文件系统、组织架构、GUI 组件树、表达式树
