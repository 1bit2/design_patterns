/**
 * ============================================================================
 * 备忘录模式 (Memento Pattern) - 文本编辑器撤销/重做
 * ============================================================================
 *
 * 【解决什么问题】
 *   在不破坏封装性的前提下，捕获并外部化对象的内部状态，
 *   以便以后可以将对象恢复到这个状态。
 *   典型场景：文本编辑器的撤销 (undo) / 重做 (redo)。
 *
 * 【三个角色】
 *   - Originator（原发器）TextEditor：
 *       拥有需要保存的状态（文本内容 + 光标位置），
 *       能创建备忘录 save() 和从备忘录恢复 restore()。
 *   - Memento（备忘录）EditorMemento：
 *       保存状态快照。状态只对 Originator 可见（宽接口），
 *       对 Caretaker 不可见（窄接口）。
 *   - Caretaker（管理者）CommandHistory：
 *       管理 undo/redo 两个栈，控制撤销/重做流程，
 *       但从不读取或修改备忘录内容。
 *
 * 【宽接口 / 窄接口分离（C++ 用 friend 实现）】
 *   - EditorMemento 的状态字段、构造函数、访问器全部 private，
 *     只有 friend TextEditor 能访问     → 宽接口。
 *   - Caretaker 只能持有、搬运、销毁备忘录，无法读取内部状态 → 窄接口。
 *   - 若在 CommandHistory 中写 memento.getText() 会编译失败（getText 是
 *     private 且 CommandHistory 不是 friend）→ 封装被强制保证。
 *
 * 【与命令模式对比】
 *   命令模式也支持撤销，但保存的是“操作”（execute/逆向 undo）；
 *   备忘录保存的是“状态快照”。当逆向操作难以计算或状态较小时，备忘录更简单。
 *   两者常配合使用：命令对象内部用备忘录保存执行前的状态。
 *
 * 编译：g++ -std=c++17 -Wall -Wextra -o memento memento.cpp
 * ============================================================================
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================================
// Memento：备忘录，保存 TextEditor 的状态快照
//
// 宽接口 / 窄接口分离：
//   - 状态字段 text_, cursor_ 全部 private
//   - 构造函数 private  → 只有 friend TextEditor 能创建备忘录
//   - 访问器 getText()/getCursor() private → 只有 TextEditor 能读取快照
//   → Caretaker 拿到 EditorMemento 后，无法读取或修改其内容，
//     只能原样保存 / 传回。这是“不破坏封装性”的强制保证。
//
// 拷贝/移动构造与赋值由编译器隐式生成（public），便于在栈中搬运；
// 因为成员是 string + size_t，默认行为正确（rule of zero）。
// ============================================================================

class EditorMemento {
    friend class TextEditor;  // 宽接口：仅原发器 TextEditor 可访问下面的 private 成员

private:
    std::string text_;     // 快照保存的文本内容
    std::size_t cursor_;   // 快照保存的光标位置

    // private 构造：外部无法 new/构造备忘录，只有 TextEditor（friend）能创建
    EditorMemento(std::string text, std::size_t cursor)
        : text_(std::move(text)), cursor_(cursor) {}

    // private 访问器：只有 TextEditor 能读取快照内容
    // Caretaker 调用 m.getText() 会编译失败（不是 friend）
    const std::string& getText() const { return text_; }
    std::size_t getCursor() const { return cursor_; }

public:
    // 窄接口：public 部分没有任何状态访问器
    // Caretaker 只能持有 / 拷贝 / 移动 / 销毁备忘录，看不到快照内容
    // 拷贝/移动构造由编译器隐式生成（rule of zero），便于在 vector 中搬运
};

// ============================================================================
// Originator：原发器，拥有需要被保存/恢复的状态
// ============================================================================

class TextEditor {
public:
    TextEditor() = default;

    // ---- 编辑操作（改变状态）----

    // 在光标位置插入文本，光标随之后移
    void type(const std::string& text) {
        text_.insert(cursor_, text);   // 在 cursor_ 处插入
        cursor_ += text.size();        // 光标后移到插入内容末尾
    }

    // 移动光标到 pos，超过末尾则停在末尾（防越界）
    void moveCursor(std::size_t pos) {
        cursor_ = std::min(pos, text_.size());  // 防越界
    }

    // 删除光标前的 count 个字符（退格键）
    void backspace(std::size_t count) {
        count = std::min(count, cursor_);       // 最多删到光标开头，不能删光标后的内容
        text_.erase(cursor_ - count, count);    // 从 (cursor_ - count) 处删 count 个字符
        cursor_ -= count;                        // 光标前移
    }

    // ---- 备忘录接口 ----

    // 创建当前状态的快照（宽接口：Originator 访问自己的状态，能调 private 构造）
    EditorMemento save() const {
        return EditorMemento(text_, cursor_);   // friend 才能调用 private 构造
    }

    // 从快照恢复状态（friend 才能访问 EditorMemento 的 private 字段 m.text_）
    void restore(EditorMemento m) {
        text_ = std::move(m.text_);   // 移动语义，避免深拷贝
        cursor_ = m.cursor_;         // size_t 直接拷贝
    }

    // ---- 展示（读自己的状态，与备忘录无关）----
    void print(const std::string& label) const {
        std::cout << "  " << label << ": text=\"" << text_
                  << "\"  cursor=" << cursor_ << "\n";
    }

private:
    std::string text_;
    std::size_t cursor_ = 0;
};

// ============================================================================
// Caretaker：备忘录管理者，用两个栈管理 undo/redo
//
// 职责：
//   - checkpoint：编辑前把当前状态压入 undo 栈，并清空 redo 栈
//                （新的编辑会使 redo 历史失效）
//   - undo：当前状态入 redo 栈，弹出 undo 栈顶并恢复
//   - redo：当前状态入 undo 栈，弹出 redo 栈顶并恢复
//
// 关键：Caretaker 只“持有 / 搬运”备忘录，从不读取其内容。
//       它调用 editor.save() 得到不透明快照存入栈，
//       undo 时把快照原样交回 editor.restore()。
//       例如在下面的方法里写 m.getText() 会编译失败
//       （getText 是 private，且 CommandHistory 不是 EditorMemento 的 friend）。
// ============================================================================

class CommandHistory {
public:
    // 编辑前调用：保存当前状态到 undo 栈，清空 redo 栈
    // 为什么清空 redo？因为新编辑会分叉，旧的"前进"历史不再有效
    void checkpoint(const TextEditor& editor) {
        undoStack_.push_back(editor.save());  // 拿到不透明快照存入栈（不读内容）
        redoStack_.clear();                    // 新编辑使 redo 历史失效
    }

    // 撤销：当前状态进 redo 栈 → 弹出 undo 栈顶 → 恢复
    // 关键记忆点：谁被弹出，当前状态就进对面的栈
    bool undo(TextEditor& editor) {
        if (undoStack_.empty()) {
            std::cout << "    (undo 栈为空，无可撤销操作)\n";
            return false;
        }
        redoStack_.push_back(editor.save());   // ① 当前状态存入 redo（供将来 redo）
        EditorMemento m = std::move(undoStack_.back());  // ② 取出 undo 栈顶快照
        undoStack_.pop_back();                            // ③ 弹栈
        editor.restore(std::move(m));          // ④ 用快照恢复编辑器状态
        return true;
    }

    // 重做：undo 的镜像——当前状态进 undo 栈 → 弹出 redo 栈顶 → 恢复
    bool redo(TextEditor& editor) {
        if (redoStack_.empty()) {
            std::cout << "    (redo 栈为空，无可重做操作)\n";
            return false;
        }
        undoStack_.push_back(editor.save());   // ① 当前状态存入 undo（供将来再 undo）
        EditorMemento m = std::move(redoStack_.back());  // ② 取出 redo 栈顶快照
        redoStack_.pop_back();                            // ③ 弹栈
        editor.restore(std::move(m));          // ④ 用快照恢复编辑器状态
        return true;
    }

    // 展示栈规模：Caretaker 只知道“有几个快照”，不知道快照内容
    void printStatus() const {
        std::cout << "    [history] undo 栈=" << undoStack_.size()
                  << " 个快照, redo 栈=" << redoStack_.size()
                  << " 个快照\n";
    }

private:
    // 用 vector 当栈：push_back 压栈，back 看栈顶，pop_back 弹栈
    std::vector<EditorMemento> undoStack_;  // 撤销栈：保存历史状态，undo 时从这里弹出
    std::vector<EditorMemento> redoStack_;  // 重做栈：保存"被撤销"的状态，redo 时从这里弹出
};

// ============================================================================
// 辅助：打印小节标题
// ============================================================================
static void printSection(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

// ============================================================================
// Main：演示 编辑 → 撤销 → 重做 → 新编辑使 redo 失效
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "####################################################\n";
    std::cout << "#  Memento Pattern - Text Editor Undo/Redo          #\n";
    std::cout << "####################################################\n";

    TextEditor editor;
    CommandHistory history;

    printSection("初始状态");
    editor.print("now  ");
    history.printStatus();

    // ---- 编辑 1：输入 Hello ----
    printSection("编辑1：输入 \"Hello\"");
    history.checkpoint(editor);
    editor.type("Hello");
    editor.print("now  ");
    history.printStatus();

    // ---- 编辑 2：追加 " World" ----
    printSection("编辑2：追加 \" World\"");
    history.checkpoint(editor);
    editor.type(" World");
    editor.print("now  ");
    history.printStatus();

    // ---- 编辑 3：退格删除末尾 6 个字符（" World"）----
    printSection("编辑3：退格删除末尾 6 个字符");
    history.checkpoint(editor);
    editor.backspace(6);
    editor.print("now  ");
    history.printStatus();

    // ---- 连续撤销 ----
    printSection("撤销1（回到编辑2的结果 \"Hello World\"）");
    history.undo(editor);
    editor.print("now  ");
    history.printStatus();

    printSection("撤销2（回到编辑1的结果 \"Hello\"）");
    history.undo(editor);
    editor.print("now  ");
    history.printStatus();

    printSection("撤销3（回到初始状态 \"\"）");
    history.undo(editor);
    editor.print("now  ");
    history.printStatus();

    printSection("撤销4（undo 栈已空）");
    history.undo(editor);
    history.printStatus();

    // ---- 连续重做 ----
    printSection("重做1（恢复编辑1结果 \"Hello\"）");
    history.redo(editor);
    editor.print("now  ");
    history.printStatus();

    printSection("重做2（恢复编辑2结果 \"Hello World\"）");
    history.redo(editor);
    editor.print("now  ");
    history.printStatus();

    printSection("重做3（恢复编辑3结果 \"Hello\"）");
    history.redo(editor);
    editor.print("now  ");
    history.printStatus();

    printSection("重做4（redo 栈已空）");
    history.redo(editor);
    history.printStatus();

    // ---- 新编辑使 redo 历史失效 ----
    printSection("先撤销一步到编辑2结果 \"Hello World\"");
    history.undo(editor);
    editor.print("now  ");
    history.printStatus();

    printSection("新编辑4：追加 \"!!!\"（redo 栈应被清空）");
    history.checkpoint(editor);
    editor.type("!!!");
    editor.print("now  ");
    history.printStatus();

    printSection("撤销（只能回到 \"Hello World\"，无法再 redo 到旧的编辑3）");
    history.undo(editor);
    editor.print("now  ");
    history.printStatus();

    printSection("重做（只能恢复刚输入的 \"!!!\"）");
    history.redo(editor);
    editor.print("now  ");
    history.printStatus();

    // ---- 总结 ----
    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "总结\n";
    std::cout << "====================================================\n";
    std::cout << "  - EditorMemento 状态对 Caretaker 完全不可见（friend 隔离）\n";
    std::cout << "  - undo/redo 用两个栈管理快照，Caretaker 只搬运不读取\n";
    std::cout << "  - 新编辑使 redo 历史失效（checkpoint 清空 redo 栈）\n";
    std::cout << "  - 保存的是\"状态快照\"，而非\"逆向操作\"（与命令模式区别）\n";

    return 0;
}
