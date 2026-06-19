# Design Patterns / 设计模式

本仓库按 GoF（Gang of Four）分类收录常用设计模式的 C++ 示例代码。

## 设计模式三大分类

GoF 在《设计模式》中将 23 种设计模式按**关注点**分为三大类：

### 创建型模式 (Creational) — 对象怎么"生出来"

将对象的**创建逻辑**与**使用逻辑**分离，让系统不依赖具体类的实例化方式。

- [ ] [建造者 (Builder)](creational/builder/) — 一步步组装复杂对象 `代码 ✅`
- [ ] 工厂方法 (Factory Method) — 一个方法决定创建哪种产品
- [ ] 抽象工厂 (Abstract Factory) — 一个工厂生产一整套配套产品
- [ ] 单例 (Singleton) — 全局只有一个实例
- [ ] 原型 (Prototype) — 通过克隆已有对象创建新对象

**类比**：创建型关心的是"去哪家工厂、用什么方式拿到一辆车"。

→ 详见 [creational/README.md](creational/README.md)

---

### 结构型模式 (Structural) — 对象怎么"拼在一起"

通过**组合、聚合、包装**等方式，把类和对象组装成更大的结构，同时保持灵活。

- [ ] [组合 (Composite)](structural/composite/) — 树形结构，叶子和容器统一接口 `代码 ✅`
- [ ] [桥接 (Bridge)](structural/bridge/) — 拆分抽象和实现，独立变化 `代码 ✅`
- [ ] 适配器 (Adapter) — 接口转换，让不兼容的类能协作
- [ ] 装饰器 (Decorator) — 动态给对象加功能，替代继承
- [ ] 外观 (Facade) — 给复杂子系统一个简单入口
- [ ] 享元 (Flyweight) — 共享细粒度对象，节省内存
- [ ] 代理 (Proxy) — 控制对目标对象的访问

**类比**：结构型关心的是"零件怎么组装成一辆车"。

→ 详见 [structural/README.md](structural/README.md)

---

### 行为型模式 (Behavioral) — 对象之间怎么"协作通信"

定义对象之间的**交互方式、职责划分和算法分配**。

- [ ] [中介者 (Mediator)](behavioral/mediator/) — 中介集中管理多对象通信 `代码 ✅`
- [ ] 观察者 (Observer) — 一对多依赖，状态变更自动通知
- [ ] 策略 (Strategy) — 算法族可互换
- [ ] 命令 (Command) — 把请求封装为对象
- [ ] 模板方法 (Template Method) — 定义骨架，子类填充步骤
- [ ] 迭代器 (Iterator) — 顺序访问聚合元素
- [ ] 状态 (State) — 状态改变行为
- [ ] 职责链 (Chain of Responsibility) — 请求沿链传递直到有人处理
- [ ] 访问者 (Visitor) — 不修改类的前提下新增操作
- [ ] 备忘录 (Memento) — 保存和恢复对象状态
- [ ] 解释器 (Interpreter) — 定义语言语法并解释执行

**类比**：行为型关心的是"车造好之后，司机、导航、交通灯之间怎么配合"。

→ 详见 [behavioral/README.md](behavioral/README.md)

## 总结

```
创建型 → 对象的"出生"     （怎么造）
结构型 → 对象的"组装"     （怎么拼）
行为型 → 对象的"协作"     （怎么配合干活）
```

三者不是互斥的，实际项目中经常混合使用。

## 编译运行

```bash
cmake -B build && cmake --build build
./build/builder
./build/composite
./build/bridge
./build/mediator
```
