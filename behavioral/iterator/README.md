# Iterator Pattern / 迭代器模式

**分类**: 行为型模式 (Behavioral)

## 一句话总结

把聚合对象的遍历逻辑，委托给独立的迭代器类。

## 解决的问题

### 问题1：没有迭代器模式 — 客户端可能直接依赖存储特有操作

```cpp
// 没有迭代器模式：客户端直接依赖存储结构
class Menu {
    std::vector<MenuItem> items_;  // 用 vector 存储
public:
    MenuItem& getItem(size_t i) { return items_[i]; }
    size_t size() const { return items_.size(); }
};

// 客户端遍历代码 — 直接依赖 vector 的索引访问
Menu menu;
for (size_t i = 0; i < menu.size(); i++) {   // 依赖索引
    menu.getItem(i).print();
}

// ---- 某天需求变化，存储从 vector 换成 list ----

class Menu {
    std::list<MenuItem> items_;  // 改成 list
    // list 不支持索引访问！
};

// 所有客户端代码都要改！
Menu menu;
for (auto it = menu.begin(); it != menu.end(); ++it) {  // 全部重写
    it->print();
}
```

**注意**：`list` 本身也提供 `begin()/end()`，如果 Menu 直接暴露 `begin()/end()`，
存储变化时客户端用 range-for 遍历也不需要改。但关键区别在于是否提供了统一抽象：

- 没有迭代器模式时，**没有统一抽象可用**，客户端容易直接依赖存储特有操作（如索引访问）；
  一旦存储变化，依赖这些特有操作的客户端代码必然需要修改
- 有迭代器模式时，提供统一迭代器接口，客户端自然通过它遍历，
  不会绕过去使用存储特有操作

### 有迭代器模式 — 客户端只依赖统一迭代器接口

```cpp
// 有迭代器模式：客户端只依赖 begin()/end() 统一接口
class VectorMenu {
    std::vector<MenuItem> items_;
public:
    using Iterator = MenuIterator<std::vector<MenuItem>::iterator>;
    Iterator begin() { return Iterator(items_.begin()); }
    Iterator end()   { return Iterator(items_.end()); }
};

class ListMenu {
    std::list<MenuItem> items_;
public:
    using Iterator = MenuIterator<std::list<MenuItem>::iterator>;
    Iterator begin() { return Iterator(items_.begin()); }
    Iterator end()   { return Iterator(items_.end()); }
};

// 注意：VectorMenu::Iterator 和 ListMenu::Iterator 是不同的具体类型，
// 客户端能统一调用是因为模板的鸭子类型（只要支持 begin()/end()/++/* 即可），
// 并非共享同一个基类。

// 客户端遍历代码 — 只依赖统一接口（C++17 模板写法）
template <typename Menu>
void printMenu(const Menu& menu) {
    for (auto it = menu.begin(); it != menu.end(); ++it) {
        it->print();
    }
}

// 存储从 vector 换成 list，客户端代码完全不用改
printMenu(vectorMenu);  // ✅ 不用改
printMenu(listMenu);    // ✅ 不用改
```

**关键区别**：

```
没有迭代器模式：存储变化 → 没有统一抽象可用，客户端依赖存储特有操作，需要改
有迭代器模式：存储变化 → 只改迭代器实现，客户端代码不变（通过统一接口隔离）
```

### 问题2：没有迭代器模式 — 无法支持嵌套遍历

```cpp
// 聚合自己维护遍历状态
class Menu {
    std::vector<MenuItem> items_;
    size_t currentIndex_ = 0;  // 只有一个遍历状态
public:
    bool hasNext() { return currentIndex_ < items_.size(); }
    MenuItem& next() { return items_[currentIndex_++]; }
    void reset() { currentIndex_ = 0; }
};

// 嵌套遍历 — 外层和内层共享同一个 currentIndex_，互相干扰！
Menu menu;
while (menu.hasNext()) {
    auto item1 = menu.next();

    // 想再遍历一遍？必须 reset，但这样外层遍历就乱了
    menu.reset();
    while (menu.hasNext()) {
        auto item2 = menu.next();
        // ...
    }
    // 内层遍历结束后，currentIndex_ == items_.size()
    // 回到外层时，hasNext() 返回 false，外层循环直接结束
    // 结果：外层只执行了一次，后续元素全部被跳过
}
```

### 有迭代器模式 — 每个迭代器独立维护状态

```cpp
// 每个迭代器有自己的状态，互不干扰
for (auto outer = menu.begin(); outer != menu.end(); ++outer) {
    for (auto inner = menu.begin(); inner != menu.end(); ++inner) {
        // outer 和 inner 各自维护自己的位置，互不影响
    }
}
```

### 问题3：没有迭代器模式 — 聚合职责过重

```cpp
// 聚合既要管存储，又要管各种遍历方式
class Menu {
    std::vector<MenuItem> items_;
public:
    // 存储相关
    void add(const MenuItem& item);
    void remove(size_t index);

    // 正向遍历
    bool hasNext();
    MenuItem& next();

    // 反向遍历
    bool hasPrev();
    MenuItem& prev();

    // 过滤遍历：只遍历价格 > minPrice 的
    bool hasNextExpensive(double minPrice);
    MenuItem& nextExpensive(double minPrice);

    // ... 每多一种遍历方式，聚合就多一堆方法
};
```

### 有迭代器模式 — 每种遍历策略一个迭代器类

```cpp
// 聚合只管存储
class Menu {
    std::vector<MenuItem> items_;
public:
    Iterator begin();                    // 正向遍历
    ReverseIterator rbegin();            // 反向遍历
    FilterIterator expensive(double p);  // 过滤遍历
};

// 每种遍历逻辑封装在独立的迭代器类中，职责清晰
```

### 总结

```
聚合对象：只管存储数据
迭代器：只管遍历逻辑
```

## 两种实现风格

### GoF 经典风格（Java 风格）

基于虚函数的多态，运行时绑定：

```cpp
Iterator* it = menu.createIterator();
for (it->first(); !it->isDone(); it->next()) {
    it->currentItem();
}
```

### STL 风格（C++ 风格）

基于模板 + 运算符重载，编译期绑定：

```cpp
for (auto it = menu.begin(); it != menu.end(); ++it) {
    *it;
}
```

**本质相同**：都是把遍历逻辑委托给迭代器类，只是接口风格不同。

## 示例说明

本目录中的 `iterator_storage_demo.cpp` 采用 **STL 风格**实现，
以餐厅菜单为例，对比 `vector` 和 `list` 两种存储方式：

- `MenuIterator<InnerIterator>` — 通用迭代器模板，包装底层迭代器，
  对外暴露统一的 `operator++` / `operator*` / `operator!=` 接口
- `VectorMenu` — 基于 `vector` 存储，`begin()/end()` 包装 `vector::iterator`
- `ListMenu` — 基于 `list` 存储，`begin()/end()` 包装 `list::iterator`

演示了以下功能：
- 统一遍历接口遍历不同存储类型的菜单
- 存储类型变化时，客户端代码不受影响
- 嵌套遍历（每个迭代器独立维护状态）

## 编译运行

```bash
g++ -std=c++17 -o iterator_storage_demo iterator_storage_demo.cpp && ./iterator_storage_demo
```

## 要点

- **职责分离** — 聚合管存储，迭代器管遍历
- **支持嵌套遍历** — 每个迭代器独立维护自己的遍历状态
- **存储变化不影响客户端** — 只改迭代器实现，客户端代码不变
- **典型应用** — STL 容器（`vector`、`list`、`map`）、数据库结果集、文件系统遍历

## 与其他模式的关系

- **组合模式**：迭代器可以遍历组合模式构建的树形结构
- **访问者模式**：访问者通常与迭代器结合使用，先遍历再操作
- **工厂方法**：聚合使用工厂方法创建迭代器
