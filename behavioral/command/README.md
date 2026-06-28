# Command Pattern / 命令模式

**分类**: 行为型模式 (Behavioral)

## 一句话总结

把"请求"（一次方法调用）封装成对象，让发起者只依赖命令接口，从而支持参数化、排队、记录日志、撤销和组合。

## 解决的问题

### 问题1：没有命令模式 — 发起者直接耦合接收者，难以扩展

```cpp
// 没有命令模式：遥控器直接调用各设备的方法
class RemoteControl {
    Light* light_;
    AirConditioner* ac_;
public:
    void pressOn(int slot) {
        if (slot == 0)      light_->on();   // 直接耦合 Light::on
        else if (slot == 1) ac_->on();       // 直接耦合 AirConditioner::on
    }
};
```

**痛点**：
- 遥控器要持有每个接收者的引用，并为每个动作写分支
- 新增一种设备（如窗帘 Curtain），遥控器要加字段、加分支
- 遥控器与接收者绑定死，**无法在运行时把动作"装"到按钮上**

### 有命令模式 — 发起者只依赖 Command 接口

```cpp
// 有命令模式：遥控器只认识 Command 抽象接口
class RemoteControl {
    std::vector<Command*> onCommands_;
public:
    void setCommand(int slot, Command* onCmd) { onCommands_[slot] = onCmd; }
    void pressOn(int slot) { onCommands_[slot]->execute(); }  // 不认识任何接收者
};

// 新增设备：只需写一个新的 Command 子类，遥控器代码一行都不用改
class CurtainOpenCommand : public Command { /* ... */ };
remote.setCommand(3, new CurtainOpenCommand(&curtain));   // 运行时装配
```

**关键区别**：

```
没有命令模式：新增设备 → 遥控器要加字段、加分支（发起者与接收者紧耦合）
有命令模式：  新增设备 → 只写新 Command 子类，遥控器不变（通过命令接口隔离）
```

### 问题2：没有命令模式 — 撤销/宏/排队等横切能力无处安放

```cpp
// 想加"撤销"：遥控器要为每种接收者各写一段还原逻辑
class RemoteControl {
    void undo() {
        // 灯怎么还原？空调怎么还原？音响怎么还原？每种都要 if-else
        if (lastAction == LIGHT_ON) light_->off();
        else if (lastAction == AC_ON) ac_->off();
        // ... 每加一种设备，这里就要加一段
    }
};

// 想加"宏"（一键多动作）：遥控器要硬编码一组调用顺序
void partyMode() {
    light_->on();
    ac_->on();
    stereo_->on();   // 顺序写死在调用者里，无法复用、无法撤销
}
```

### 有命令模式 — 横切能力由命令对象自己承担

```cpp
// 撤销：每条命令自己记下"执行前状态"，undo() 负责还原
class LightOnCommand : public Command {
    Light* light_;
    bool wasOn_;                    // 执行前记录状态
public:
    void execute() { wasOn_ = light_->isOn(); light_->on(); }
    void undo()    { if (wasOn_) light_->on(); else light_->off(); }
};

// 宏命令：用组合模式把多条命令组合成一个 Command
class MacroCommand : public Command {
    std::vector<Command*> cmds_;
public:
    void execute() { for (auto* c : cmds_) c->execute(); }
    void undo()    { for (auto it = cmds_.rbegin(); it != cmds_.rend(); ++it) (*it)->undo(); }
};

// 遥控器撤销逻辑只有一行：lastCommand->undo()，与设备种类无关
remote.pressUndo();
```

**关键区别**：

```
没有命令模式：撤销/宏等能力散落在发起者里，每加一种设备都要改发起者
有命令模式：  撤销由命令自己实现，宏也是一条命令，发起者只调用 execute/undo
```

### 总结

```
发起者（Invoker）：只管"什么时候执行哪条命令"
命令（Command）：  封装"对谁、做什么、怎么撤销"
接收者（Receiver）：真正干活的业务对象
```

## 实现说明

本目录中的 `command.cpp` 以**智能家居遥控器**为例：

### 类结构

- **Command**（抽象命令）— 声明 `execute()` / `undo()` 接口
- **NoCommand**（空命令）— Null Object，占据未绑定槽位，调用始终安全
- **Receiver**
  - `Light` — 开/关
  - `AirConditioner` — 开/关 + 温度
  - `Stereo` — 开/关 + 音量
- **ConcreteCommand**
  - `LightOnCommand` / `LightOffCommand`
  - `AirConditionerOnCommand` / `AirConditionerOffCommand`
  - `SetTemperatureCommand` — 带参数命令，演示"参数化请求"
  - `StereoOnCommand` / `StereoOffCommand`
  - `MacroCommand` — 宏命令，组合多条命令
- **RemoteControl**（Invoker）— 3 个槽位，每槽绑定开/关一对命令；维护执行历史栈，支持连续撤销

### 关键设计决策

1. **撤销由命令自行实现**：每条命令在 `execute()` 前记录接收者状态（如 `wasOn_`、`prevTemp_`），`undo()` 据此还原。这样撤销逻辑与设备种类解耦，发起者只需 `lastCommand->undo()`。
2. **历史栈支持连续撤销**：`RemoteControl` 用 `vector` 记录每次执行的命令，`pressUndo()` 弹栈并撤销，可连续回退多步（而非只撤一步）。
3. **宏命令逆序撤销**：`MacroCommand::undo()` 按执行的反序撤销子命令，保证中间状态正确还原（后执行的先撤）。
4. **shared_ptr 共享命令**：同一条命令既可单独装到按钮上，也可作为宏的一部分，用 `shared_ptr` 共享所有权，互不影响。
5. **NoCommand 占位**：未绑定的槽位用 `NoCommand`，调用方无需判空，符合 Null Object 模式。

## 编译运行

```bash
g++ -std=c++17 -Wall -Wextra -o command command.cpp && ./command
```

## 要点

- **请求对象化** — 方法调用变成对象，于是可以参数化（按钮装哪条命令）、排队（命令队列）、记录日志
- **发起者与接收者解耦** — 遥控器只依赖 `Command` 接口，新增设备/动作不改动遥控器
- **撤销可扩展** — 每条命令自管撤销状态，发起者无需为每种设备写还原逻辑
- **可组合** — 宏命令用组合模式把多条命令合成一个，对外仍是一条 `Command`

## 与其他模式的关系

- **组合模式 (Composite)**：宏命令 `MacroCommand` 正是组合模式的应用——把多条命令组合成一棵树，对外仍是统一的 `Command` 接口，`execute`/`undo` 递归传播。
- **备忘录模式 (Memento)**：两者都解决"撤销/恢复"问题。命令模式的撤销靠"命令自行记录执行前状态"（本例做法）；备忘录模式靠"保存接收者完整快照"再还原。复杂场景下命令内部可借助备忘录保存状态。
- **中介者模式 (Mediator)**：中介者解耦"多对象之间的通信关系"（网状变星形）；命令模式解耦"请求发起者与接收者"。前者关注对象间通信，后者关注把单次请求对象化。
- **策略模式 (Strategy)**：命令与策略结构相似（都把行为封装成对象）。区别在于意图：策略模式用于"算法可互换"，命令模式用于"请求的参数化/排队/撤销"。命令通常有 `undo`，策略一般没有。
