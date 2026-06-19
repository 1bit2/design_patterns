# Builder Pattern / 建造者模式

**分类**: 创建型模式 (Creational)

## 解决的问题

当一个复杂对象的创建需要多个步骤，且步骤的组合方式不同时，
直接使用构造函数会导致参数爆炸（"伸缩式构造函数"反模式）。

建造者模式将**构建过程**与**内部表示**分离，使得同样的构建过程
可以创建不同的表示。

## 示例说明

本目录中的 `builder.cpp` 以**组装电脑**为例：

- `Computer` — 复杂产品（CPU、GPU、内存、硬盘、系统）
- `ComputerBuilder` — 抽象建造者接口（链式调用）
- `GamingComputerBuilder` — 游戏电脑建造者
- `OfficeComputerBuilder` — 办公电脑建造者（自动覆盖 GPU、追加 Enterprise 后缀）
- `Director` — 指导者，提供预定义配置（可选）

## 编译运行

```bash
g++ -std=c++17 -o builder builder.cpp && ./builder
```

## 要点

- 同样的构建步骤，不同 Builder 实现 → 不同产品
- 链式调用（fluent interface）让构建过程直观可读
- Director 不是必须的，可以直接用 Builder 链式构建
