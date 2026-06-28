# Memento Pattern / 备忘录模式

**分类**: 行为型模式 (Behavioral)

## 一句话总结

在不破坏封装性的前提下，捕获并外部化对象的内部状态，以便以后能把对象恢复到这个状态。

## 解决的问题

### 没有 备忘录模式 — 为支持撤销，被迫把内部状态公开

文本编辑器要做撤销/重做，需要把“过去某个时刻的状态”存起来再恢复。
如果直接把编辑器的字段公开给历史管理器，就会出现下面的封装破坏：

```cpp
// 没有备忘录模式：历史管理器直接读写编辑器的内部字段
class TextEditor {
public:
    std::string text;        // 被迫公开内部状态
    std::size_t cursor;
};

class History {
public:
    void save(const TextEditor& e) {
        snapshots_.push_back({e.text, e.cursor});   // History 直接读 text/cursor
    }
    void undo(TextEditor& e) {
        auto s = snapshots_.back(); snapshots_.pop_back();
        e.text   = s.text;      // History 直接写回 text
        e.cursor = s.cursor;    // History 直接写回 cursor
    }
private:
    struct Snapshot { std::string text; std::size_t cursor; };
    std::vector<Snapshot> snapshots_;
};
```

**痛点（具体表现，不是含糊的“耦合”）**：

1. **History 直接依赖 `text` / `cursor` 字段名**。
   一旦 `TextEditor` 内部表示变化，History 必须同步修改：
   - `text` 从 `std::string` 换成 `rope` 或分块存储 → `snapshots_` 的存储和 `undo` 里
     `e.text = s.text` 都要改；
   - 新增状态字段（如选区 `selectionStart`）→ 必须记得在 `Snapshot`、`save`、`undo`
     三处都补上，遗漏一处就会“撤销时选区没恢复”，状态不一致。

2. **History 有能力伪造非法状态**。
   它可以直接写 `e.cursor = 999`（即使 `text` 只有 5 个字符），破坏 `TextEditor`
   的不变式（`cursor <= text.size()`）。模式想约束的“只有编辑器自己懂自己的合法状态”，
   在这里失去了保证。

注意：这是“可以伪造”而非“必然伪造”——只要写代码小心也能不出错，
但模式的价值正是把“不出错”从“靠人小心”变成“靠类型系统强制”。

### 有备忘录模式 — 状态封装在 Memento 里，只对原发器可见

```cpp
// 有备忘录模式：状态封装在 EditorMemento 中，只对 friend TextEditor 可见
class EditorMemento {
    friend class TextEditor;          // 宽接口：仅原发器可访问
private:
    std::string text_;
    std::size_t cursor_;
    EditorMemento(std::string t, std::size_t c);  // private 构造
    const std::string& getText() const;          // private 访问器
    std::size_t getCursor() const;
public:
    // 窄接口：无状态访问器，Caretaker 看不到内容
};

class TextEditor {
public:
    EditorMemento save() const;       // 创建快照（自己访问自己的状态）
    void restore(EditorMemento m);     // 从快照恢复（friend 才能读 m）
    void type(const std::string& s);
    void backspace(std::size_t n);
private:
    std::string text_;
    std::size_t cursor_ = 0;
};

class CommandHistory {
public:
    void checkpoint(const TextEditor& e) {
        undoStack_.push_back(e.save());   // 拿到不透明快照，存进栈
        redoStack_.clear();               // 新编辑使 redo 历史失效
    }
    bool undo(TextEditor& e) {
        if (undoStack_.empty()) return false;
        redoStack_.push_back(e.save());
        EditorMemento m = std::move(undoStack_.back());
        undoStack_.pop_back();
        e.restore(std::move(m));          // 原样交回，History 不知道内容
        return true;
    }
    // redo 同理……
private:
    std::vector<EditorMemento> undoStack_;
    std::vector<EditorMemento> redoStack_;
};
```

**优势（对应上面的痛点）**：

```
没有备忘录：History 直接读写 Originator 的私有字段
          → 内部表示变化时 History 必须改；History 可伪造非法状态
有备忘录：  状态封装在 EditorMemento 里，History 只搬运不读取
          → 内部表示变化只影响 EditorMemento 和 TextEditor 自己，不影响 Caretaker；
            History 拿不到状态，自然无法伪造非法状态（在 Caretaker 里写
            m.getText() 会编译失败：'getText() const' is private within this context）
```

**必然与偶然**：`TextEditor` 内部表示变化，必然要改 `EditorMemento` 的字段和
`TextEditor::save/restore` 的实现——这是 Originator 自己的事；
而 `CommandHistory` 不用改，是模式带来的收益。

## 实现说明

### 类结构（三个角色）

| 角色 | 类 | 职责 |
|------|----|----|
| Originator（原发器） | `TextEditor` | 拥有状态（text + cursor），提供 `save()` 创建快照、`restore()` 恢复快照 |
| Memento（备忘录） | `EditorMemento` | 保存状态快照，状态只对 `TextEditor` 可见 |
| Caretaker（管理者） | `CommandHistory` | 管理 undo/redo 两个栈，控制撤销/重做流程，从不读取快照内容 |

### 关键设计决策

1. **宽接口 / 窄接口分离，用 `friend` 实现**
   `EditorMemento` 的状态字段、构造函数、访问器全部 `private`，
   只声明 `friend class TextEditor`。
   - **宽接口**（`getText/getCursor`/构造）：只有 `TextEditor` 能访问；
   - **窄接口**：`Caretaker` 只能持有、移动、销毁备忘录，看不到内容。
   这是“不破坏封装性”的**类型系统级强制**，不是靠注释或自觉。

2. **`private` 构造函数**
   只有 `TextEditor`（friend）能构造 `EditorMemento`，
   `Caretaker` 无法 `new`/构造一个备忘录——它只能接收 `editor.save()` 产生的不透明快照。

3. **两个栈管理 undo/redo（`std::vector` 当栈用）**
   - `checkpoint()`：编辑前把当前状态压入 undo 栈，并清空 redo 栈
     （新编辑使 redo 历史失效）；
   - `undo()`：当前状态入 redo 栈 → 弹出 undo 栈顶 → `restore`；
   - `redo()`：当前状态入 undo 栈 → 弹出 redo 栈顶 → `restore`。

4. **快照按值存储 + 移动语义**
   `EditorMemento` 内含 `std::string`，按值存入 `vector`，用 `std::move` 搬运，
   避免深拷贝。遵循 rule of zero——成员是 `string + size_t`，默认拷贝/移动行为正确，
   无需手写特殊成员函数。

5. **checkpoint-before-edit 语义**
   由调用方（`main`）在每次编辑前显式 `checkpoint(editor)`，
   体现 GoF 的核心约束：**Caretaker 决定何时快照**，Originator 只负责“怎么存/怎么恢复”。

### 双栈状态追踪（对应 main 的演示流程）

三条规则（关键记忆点：**谁被弹出，当前状态就进对面的栈**）：

```
checkpoint（编辑前）：当前状态 → undo 栈，清空 redo 栈
undo：  当前状态 → redo 栈，弹出 undo 栈顶 → restore
redo：  当前状态 → undo 栈，弹出 redo 栈顶 → restore
```

下表追踪 main 的完整流程（`text` 为当前编辑器内容，栈内容按底→顶排列）：

| 步骤 | 操作 | text | undo 栈 | redo 栈 |
|------|------|------|---------|---------|
| 初始 | — | `""` | `[]` | `[]` |
| 1 | checkpoint + type("Hello") | `Hello` | `[""]` | `[]` |
| 2 | checkpoint + type(" World") | `Hello World` | `["","Hello"]` | `[]` |
| 3 | checkpoint + backspace(6) | `Hello` | `["","Hello","Hello World"]` | `[]` |
| 4 | undo | `Hello World` | `["","Hello"]` | `["Hello"]` |
| 5 | undo | `Hello` | `[""]` | `["Hello","Hello World"]` |
| 6 | undo | `""` | `[]` | `["Hello","Hello World","Hello"]` |
| 7 | undo（栈空） | `""` | `[]` | 不变 |
| 8 | redo | `Hello` | `[""]` | `["Hello","Hello World"]` |
| 9 | redo | `Hello World` | `["","Hello"]` | `["Hello"]` |
| 10 | redo | `Hello` | `["","Hello","Hello World"]` | `[]` |
| 11 | undo | `Hello World` | `["","Hello"]` | `["Hello"]` |
| 12 | checkpoint + type("!!!") | `Hello World!!!` | `["","Hello","Hello World"]` | `[]`（被清空） |
| 13 | undo | `Hello World` | `["","Hello"]` | `["Hello World!!!"]` |
| 14 | redo | `Hello World!!!` | `["","Hello","Hello World"]` | `[]` |

重点观察：

- **步骤 4-6**：undo 时当前状态进 redo 栈，undo 栈弹出后 restore
- **步骤 8-10**：redo 是 undo 的镜像——当前状态进 undo 栈，redo 栈弹出后 restore
- **步骤 12**：新编辑触发 checkpoint，**redo 栈被清空**，步骤 10 之前"Hello"分支的历史消失
- **步骤 13-14**：只能在新分支内 undo/redo，回不到步骤 11 之前的状态

## 编译运行

```bash
g++ -std=c++17 -Wall -Wextra -o memento memento.cpp && ./memento
```

零警告，自包含，不依赖外部库。

运行输出会演示：编辑 → 多次撤销 → 多次重做 → 新编辑使 redo 历史失效，
并实时打印编辑器状态与两个栈的快照数量。

## 要点

- **封装保护** — 备忘录状态对 Caretaker 不可见（`friend` 在编译期强制，写 `m.getText()` 直接编译失败）
- **职责分离** — Originator 管“怎么存/怎么恢复”，Caretaker 管“何时存/何时恢复”，两者解耦
- **撤销实现简单** — 保存“状态快照”即可，无需为每种操作计算“逆向操作”（删除的逆向是插入？格式化的逆向是什么？快照法一视同仁）
- **代价是内存** — 每次快照占内存；大对象场景需考虑增量快照或差异快照（本示例为教学，用全量快照）

## 与其他模式的关系

- **命令模式 (Command)**：两者都常用于实现撤销。
  - 命令模式保存“操作”（`execute` / `undo` 逆向执行），更细粒度、可组合成宏命令；
  - 备忘录保存“状态快照”，更简单，无需推导逆向操作。
  - 两者常配合：命令对象内部用备忘录保存执行前的状态，从而既能撤销、又能封装内部表示。
  本项目 `behavioral/command/` 有命令模式的示例。

- **状态模式 (State)**：状态模式让对象“随当前状态改变行为”；备忘录是“保存/恢复状态历史”，
  不改变行为。状态模式的状态对象本身也可以用备忘录来存档/回放。

- **迭代器模式 (Iterator)**：备忘录的历史栈可看成对“过去状态序列”的遍历（undo/redo 沿历史移动），
  但备忘录关注“保存与恢复”，迭代器关注“顺序访问”，关注点不同。
