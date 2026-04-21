# CardMatch (Cocos2d-x 3.17)

这是一个使用 Cocos2d-x 3.17 引擎开发的纸牌消除游戏。游戏采用严格的 MVC 架构，具备流畅的动画交互、完善的判定逻辑以及多步回退功能。

### 环境配置指南

本指南旨在帮助开发者快速搭建并运行CardGame项目。请确保在开始之前已安装所有必要的软件。

#### 操作系统
- **Windows 10/11 (64位)**
- 推荐使用最新版本的Windows以获得最佳的兼容性和性能。

#### 集成开发环境
- **Visual Studio 2022 Community**
- 在安装时，请务必勾选“使用C++的桌面开发”工作负载。

#### Cocos2d-x引擎
- **版本:** 建议与项目使用的版本保持一致。
- **配置:**
    - 下载并解压Cocos2d-x引擎源码。
    - 运行引擎目录下的`setup.py`脚本，并根据提示设置环境变量。
    - 安装完成后，重启终端或电脑以使环境变量生效。

#### Git版本控制
- **Git for Windows**
- 用于代码的版本控制和同步。安装后可使用Git Bash或直接在Visual Studio中进行操作。

#### 项目构建
1. 打开`CardGame/proj.win32/CardGame.sln`解决方案文件。
2. 在Visual Studio中，确保解决方案配置为`Debug`或`Release`，平台为`Win32`。
3. 右键点击`CardGame`项目，选择“生成”以编译项目。
4. 编译成功后，可直接在Visual Studio中启动游戏，或运行生成的`.exe`文件。


## 核心功能

- **点击匹配机制**：点击桌面牌（Tableau），若点数与手牌区顶牌差值为 1（支持 A-K 循环匹配），卡片自动平移至手牌区完成匹配。
- **自动翻牌**：当桌面列底部的牌被移走后，上一张牌会自动翻开变为可操作状态。
- **发牌堆交互**：点击左下角发牌堆（Stock），卡片平滑移动至手牌区作为新的匹配目标。
- **多步回退 (Undo)**：支持无限步数回退。回退时，卡片会带有反向平移的动画效果，完美还原操作前的状态。
- **实时统计显示**：
    - **剩余牌数 (Remaining)**：右上角实时显示主牌堆剩余未消除的牌数。
    - **发牌堆计数 (Stock Count)**：左下角显示发牌堆剩余张数，为空时显示 "REFRESH"。
- **游戏结算逻辑**：
    - **胜利**：主牌堆全部消除时弹出 "CONGRATULATIONS!"。
    - **失败**：当发牌堆为空且桌面无合法移动时弹出 "NO MORE MOVES"。
- **一键重启**：结算弹窗提供 "RESTART" 按钮，无需重新加载场景即可快速重置游戏。

## 项目架构

项目遵循 **MVC (Model-View-Controller)** 设计模式：

### 1. Model (数据层)
- **[CardModel.h]**: 定义单张扑克牌的属性（点数、花色、正反面）。
- **[GameModel.h]**: 
    - 维护游戏核心数据结构（主牌堆 `vector<vector>`、手牌堆、发牌堆）。
    - 处理逻辑判定：`canMatch`（点数差1判断）、`checkWin`、`checkGameOver`。
    - 备忘录模式实现回退：`saveState` 和 `undo`。

### 2. View (表现层)
- **[CardSprite.h]**: 
    - 负责卡片的复合渲染（底图、花色图标、数字标签）。
    - 实现选中高亮逻辑（DrawNode 绘制金边）。
- **[GameScene.h]** (View 部分):
    - 负责 UI 布局、背景渲染以及 `STOCK` 和 `REMAINING` 标签的显示。

### 3. Controller (控制层)
- **[GameScene.cpp]**:
    - **输入处理**：监听触摸事件，分发至 `onCardClicked` 或 `onStockClicked`。
    - **动画驱动**：使用 `MoveTo` 和 `Sequence` 实现卡片平移、替换和回退动画。
    - **界面同步**：`refreshAllCards` 确保视图与模型数据实时一致。

## 核心数据结构与方法说明

### 1. CardModel (基础数据模型)
定义在 [CardModel.h]。
- **数据成员**:
    - `int rank`: 点数 (1-13, 1代表A)。
    - `Suit suit`: 花色 (SPADE, HEART, CLUB, DIAMOND)。
    - `bool isFaceUp`: 正反面状态。

### 2. GameState (状态快照)
定义在 [GameModel.h]，用于回退系统。
- **数据成员**:
    - `vector<vector<CardModel*>> mainAreaCards`: 主牌堆所有牌的引用。
    - `vector<CardModel*> handAreaCards`: 手牌区历史记录。
    - `vector<CardModel*> stockCards`: 发牌堆剩余牌。
    - `vector<bool> faceUpStatus`: 所有卡片的翻开状态镜像。

### 3. GameModel (业务逻辑层)
定义在 [GameModel.h]。
- **关键方法**:
    - `setupGame()`: 初始化牌局，执行洗牌并分配到 7 列主牌堆（Tableau）及发牌堆。
    - `canMatch(r1, r2)`: 核心消除算法，判断两个点数差值是否为 1 (含 A-K 循环)。
    - `drawFromStock()`: 从 Stock 抽取一张牌并存入 `handAreaCards`。
    - `undo()`: 弹出历史堆栈，恢复上一回合的所有数据状态。
    - `checkGameOver()`: 深度检查死局逻辑（Stock 为空且桌面无可匹配项）。

### 4. CardSprite (显示组件)
定义在 [CardSprite.h]。
- **关键方法**:
    - `updateView()`: 根据 `CardModel` 的数据实时更新精灵的外观（数字、花色、颜色）。
    - `setSelected(bool)`: 切换选中高亮状态，动态创建或移除金边效果。

### 5. GameScene (控制中心)
定义在 [GameScene.cpp]。
- **关键方法**:
    - `onCardClicked(sprite)`: 处理点击逻辑，执行 `canMatch` 判定并播放飞向手牌区的平移动画。
    - `onUndoClicked()`: 实现“反向平移”回退动画。先进行逻辑回退，再对比新旧坐标执行 `MoveTo`。
    - `getCardPosition(model)`: 坐标映射核心，根据逻辑归属（哪一列、第几个）计算其在屏幕上的精确像素坐标。
    - `refreshAllCards()`: 强制同步方法，停止所有动作并将精灵对齐到当前逻辑位置。

## 关键技术点

- **坐标映射与同步**：通过 `getCardPosition` 方法，在回退动画中动态计算卡片在不同牌堆间的坐标映射。
- **C++11/14 兼容性**：确保在标准编译器环境下直接编译通过。
- **内存安全**：在 `resetGame` 和 `showGameResult` 中处理了监听器移除的异步安全，并解决了 `CardSprite` 更新视图时的悬挂指针问题。
- **动态布局**：卡片缩放比例根据屏幕宽度动态计算（约占屏幕宽度的 13%），支持不同分辨率下的自动适配。

## 开发环境要求

- **引擎**: Cocos2d-x 3.17
- **编译器**: MSVC (Visual Studio 2022)
- **语言标准**: C++11 / C++14
- **文件编码**: UTF-8 with BOM (解决中文字符编译报错)
