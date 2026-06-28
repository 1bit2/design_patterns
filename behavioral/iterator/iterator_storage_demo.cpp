/**
 * ============================================================================
 * 迭代器模式 (Iterator Pattern) - STL 风格
 * ============================================================================
 *
 * 【STL 风格 vs GoF 经典风格】
 *
 *   GoF 经典风格（Java 风格）：
 *     Iterator* it = menu.createIterator();
 *     for (it->first(); !it->isDone(); it->next()) {
 *         it->currentItem();
 *     }
 *     → 基于虚函数的多态，运行时绑定
 *
 *   STL 风格（C++ 风格）：
 *     for (auto it = menu.begin(); it != menu.end(); ++it) {
 *         *it;
 *     }
 *     → 基于模板的鸭子类型，编译期绑定
 *     → 通过 operator++, operator*, operator!= 统一接口
 *
 * 【核心不变】
 *   不管哪种风格，本质都是：把遍历逻辑委托给迭代器类
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <memory>

// ============================================================================
// 元素类型
// ============================================================================

class MenuItem {
public:
    MenuItem(std::string name, double price)
        : name_(std::move(name)), price_(price) {}

    const std::string& getName() const { return name_; }
    double getPrice() const { return price_; }
    void print() const {
        std::cout << "  - " << name_ << " (¥" << price_ << ")\n";
    }

private:
    std::string name_;
    double price_;
};

// ============================================================================
// MenuIterator: 通用迭代器模板
//
// 包装任意底层迭代器（vector::iterator、list::iterator 等），
// 对外暴露统一的 STL 迭代器接口：++, *, !=
//
// 遍历逻辑（如何前进、如何取值）由底层迭代器决定，
// MenuIterator 只做一层统一封装。
// ============================================================================

template <typename InnerIterator>
class MenuIterator {
public:
    explicit MenuIterator(InnerIterator it) : it_(std::move(it)) {}

    // 前缀 ++：前进到下一个元素
    MenuIterator& operator++() {
        ++it_;
        return *this;
    }

    // 解引用：获取当前元素
    MenuItem& operator*() { return *it_; }
    const MenuItem& operator*() const { return *it_; }

    // 箭头：访问元素成员
    auto operator->() { return &(*it_); }

    // 比较：判断是否到达末尾
    bool operator!=(const MenuIterator& other) const {
        return it_ != other.it_;
    }

    bool operator==(const MenuIterator& other) const {
        return it_ == other.it_;
    }

private:
    InnerIterator it_;
};

// ============================================================================
// 方案A：基于 vector 存储
// ============================================================================

class VectorMenu {
public:
    using Iterator = MenuIterator<std::vector<MenuItem>::iterator>;
    using ConstIterator = MenuIterator<std::vector<MenuItem>::const_iterator>;

    void addItem(const MenuItem& item) { items_.push_back(item); }

    Iterator begin() { return Iterator(items_.begin()); }
    Iterator end() { return Iterator(items_.end()); }

    ConstIterator begin() const { return ConstIterator(items_.begin()); }
    ConstIterator end() const { return ConstIterator(items_.end()); }

private:
    std::vector<MenuItem> items_;
};

// ============================================================================
// 方案B：基于 list 存储
// ============================================================================

class ListMenu {
public:
    using Iterator = MenuIterator<std::list<MenuItem>::iterator>;
    using ConstIterator = MenuIterator<std::list<MenuItem>::const_iterator>;

    void addItem(const MenuItem& item) { items_.push_back(item); }

    Iterator begin() { return Iterator(items_.begin()); }
    Iterator end() { return Iterator(items_.end()); }

    ConstIterator begin() const { return ConstIterator(items_.begin()); }
    ConstIterator end() const { return ConstIterator(items_.end()); }

private:
    std::list<MenuItem> items_;
};

// ============================================================================
// 客户端代码：模板函数，支持任何提供 begin()/end() 的聚合
// ============================================================================

// 打印菜单
template <typename Menu>
void printMenu(const std::string& title, const Menu& menu) {
    std::cout << "\n" << title << "\n";
    std::cout << "========================================\n";

    for (auto it = menu.begin(); it != menu.end(); ++it) {
        it->print();
    }
}

// 计算总价
template <typename Menu>
double calculateTotal(const Menu& menu) {
    double total = 0.0;
    for (auto it = menu.begin(); it != menu.end(); ++it) {
        total += it->getPrice();
    }
    return total;
}

// 查找最贵的菜品
template <typename Menu>
const MenuItem* findMostExpensive(const Menu& menu) {
    const MenuItem* mostExpensive = nullptr;
    for (auto it = menu.begin(); it != menu.end(); ++it) {
        if (mostExpensive == nullptr || it->getPrice() > mostExpensive->getPrice()) {
            mostExpensive = &(*it);
        }
    }
    return mostExpensive;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "####################################################\n";
    std::cout << "#  Iterator Pattern - STL Style                    #\n";
    std::cout << "####################################################\n";

    // ---- 方案A：vector 存储 ----
    std::cout << "\n>>> Using vector-based menu <<<\n";

    VectorMenu vectorMenu;
    vectorMenu.addItem(MenuItem("Pancakes", 25.0));
    vectorMenu.addItem(MenuItem("Omelette", 35.0));
    vectorMenu.addItem(MenuItem("Coffee", 15.0));

    printMenu("Vector Menu", vectorMenu);
    std::cout << "Total: ¥" << calculateTotal(vectorMenu) << "\n";

    // ---- 方案B：list 存储 ----
    std::cout << "\n>>> Using list-based menu <<<\n";

    ListMenu listMenu;
    listMenu.addItem(MenuItem("Burger", 45.0));
    listMenu.addItem(MenuItem("Salad", 32.0));
    listMenu.addItem(MenuItem("Pasta", 38.0));

    printMenu("List Menu", listMenu);
    std::cout << "Total: ¥" << calculateTotal(listMenu) << "\n";

    // ---- 嵌套遍历演示 ----
    std::cout << "\n========================================\n";
    std::cout << "Nested Iteration (each iterator has independent state)\n";
    std::cout << "========================================\n\n";

    for (auto outer = vectorMenu.begin(); outer != vectorMenu.end(); ++outer) {
        for (auto inner = listMenu.begin(); inner != listMenu.end(); ++inner) {
            std::cout << "  " << outer->getName()
                      << " + " << inner->getName() << "\n";
        }
    }

    // ---- 对比总结 ----
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "Comparison Summary\n";
    std::cout << "========================================\n";
    std::cout << "\n";
    std::cout << "STL style vs GoF style:\n";
    std::cout << "\n";
    std::cout << "  GoF:    it->first() / !it->isDone() / it->next() / it->currentItem()\n";
    std::cout << "  STL:    menu.begin() / it != menu.end() / ++it / *it\n";
    std::cout << "\n";
    std::cout << "Both achieve the SAME goal:\n";
    std::cout << "  Delegate traversal logic to iterator class,\n";
    std::cout << "  keep aggregate focused on data storage.\n";

    return 0;
}
