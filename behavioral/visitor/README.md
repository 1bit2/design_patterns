# Visitor Pattern / 访问者模式

**分类**: 行为型模式 (Behavioral)

## 一句话总结

把"对一组稳定对象结构的操作"抽离到独立的访问者类中，通过双重分派在不修改元素类的前提下新增操作。

## 解决的问题

访问者模式最关键的特征是一个**权衡**：**新增操作容易，新增元素类型困难**。下面用代码对比说清楚这一点。

### 场景：文件系统由 File + Directory 构成，需要对其执行多种操作

（算总大小、按名搜索、导出文本树……）

### 没有访问者模式 — 把每个操作做成 Element 的成员方法

```cpp
class File : public Element {
    std::string name_;
    size_t size_;
public:
    // 算大小：File 自己实现
    size_t getSize() const { return size_; }
    // 搜索：File 实现一份
    bool nameContains(const std::string& kw) const {
        return name_.find(kw) != std::string::npos;
    }
    // 导出：File 实现一份
    void exportTo(std::ostream& os, const std::string& indent) const {
        os << indent << name_ << " (" << size_ << " KB)\n";
    }
};

class Directory : public Element {
    std::string name_;
    std::vector<std::shared_ptr<Element>> children_;
public:
    // 算大小：Directory 递归实现
    size_t getSize() const {
        size_t total = 0;
        for (const auto& c : children_) total += c->getSize();
        return total;
    }
    // 搜索：Directory 递归实现
    bool nameContains(const std::string& kw) const { /* 递归子节点 */ }
    // 导出：Directory 递归实现
    void exportTo(std::ostream& os, const std::string& indent) const { /* 递归子节点 */ }
};
```

**痛点**：

- 每新增一个操作（比如"找最大文件"），就要在 `File` **和** `Directory` 两个类里各加一个方法 ——
  元素类型越多，改动面越大
- `Element` 类越来越胖，承担了算大小 / 搜索 / 导出等与"自身存储职责"无关的操作
- 这些操作互相独立却被塞进同一个类，职责混乱

### 有访问者模式 — 操作外置到 Visitor，元素类不动

```cpp
// 元素类：只保留最小接口（accept + getName），不再包含任何"操作"
class File : public Element {
public:
    void accept(Visitor& v) const override { v.visit(*this); }   // 双重分派入口
    std::string getName() const override;
    size_t getSize() const;           // 自身数据，供 visitor 读取
};

class Directory : public Element {
public:
    void accept(Visitor& v) const override { v.visit(*this); }
    const std::vector<std::shared_ptr<Element>>& getChildren() const;
};

// 每个操作 = 一个独立的 Visitor 子类
class SizeCalculatorVisitor : public Visitor {
    void visit(const File& f)      override { total_ += f.getSize(); }
    void visit(const Directory& d) override { for (auto& c : d.getChildren()) c->accept(*this); }
};
class SearchVisitor   : public Visitor { /* ... */ };
class ExportVisitor    : public Visitor { /* ... */ };
```

**新增操作（容易 ✅）**：想加"找最大文件"，只需写一个 `LargestFileVisitor` 子类。
`File` / `Directory` / `Visitor` 基类**全部不动**。本例的三个 Visitor 就是这样独立加上去的。

**新增元素类型（困难 ❌）**：想加一个 `Symlink` 元素，必须：

1. 在 `Visitor` 基类加 `virtual void visit(const Symlink&) = 0;`
2. 在 `SizeCalculatorVisitor` 实现 `visit(Symlink)`
3. 在 `SearchVisitor` 实现 `visit(Symlink)`
4. 在 `ExportVisitor` 实现 `visit(Symlink)`

—— 每一个已有 Visitor 都要改。已有 Visitor 越多，改动越大。

```
新增操作   = 新增一个 Visitor 子类       → 元素类 0 改动   (模式的收益)
新增元素   = 改 Visitor 基类 + 所有 Visitor → 全部 Visitor 都改  (模式的代价)
```

### 适用判断

> 元素类型稳定、操作频繁变化 → 用访问者模式；
> 元素类型经常新增 → 不要用，否则每次加元素都要改所有 Visitor。

## 实现说明

### 类结构

```
Element (抽象)              Visitor (抽象)
  │ accept(Visitor&)          │ visit(const File&)
  │ getName()                 │ visit(const Directory&)
  ├─────────────┐            ▲
  │             │            │ (实现)
File          Directory    SizeCalculatorVisitor
                            SearchVisitor
                            ExportVisitor
```

- `Element` 只暴露 `accept(Visitor&)` 和 `getName()`，**刻意不写** `getSize()`（目录总大小）/ 搜索 / 导出等操作 —— 这些操作全部交给 Visitor 从外部添加
- `File` 自身数据（`size_`）通过 `getSize()` 暴露给 visitor 读取；`Directory` 通过 `getChildren()` 暴露子节点，让 visitor 自行驱动遍历
- 本例的 Directory 与组合模式结合（树形结构），访问者作用在组合树上

### 关键设计决策：双重分派 (Double Dispatch)

#### 先搞清楚：为什么不直接 `visitor.visit(element)`？

```cpp
SizeCalculatorVisitor sizeVisitor;
Element* elem = new File("a.txt", 10);   // 静态类型 Element*，实际指向 File

sizeVisitor.visit(*elem);   // ❌ 编译失败！
```

**为什么失败？** `*elem` 的静态类型是 `Element&`，而 Visitor 只重载了 `visit(const File&)` 和 `visit(const Directory&)`，没有 `visit(const Element&)` 这个重载，编译器找不到匹配。

那加一个 `visit(const Element&)` 行不行？也不行——那样所有元素都走同一个函数，无法按 File / Directory 分别处理。

**根本原因**：C++ 的虚函数是**单分派**——一次虚调用只按一个对象的实际类型绑定，`arg` 的**静态类型**在编译期就决定调用哪个重载，无法在运行时根据 `arg` 的实际类型分派。

#### 双重分派：两次虚调用，各拿一个对象的实际类型

```cpp
elem->accept(sizeVisitor);
    │
    │  第 1 次分派：按 elem 的实际类型 → File
    ▼
void File::accept(Visitor& visitor) const override {   // 进入 File::accept
    visitor.visit(*this);   // 此时 *this 的静态类型是 const File&
}
    │
    │  编译器看 *this 静态类型 = const File&
    │  → 选中 visit(const File&) 重载（不是 visit(Directory)）
    │  第 2 次分派：按 visitor 的实际类型 → SizeCalculatorVisitor
    ▼
void SizeCalculatorVisitor::visit(const File& file) {
    total_ += file.getSize();   // ✅ 命中"对 File 执行算大小"
}
```

- 第 1 次分派拿到 **元素的实际类型**（File）
- 第 2 次分派拿到 **访问者的实际类型**（SizeCalculatorVisitor）
- 两者结合，精确命中"对 File 执行算大小"的方法，无需 if/else 或 RTTI

#### 通俗类比

| 角色 | 类比 |
|-----|------|
| Element（File / Directory） | 两种不同的**病人** |
| Visitor（Size / Search / Export） | 三种不同的**医生** |
| `accept(visitor)` | 病人**挂号**：亮明自己是哪种病人（第 1 次分派） |
| `visitor.visit(*this)` | 医生**接诊**：亮明自己是哪个科室（第 2 次分派） |

- 单分派：只有病人说"我牙痛"，但医生不知道自己该干什么
- 双重分派：病人挂号（→ 牙痛）→ 对应医生接诊（→ 牙医处理牙痛）

#### 其他设计决策

- `accept` 是 `const`，因为访问只读元素、不改元素；状态变化都发生在 visitor 内部
- 访问者自己驱动遍历（`visit(Directory)` 内 `child->accept(*this)`），而非由元素驱动 —— 这样遍历策略可随 visitor 变化，更灵活

### 本例三个具体访问者

| 访问者 | visit(File) | visit(Directory) | 说明 |
|--------|-------------|------------------|------|
| `SizeCalculatorVisitor` | 累加文件大小 | 递归子节点 | "算目录总大小"操作完全在 visitor 里，Directory 不参与 |
| `SearchVisitor` | 子串匹配后收集 | 递归子节点 | 按名搜索，结果收集在 visitor 内部 |
| `ExportVisitor` | 输出文件行 | 输出目录行 + 递归子节点 | 用栈记录每层"是否末位"绘制 `|--`/`` `-- `` 连接线 |

## 编译运行

```bash
g++ -std=c++17 -Wall -Wextra -o visitor visitor.cpp && ./visitor
```

预期输出片段：

```
[1] Visitor #1: SizeCalculatorVisitor
  total size : 602 KB
  file count : 9
  dir  count : 5

[2] Visitor #2: SearchVisitor (keyword = "utils")
  found: utils.cpp
  found: utils.h

[3] Visitor #3: ExportVisitor
project/
|-- src/
|   |-- main.cpp (12 KB)
|   |-- utils.cpp (8 KB)
|   `-- utils.h (3 KB)
...
```

## 要点

- **操作外置** — 把"做什么操作"从元素类抽离到独立 Visitor，元素类保持精简、职责单一
- **双重分派** — 两次虚调用实现"按元素类型 + 操作类型"分派，无需 if/else 或 RTTI
- **新增操作零侵入** — 新操作 = 新 Visitor 子类，元素类与 Visitor 基类都不动（模式的收益）
- **新增元素类型代价大** — 加新元素要改 Visitor 基类 + 所有已有 Visitor（模式的代价），故适用于元素类型稳定、操作频繁变化的场景

## 与其他模式的关系

- **迭代器模式**：迭代器把"遍历"逻辑抽离（只管遍历，不管做什么）；访问者把"操作"逻辑抽离（对每个元素做什么由 visitor 决定）。访问者常自带遍历（如本例 `visit(Directory)` 递归 `child->accept`），也可配合迭代器先遍历再操作
- **组合模式**：组合让叶子和容器共享接口，访问者常作用在组合构建的树形结构上（本例 `File`/`Directory` 即组合结构）。访问者负责"对节点做什么"，组合负责"如何组织节点"
- **与桥接 / 中介者的区别**：访问者关注"在不改类的前提下新增操作"（行为型）；桥接关注"多维度独立变化"（结构型）；中介者关注"多对象通信解耦"（行为型）
