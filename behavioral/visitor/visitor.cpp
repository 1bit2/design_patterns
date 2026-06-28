/**
 * ============================================================================
 * 访问者模式 (Visitor Pattern) - 行为型设计模式
 * ============================================================================
 *
 * 【解决的问题】
 *
 *   当一组稳定的对象结构（这里以文件系统为例：File + Directory）需要不断
 *   新增"操作"——算总大小、按名搜索、导出文本树——时，如果把这些操作
 *   都做成 Element 的成员方法，会出现两个痛点：
 *
 *     1. Element 类越来越胖，承担了与自身核心职责无关的操作
 *     2. 每新增一个操作，都要改每一个 Element 子类（File、Directory 都加方法）
 *
 *   访问者模式把"操作"从 Element 中抽离，封装到独立的 Visitor 类中：
 *   新增操作 = 新增一个 Visitor 类，Element 的类结构完全不用动。
 *
 * 【核心思想：双重分派 (Double Dispatch)】
 *
 *   C++ 的虚函数是"单分派"：调用 obj.foo(arg) 时，只根据 obj 的实际类型
 *   绑定到对应虚函数，arg 的"静态类型"决定调用哪个重载版本。
 *   也就是说，仅靠一次虚函数调用，无法同时按"元素类型 + 操作类型"分派。
 *
 *   访问者用两次虚函数调用实现"按两个对象的实际类型"分派：
 *
 *     element->accept(visitor)        // 第 1 次分派：按 element 的实际类型
 *                                     //   进入 File::accept 或 Directory::accept
 *           visitor.visit(*this)     // 第 2 次分派：*this 的"静态类型"是
 *                                     //   const File& / const Directory&，由此
 *                                     //   选中 visit(File) 或 visit(Directory)
 *                                     //   的重载，再按 visitor 实际类型绑定
 *
 *   结果：一个 visitor 对象 + 一个 element 对象，能精确命中"对这种元素
 *   执行这种操作"的方法，无需在 Element 里写任何 if/else 或 RTTI。
 *
 * 【权衡：新增操作易，新增元素类型难】  ← 访问者模式最关键的特征
 *
 *   ✅ 新增操作（容易）：写一个新 Visitor 子类即可，File / Directory /
 *      Visitor 基类全部不动。本例的三个 Visitor 就是这么加上去的。
 *
 *   ❌ 新增元素类型（困难）：想加一个 Symlink 元素，必须
 *        - 在 Visitor 基类加 `virtual void visit(const Symlink&) = 0;`
 *        - 在每一个已有 Visitor（Size / Search / Export）里补 visit(Symlink)
 *      已有 visitor 越多，改动越大。
 *
 *   结论：访问者模式适用于"元素类型稳定、操作频繁变化"的场景。
 *
 * 【与迭代器、组合模式的区别】
 *
 *   迭代器 (Iterator)：把"遍历"逻辑抽离 —— 只管遍历，不管对元素做什么
 *   访问者 (Visitor)：把"操作"逻辑抽离 —— 对每个元素做什么由 visitor 决定
 *   组合 (Composite)：让叶子和容器共享接口 —— 访问者常作用在组合树上
 *
 *   典型配合：用组合构建树 → 用访问者对每个节点执行操作（本例即如此）；
 *   访问者也可配合迭代器，先遍历再操作。
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>

// ============================================================================
// 前向声明
//   Element 的 accept 需要引用 Visitor，Visitor 的 visit 需要引用
//   File / Directory，二者互相引用，故先前向声明再定义。
// ============================================================================
class Visitor;
class File;
class Directory;

// ============================================================================
// Element：元素抽象基类
//
// 只暴露最小公共接口：accept(Visitor) 与 getName()。
// 注意：这里刻意不写 getSize() / getChildren() 等操作——
// "算总大小 / 搜索 / 导出"等操作全部交给 Visitor 从外部添加，
// 这正是访问者模式把操作外置的体现。
// ============================================================================
class Element {
public:
    virtual ~Element() = default;
    virtual void accept(Visitor& visitor) const = 0;
    virtual std::string getName() const = 0;
};

// ============================================================================
// Visitor：访问者抽象基类
//
// 为每一种具体 Element 重载一个 visit 方法。
//   ⚠ 新增元素类型时，必须在这里加方法，并在每个具体 Visitor 里实现
//     —— 这就是"新增元素类型困难"的代价来源。
// ============================================================================
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void visit(const File& file) = 0;
    virtual void visit(const Directory& dir) = 0;
};

// ============================================================================
// File：叶子元素
//
// accept 的实现只有一行：把 *this（静态类型为 const File&）交给 visitor。
//   关键点：visit 是重载函数，编译器依据 *this 的"静态类型 const File&"
//   选中 visit(const File&)，再由虚函数按 visitor 的实际类型绑定——
//   这就是第二次分派。
// ============================================================================
class File : public Element {
public:
    File(std::string name, size_t size)
        : name_(std::move(name)), size_(size) {}

    std::string getName() const override { return name_; }
    size_t getSize() const { return size_; }   // File 自身数据，供 visitor 读取

    void accept(Visitor& visitor) const override {
        visitor.visit(*this);
    }

private:
    std::string name_;
    size_t size_;
};

// ============================================================================
// Directory：容器元素（与组合模式结合，可挂子节点）
//
// 刻意不实现"算总大小 / 搜索 / 导出"等方法——这些操作全部由
// Visitor 从外部添加，Directory 类不参与，体现"操作外置"。
// 仅暴露 getChildren() 让 visitor 自行驱动遍历。
// ============================================================================
class Directory : public Element {
public:
    explicit Directory(std::string name) : name_(std::move(name)) {}

    std::string getName() const override { return name_; }

    void addChild(std::shared_ptr<Element> child) {
        children_.push_back(std::move(child));
    }

    const std::vector<std::shared_ptr<Element>>& getChildren() const {
        return children_;
    }

    void accept(Visitor& visitor) const override {
        visitor.visit(*this);
    }

private:
    std::string name_;
    std::vector<std::shared_ptr<Element>> children_;
};

// ============================================================================
// 具体访问者 1：SizeCalculatorVisitor —— 计算整棵树的总大小
//
//   visit(File)      累加文件自身大小
//   visit(Directory) 递归遍历子节点（visitor 自己驱动遍历）
//
// "算目录总大小"这个操作完全定义在 visitor 里，Directory 类无需知道。
// 若再来一个"找最大文件"的操作，再写一个 Visitor 子类即可，元素不动。
// ============================================================================
class SizeCalculatorVisitor : public Visitor {
public:
    void visit(const File& file) override {
        total_ += file.getSize();
        ++fileCount_;
    }

    void visit(const Directory& dir) override {
        ++dirCount_;
        // 访问者自己驱动遍历：对每个子节点回调 accept，再次进入双重分派
        for (const auto& child : dir.getChildren()) {
            child->accept(*this);
        }
    }

    size_t totalSize() const { return total_; }
    size_t fileCount() const { return fileCount_; }
    size_t dirCount() const { return dirCount_; }

private:
    size_t total_ = 0;
    size_t fileCount_ = 0;
    size_t dirCount_ = 0;
};

// ============================================================================
// 具体访问者 2：SearchVisitor —— 按文件名子串搜索
//
//   新增"搜索"操作无需改 File / Directory，只新增这一个 visitor 类。
//   结果收集在 visitor 内部，遍历结束后一次性取出。
// ============================================================================
class SearchVisitor : public Visitor {
public:
    explicit SearchVisitor(std::string keyword)
        : keyword_(std::move(keyword)) {}

    void visit(const File& file) override {
        if (file.getName().find(keyword_) != std::string::npos) {
            results_.push_back(file.getName());
        }
    }

    void visit(const Directory& dir) override {
        for (const auto& child : dir.getChildren()) {
            child->accept(*this);
        }
    }

    const std::vector<std::string>& results() const { return results_; }
    const std::string& keyword() const { return keyword_; }

private:
    std::string keyword_;
    std::vector<std::string> results_;
};

// ============================================================================
// 具体访问者 3：ExportVisitor —— 导出文件树为带连接线的文本
//
//   用栈 lastFlags_ 记录每一层"是否为最后一个子节点"：
//     - 当前层（栈顶）决定本节点的连接符：└── (末尾) / ├── (非末尾)
//     - 上层决定本层的缩进延续：    "    " (末尾) / "│   " (非末尾)
//   根节点栈为空，不打印连接符，整棵树自然成形。
// ============================================================================
class ExportVisitor : public Visitor {
public:
    void visit(const File& file) override {
        printIndent();
        out_ << file.getName() << " (" << file.getSize() << " KB)\n";
    }

    void visit(const Directory& dir) override {
        printIndent();
        out_ << dir.getName() << "/\n";

        const auto& children = dir.getChildren();
        for (size_t i = 0; i < children.size(); ++i) {
            // 入栈：标记这一层是否为最后一个子节点
            lastFlags_.push_back(i + 1 == children.size());
            children[i]->accept(*this);   // 递归 → 再次双重分派
            lastFlags_.pop_back();         // 出栈：恢复上层状态
        }
    }

    std::string result() const { return out_.str(); }

private:
    void printIndent() {
        for (size_t d = 0; d < lastFlags_.size(); ++d) {
            bool last = lastFlags_[d];
            if (d + 1 == lastFlags_.size()) {
                // 当前层：本节点自己的连接符
                out_ << (last ? "`-- " : "|-- ");
            } else {
                // 上层：本层的缩进延续
                out_ << (last ? "    " : "|   ");
            }
        }
    }

    std::ostringstream out_;
    std::vector<bool> lastFlags_;   // 栈：每层"是否为最后一个子节点"
};

// ============================================================================
// 辅助：构建示例文件系统树
// ============================================================================
static std::shared_ptr<Directory> buildSampleTree() {
    auto root = std::make_shared<Directory>("project");

    auto srcDir = std::make_shared<Directory>("src");
    srcDir->addChild(std::make_shared<File>("main.cpp", 12));
    srcDir->addChild(std::make_shared<File>("utils.cpp", 8));
    srcDir->addChild(std::make_shared<File>("utils.h", 3));

    auto docDir = std::make_shared<Directory>("docs");
    docDir->addChild(std::make_shared<File>("README.md", 5));
    docDir->addChild(std::make_shared<File>("design.md", 15));

    auto assetsDir = std::make_shared<Directory>("assets");
    auto imgDir = std::make_shared<Directory>("images");
    imgDir->addChild(std::make_shared<File>("logo.png", 200));
    imgDir->addChild(std::make_shared<File>("banner.jpg", 350));
    assetsDir->addChild(imgDir);
    assetsDir->addChild(std::make_shared<File>("style.css", 7));

    root->addChild(srcDir);
    root->addChild(docDir);
    root->addChild(assetsDir);
    root->addChild(std::make_shared<File>("Makefile", 2));

    return root;
}

// ============================================================================
// Main：演示三种访问者操作 + 权衡说明
// ============================================================================
int main() {
    std::cout << "\n";
    std::cout << "####################################################\n";
    std::cout << "#     Visitor Pattern - C++ Demonstration          #\n";
    std::cout << "####################################################\n";

    auto root = buildSampleTree();

    // ---- 0. 先看一眼要操作的对象结构 ----
    std::cout << "\n[0] Object structure (File + Directory tree)\n";
    std::cout << "----------------------------------------\n";
    std::cout << "project/\n";
    std::cout << "  src/         {main.cpp, utils.cpp, utils.h}\n";
    std::cout << "  docs/        {README.md, design.md}\n";
    std::cout << "  assets/      {images/{logo.png, banner.jpg}, style.css}\n";
    std::cout << "  Makefile\n";

    // ---- 1. 访问者 #1：计算总大小 ----
    std::cout << "\n[1] Visitor #1: SizeCalculatorVisitor\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Operation: compute total size of the tree.\n";
    std::cout << "          (NOT a method on Directory — defined here in visitor)\n\n";

    SizeCalculatorVisitor sizeVisitor;
    root->accept(sizeVisitor);   // 双重分派入口
    std::cout << "  total size : " << sizeVisitor.totalSize() << " KB\n";
    std::cout << "  file count : " << sizeVisitor.fileCount() << "\n";
    std::cout << "  dir  count : " << sizeVisitor.dirCount() << "\n";

    // ---- 2. 访问者 #2：按名搜索 ----
    std::cout << "\n[2] Visitor #2: SearchVisitor (keyword = \"utils\")\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Operation: find files whose name contains \"utils\".\n";
    std::cout << "          (Again, NOT a method on File/Directory)\n\n";

    SearchVisitor searchVisitor("utils");
    root->accept(searchVisitor);
    if (searchVisitor.results().empty()) {
        std::cout << "  (no match)\n";
    } else {
        for (const auto& name : searchVisitor.results()) {
            std::cout << "  found: " << name << "\n";
        }
    }

    // ---- 3. 访问者 #3：导出文本树 ----
    std::cout << "\n[3] Visitor #3: ExportVisitor\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Operation: export the tree as indented text.\n\n";

    ExportVisitor exportVisitor;
    root->accept(exportVisitor);
    std::cout << exportVisitor.result();

    // ---- 4. 关键权衡：新增操作 vs 新增元素类型 ----
    std::cout << "\n[4] Tradeoff: adding operations vs adding element types\n";
    std::cout << "========================================================\n";

    std::cout << "\n  Adding a new OPERATION  (easy  -- the win):\n";
    std::cout << "    e.g. want \"find largest file\"?\n";
    std::cout << "    -> just write a new LargestFileVisitor class.\n";
    std::cout << "       File / Directory / Visitor base class: ALL UNTOUCHED.\n";

    std::cout << "\n  Adding a new ELEMENT TYPE (hard -- the cost):\n";
    std::cout << "    e.g. want a Symlink element? You MUST:\n";
    std::cout << "       1. add `virtual void visit(const Symlink&) = 0;` to Visitor\n";
    std::cout << "       2. implement visit(Symlink) in SizeCalculatorVisitor\n";
    std::cout << "       3. implement visit(Symlink) in SearchVisitor\n";
    std::cout << "       4. implement visit(Symlink) in ExportVisitor\n";
    std::cout << "    -> every existing visitor must change.\n";

    std::cout << "\n  => Use Visitor when: element types are stable,\n";
    std::cout << "                      operations vary often.\n";

    std::cout << "\n####################################################\n";
    return 0;
}
