/**
 * ============================================================================
 * 命令模式 (Command Pattern) - 行为型设计模式
 * ============================================================================
 *
 * 【解决的问题】
 *
 * 当一个"请求发起者"（如遥控器、按钮、菜单项）需要触发各种"接收者"
 * （如灯、空调、音响）的动作时，如果让发起者直接调用接收者的方法，
 * 会出现紧耦合：
 *
 *   - 发起者要持有每个接收者的引用，并为每个动作写分支
 *   - 新增一种设备，发起者要加字段、加分支
 *   - 想增加"撤销 / 宏 / 排队 / 日志"等横切能力时，发起者要为
 *     每种接收者各写一遍逻辑
 *   - 发起者与接收者绑定死，无法在运行时把动作"装"到按钮上
 *
 * 命令模式把"请求"（一次方法调用）封装成一个对象。发起者只依赖
 * Command 抽象接口，不再认识任何具体接收者。
 *
 * 【核心思想】
 *
 *   - Command（命令）：声明 execute() / undo() 接口
 *   - ConcreteCommand（具体命令）：绑定一个接收者，实现 execute/undo
 *   - Receiver（接收者）：真正干活的业务对象（灯、空调……）
 *   - Invoker（调用者）：持有命令，在合适时机调用 execute()
 *
 *   请求从"方法调用"变成"对象传递"，于是可以：
 *     参数化（按钮装哪条命令）、排队（命令队列）、记录日志、
 *     撤销（命令记下执行前状态）、组合（宏命令一次执行多条）
 *
 * 【典型场景】
 *   - 遥控器 / 菜单项 / 快捷键绑定动作
 *   - 事务系统（命令可撤销）
 *   - 任务队列 / 线程池（命令对象可排队）
 *   - GUI 撤销重做
 *
 * 【与中介者、组合、备忘录模式的区别/联系】
 *
 *   命令模式 (Command):
 *     - 行为型，把"请求"封装成对象，发起者与接收者解耦
 *     - 关注：请求的参数化、排队、撤销、组合
 *
 *   中介者模式 (Mediator):
 *     - 行为型，把"多对象网状通信"收拢成星形
 *     - 关注：对象之间的通信解耦，而非把单次请求对象化
 *
 *   组合模式 (Composite):
 *     - 结构型，树形结构统一操作
 *     - 联系：宏命令 (MacroCommand) 用组合模式把多条命令组合成一棵树，
 *       对外仍是一个 Command，递归 execute/undo
 *
 *   备忘录模式 (Memento):
 *     - 行为型，保存并恢复对象内部状态
 *     - 联系：命令的 undo 可以用"命令自行记录执行前状态"实现（本例做法），
 *       也可以用备忘录保存接收者快照实现；两者解决的是同一类问题
 *
 * ============================================================================
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ============================================================================
// Receiver: 灯
// ============================================================================

class Light {
public:
    explicit Light(std::string location) : location_(std::move(location)) {}

    void on() {
        on_ = true;
        std::cout << "      [" << location_ << " Light] ON\n";
    }
    void off() {
        on_ = false;
        std::cout << "      [" << location_ << " Light] OFF\n";
    }
    bool isOn() const { return on_; }

private:
    std::string location_;
    bool on_ = false;
};

// ============================================================================
// Receiver: 空调（带温度，便于演示"带参数命令"的撤销）
// ============================================================================

class AirConditioner {
public:
    explicit AirConditioner(std::string location) : location_(std::move(location)) {}

    void on() {
        on_ = true;
        std::cout << "      [" << location_ << " AC] ON (temp=" << temperature_ << "C)\n";
    }
    void off() {
        on_ = false;
        std::cout << "      [" << location_ << " AC] OFF\n";
    }
    void setTemperature(int temperature) {
        temperature_ = temperature;
        if (on_) {
            std::cout << "      [" << location_ << " AC] temp -> " << temperature_ << "C\n";
        }
    }
    bool isOn() const { return on_; }
    int temperature() const { return temperature_; }

private:
    std::string location_;
    bool on_ = false;
    int temperature_ = 26;
};

// ============================================================================
// Receiver: 音响（带音量）
// ============================================================================

class Stereo {
public:
    explicit Stereo(std::string location) : location_(std::move(location)) {}

    void on() {
        on_ = true;
        std::cout << "      [" << location_ << " Stereo] ON (vol=" << volume_ << ")\n";
    }
    void off() {
        on_ = false;
        std::cout << "      [" << location_ << " Stereo] OFF\n";
    }
    void setVolume(int volume) {
        volume_ = volume;
        if (on_) {
            std::cout << "      [" << location_ << " Stereo] vol -> " << volume_ << "\n";
        }
    }
    bool isOn() const { return on_; }
    int volume() const { return volume_; }

private:
    std::string location_;
    bool on_ = false;
    int volume_ = 10;
};

// ============================================================================
// Command: 抽象命令接口
//
// execute() 执行请求；undo() 撤销上一次 execute() 的效果。
// 每条命令自行记录"执行前的接收者状态"，撤销时据此还原。
// ============================================================================

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

// ============================================================================
// NoCommand: 空对象（Null Object）
//
// 占据未绑定的按钮槽位，让 Invoker 不必判空，调用始终安全。
// ============================================================================

class NoCommand : public Command {
public:
    void execute() override { std::cout << "      [NoCommand] (slot empty)\n"; }
    void undo() override { std::cout << "      [NoCommand] (nothing to undo)\n"; }
};

// ============================================================================
// ConcreteCommand: 灯的开关命令
//
// execute 前记录 wasOn_，undo 时恢复到原来的开/关状态。
// ============================================================================

class LightOnCommand : public Command {
public:
    explicit LightOnCommand(Light* light) : light_(light) {}

    void execute() override {
        wasOn_ = light_->isOn();
        light_->on();
    }
    void undo() override {
        if (wasOn_) light_->on();
        else        light_->off();
    }

private:
    Light* light_;
    bool wasOn_ = false;
};

class LightOffCommand : public Command {
public:
    explicit LightOffCommand(Light* light) : light_(light) {}

    void execute() override {
        wasOn_ = light_->isOn();
        light_->off();
    }
    void undo() override {
        if (wasOn_) light_->on();
        else        light_->off();
    }

private:
    Light* light_;
    bool wasOn_ = false;
};

// ============================================================================
// ConcreteCommand: 空调开关命令
// ============================================================================

class AirConditionerOnCommand : public Command {
public:
    explicit AirConditionerOnCommand(AirConditioner* ac) : ac_(ac) {}

    void execute() override {
        wasOn_ = ac_->isOn();
        prevTemp_ = ac_->temperature();
        ac_->on();
    }
    void undo() override {
        ac_->setTemperature(prevTemp_);
        if (wasOn_) ac_->on();
        else        ac_->off();
    }

private:
    AirConditioner* ac_;
    bool wasOn_ = false;
    int prevTemp_ = 26;
};

class AirConditionerOffCommand : public Command {
public:
    explicit AirConditionerOffCommand(AirConditioner* ac) : ac_(ac) {}

    void execute() override {
        wasOn_ = ac_->isOn();
        prevTemp_ = ac_->temperature();
        ac_->off();
    }
    void undo() override {
        ac_->setTemperature(prevTemp_);
        if (wasOn_) ac_->on();
        else        ac_->off();
    }

private:
    AirConditioner* ac_;
    bool wasOn_ = false;
    int prevTemp_ = 26;
};

// ============================================================================
// ConcreteCommand: 设定空调温度（带参数命令）
//
// 演示"参数化请求"：同一条命令，不同温度参数产生不同效果，
// undo 恢复到设定前的温度。
// ============================================================================

class SetTemperatureCommand : public Command {
public:
    SetTemperatureCommand(AirConditioner* ac, int targetTemp)
        : ac_(ac), targetTemp_(targetTemp) {}

    void execute() override {
        prevTemp_ = ac_->temperature();
        wasOn_ = ac_->isOn();
        if (!wasOn_) ac_->on();
        ac_->setTemperature(targetTemp_);
    }
    void undo() override {
        ac_->setTemperature(prevTemp_);
        if (!wasOn_) ac_->off();
    }

private:
    AirConditioner* ac_;
    int targetTemp_;
    int prevTemp_ = 26;
    bool wasOn_ = false;
};

// ============================================================================
// ConcreteCommand: 音响开关命令
// ============================================================================

class StereoOnCommand : public Command {
public:
    explicit StereoOnCommand(Stereo* stereo) : stereo_(stereo) {}

    void execute() override {
        wasOn_ = stereo_->isOn();
        stereo_->on();
    }
    void undo() override {
        if (wasOn_) stereo_->on();
        else        stereo_->off();
    }

private:
    Stereo* stereo_;
    bool wasOn_ = false;
};

class StereoOffCommand : public Command {
public:
    explicit StereoOffCommand(Stereo* stereo) : stereo_(stereo) {}

    void execute() override {
        wasOn_ = stereo_->isOn();
        stereo_->off();
    }
    void undo() override {
        if (wasOn_) stereo_->on();
        else        stereo_->off();
    }

private:
    Stereo* stereo_;
    bool wasOn_ = false;
};

// ============================================================================
// MacroCommand: 宏命令（组合模式的应用）
//
// 把多条命令组合成一个 Command。execute 顺序执行子命令；
// undo 按相反顺序撤销，保证状态正确回退。
//
// 这里用 shared_ptr 共享子命令：同一条命令既可单独装到按钮上，
// 也可作为宏的一部分，互不影响。
// ============================================================================

class MacroCommand : public Command {
public:
    explicit MacroCommand(std::string name) : name_(std::move(name)) {}

    void addCommand(std::shared_ptr<Command> cmd) {
        commands_.push_back(std::move(cmd));
    }

    void execute() override {
        std::cout << "      >> Macro [" << name_ << "] execute begin\n";
        for (auto& cmd : commands_) {
            cmd->execute();
        }
        std::cout << "      >> Macro [" << name_ << "] execute end\n";
    }

    void undo() override {
        std::cout << "      >> Macro [" << name_ << "] undo begin (reverse order)\n";
        // 撤销必须逆序：最后执行的先撤，才能正确还原中间状态
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
            (*it)->undo();
        }
        std::cout << "      >> Macro [" << name_ << "] undo end\n";
    }

private:
    std::string name_;
    std::vector<std::shared_ptr<Command>> commands_;
};

// ============================================================================
// Invoker: 遥控器
//
// 每个槽位绑定一对命令（开/关）。按下按钮时执行对应命令，
// 并把"执行过的命令"压入历史栈，供 pressUndo() 连续撤销多步。
//
// 遥控器只认识 Command 接口，不认识任何具体接收者——
// 这正是命令模式带来的解耦：新设备、新动作、宏命令都能装进槽位，
// 遥控器代码一行都不用改。
// ============================================================================

class RemoteControl {
public:
    static constexpr int kSlotCount = 3;

    RemoteControl() {
        // 空槽位用 NoCommand 占位，调用始终安全
        auto nop = std::make_shared<NoCommand>();
        for (int i = 0; i < kSlotCount; ++i) {
            onCommands_[i] = nop;
            offCommands_[i] = nop;
        }
    }

    void setCommand(int slot, std::shared_ptr<Command> onCmd, std::shared_ptr<Command> offCmd) {
        if (slot < 0 || slot >= kSlotCount) return;
        onCommands_[slot] = std::move(onCmd);
        offCommands_[slot] = std::move(offCmd);
    }

    void pressOn(int slot) {
        if (slot < 0 || slot >= kSlotCount) return;
        std::cout << "  [Remote] press ON  (slot " << slot << ")\n";
        onCommands_[slot]->execute();
        history_.push_back(onCommands_[slot]);
    }

    void pressOff(int slot) {
        if (slot < 0 || slot >= kSlotCount) return;
        std::cout << "  [Remote] press OFF (slot " << slot << ")\n";
        offCommands_[slot]->execute();
        history_.push_back(offCommands_[slot]);
    }

    void pressUndo() {
        std::cout << "  [Remote] press UNDO\n";
        if (history_.empty()) {
            std::cout << "      [NoCommand] (nothing to undo)\n";
            return;
        }
        // 弹出最近执行的一条命令并撤销，支持连续撤销多步
        history_.back()->undo();
        history_.pop_back();
    }

private:
    std::shared_ptr<Command> onCommands_[kSlotCount];
    std::shared_ptr<Command> offCommands_[kSlotCount];
    // 执行历史栈：每次 execute 压栈，每次 undo 弹栈
    std::vector<std::shared_ptr<Command>> history_;
};

// ============================================================================
// Main: 演示命令模式各项能力
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "####################################################\n";
    std::cout << "#     Command Pattern - C++ Demonstration          #\n";
    std::cout << "#     (Smart Home Remote Control)                  #\n";
    std::cout << "####################################################\n\n";

    // ---- 接收者（设备） ----
    Light livingLight("LivingRoom");
    AirConditioner livingAC("LivingRoom");
    Stereo livingStereo("LivingRoom");

    // ---- 具体命令 ----
    auto lightOn  = std::make_shared<LightOnCommand>(&livingLight);
    auto lightOff = std::make_shared<LightOffCommand>(&livingLight);
    auto acOn     = std::make_shared<AirConditionerOnCommand>(&livingAC);
    auto acOff    = std::make_shared<AirConditionerOffCommand>(&livingAC);
    auto stereoOn = std::make_shared<StereoOnCommand>(&livingStereo);
    auto stereoOff= std::make_shared<StereoOffCommand>(&livingStereo);

    // ---- 遥控器：把命令"装"到槽位 ----
    RemoteControl remote;
    remote.setCommand(0, lightOn,  lightOff);
    remote.setCommand(1, acOn,     acOff);
    remote.setCommand(2, stereoOn, stereoOff);

    // --------------------------------------------------------
    std::cout << "=== 1. 按钮触发设备开关 ===\n";
    remote.pressOn(0);   // 开灯
    remote.pressOn(1);   // 开空调
    remote.pressOn(2);   // 开音响
    std::cout << "\n";

    // --------------------------------------------------------
    std::cout << "=== 2. 撤销最后一次操作（关掉音响） ===\n";
    remote.pressUndo();  // 撤销"开音响" -> 音响回到关闭
    std::cout << "\n";

    // --------------------------------------------------------
    std::cout << "=== 3. 撤销链：连续撤销前两步 ===\n";
    std::cout << "-- 当前状态后，再撤销一次（撤销开空调） --\n";
    remote.pressUndo();
    std::cout << "-- 再撤销一次（撤销开灯） --\n";
    remote.pressUndo();
    std::cout << "\n";

    // --------------------------------------------------------
    std::cout << "=== 4. 带参数的命令：设定空调温度 ===\n";
    SetTemperatureCommand setToCool(&livingAC, 20);
    std::cout << "-- 先开空调 --\n";
    remote.pressOn(1);
    std::cout << "-- 直接执行 SetTemperatureCommand(20C) --\n";
    setToCool.execute();
    std::cout << "-- 撤销温度设定（应回到 26C） --\n";
    setToCool.undo();
    std::cout << "-- 关闭空调，重置到干净状态，便于后续宏命令演示 --\n";
    remote.pressOff(1);
    std::cout << "\n";

    // --------------------------------------------------------
    std::cout << "=== 5. 宏命令：一键「回家模式」 ===\n";
    // 宏命令组合：开灯 + 开空调 + 开音响
    // 用 shared_ptr 共享已创建的命令对象，既可单独用也可进宏
    auto partyMacro = std::make_shared<MacroCommand>("PartyMode");
    partyMacro->addCommand(lightOn);
    partyMacro->addCommand(acOn);
    partyMacro->addCommand(stereoOn);

    std::cout << "-- 执行宏命令（一键开启多个设备） --\n";
    partyMacro->execute();
    std::cout << "\n";

    std::cout << "-- 撤销宏命令（逆序撤销：音响 -> 空调 -> 灯，各自恢复执行前状态） --\n";
    partyMacro->undo();
    std::cout << "\n";

    // --------------------------------------------------------
    std::cout << "=== 6. 宏命令也能装进遥控器槽位 ===\n";
    // 把宏命令装到槽位 0（覆盖灯），演示命令的"参数化"
    remote.setCommand(0, partyMacro, std::make_shared<NoCommand>());
    remote.pressOn(0);   // 触发整个宏
    remote.pressUndo();  // 撤销整个宏
    std::cout << "\n";

    // --------------------------------------------------------
    std::cout << "=== 7. 空槽位调用安全（NoCommand 占位） ===\n";
    RemoteControl emptyRemote;
    emptyRemote.pressOn(1);   // 槽位 1 未绑定 -> NoCommand
    emptyRemote.pressUndo();  // 没有历史 -> NoCommand
    std::cout << "\n";

    std::cout << "####################################################\n";
    std::cout << "# Key point: Remote only knows the Command          #\n";
    std::cout << "# interface. New devices / macro commands / undo   #\n";
    std::cout << "# can be added without touching RemoteControl.      #\n";
    std::cout << "# Requests become objects -> can be parameterized,  #\n";
    std::cout << "# queued, logged, undone, composed.                 #\n";
    std::cout << "####################################################\n";

    return 0;
}
