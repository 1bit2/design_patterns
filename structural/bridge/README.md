# Bridge Pattern / 桥接模式

**分类**: 结构型模式 (Structural)

## 解决的问题

当一个类存在两个独立变化的维度（如形状 × 渲染平台），
纯继承会导致类数量爆炸（M × N 个子类）。

桥接模式将**抽象**和**实现**分离到两个独立的继承体系中，
用组合连接它们，使类数量从 M × N 降到 M + N。

## 示例说明

本目录中的 `bridge.cpp` 以**跨平台图形渲染**为例：

- `Renderer` — 实现层接口（Implementor）
- `OpenGLRenderer` / `VulkanRenderer` / `SoftwareRenderer` — 具体渲染实现
- `Shape` — 抽象层（Abstraction），持有 Renderer 引用
- `Circle` / `Rectangle` — 具体形状（RefinedAbstraction）

2 种形状 × 3 种渲染器 = 只需 5 个类（2 + 3），而非 6 个子类。

## 编译运行

```bash
g++ -std=c++17 -o bridge bridge.cpp && ./bridge
```

## 要点

- 抽象（Shape）和实现（Renderer）可以**独立扩展**
- 运行时可切换实现（`shape.setRenderer(vulkan)`）
- 新增形状只需继承 Shape，新增平台只需继承 Renderer
