# AI Chat Desktop (Codex) UI 优化方向

Qt Widgets 框架内可实现的界面优化方案。

**约束：** 不使用 QML，在当前 C++ + .qss 技术栈内优化。

**日期：** 2026-06-02

---

## 一、当前 UI 现状

| 组件 | 行数 | 当前状态 | 优化空间 |
|---|---|---|---|
| `MainWindow` | 767 | 侧栏 + 聊天区 + 输入区，布局合理但缺乏层次感 | 中 |
| `MessageWidget` | 299 | HTML 渲染 Markdown，代码块解析有问题 | **大** |
| `SettingsDialog` | 237 | 标准表单，功能完整 | 小 |
| `RolePromptDialog` | 323 | 模板列表 + 编辑区 | 小 |
| `AgentPlanDialog` | 313 | 步骤列表 + 状态标签 | 中 |
| `FileToolsDialog` | 301 | 文件选择 + 执行 | 中 |
| `ToolsDialog` | 191 | 工具选择 + 输入输出 | 小 |
| `ChatView` | 91 | 消息列表容器 | **大** |
| `LogViewerDialog` | 110 | 纯文本 + 按钮 | 小 |
| `app.qss` | 345 | 基础灰色系，颜色单一 | **大** |

---

## 二、优先级分类

### P0：立即见效（1-2 天）

| 优化项 | 效果 | 工作量 |
|---|---|---|
| **MessageWidget 代码块独立渲染** | 代码块脱离 HTML，用原生 QPlainTextEdit 渲染，支持语法着色 | 0.5 天 |
| **消息淡入动画** | QPropertyAnimation 控制 opacity，新消息平滑出现 | 0.5 天 |
| **AI 回复打字机效果** | 流式文本逐字显示时，滚动条平滑跟随 | 0.5 天 |

### P1：体验提升（3-5 天）

| 优化项 | 效果 | 工作量 |
|---|---|---|
| **暗色主题** | 深色模式切换，独立 .qss 文件 | 1 天 |
| **侧栏收起/展开动画** | QPropertyAnimation 控制 sidebar width（240px ↔ 48px 图标模式） | 1 天 |
| **消息状态指示器** | 发送中 / 生成中 / 失败 / 成功 的不同视觉状态 | 0.5 天 |
| **代码块行号 + 高亮** | 用 QPlainTextEdit 的 `lineNumberArea` 实现 | 1 天 |

### P2：细节打磨（可选）

| 优化项 | 效果 | 工作量 |
|---|---|---|
| **自定义滚动条** | QScrollBar 样式美化 | 0.5 天 |
| **拖拽文件到输入框** | drag & drop 上传图片/文件 | 0.5 天 |
| **消息右键菜单** | 复制/删除/重新生成 | 0.5 天 |
| **Toast 通知** | 设置保存成功 / 导出完成的短暂弹出提示 | 1 天 |

---

## 三、P0 详细方案

### 3.1 MessageWidget 代码块独立渲染

**当前问题：**

代码块通过 `QTextDocument::setMarkdown()` → `toHtml()` → `QLabel` 渲染，存在：
- ```` ``` ```` 标记解析不稳定
- 代码和普通文本混在一个 QLabel 里
- 无法独立复制代码块
- 无行号、无语法着色

**方案：分析 Markdown 结构，代码块用独立 QPlainTextEdit**

```cpp
void MessageWidget::rebuildContent()
{
    clearContentWidgets();
    
    // 解析内容，按段落类型拆分
    const auto parts = parseContentParts(m_content);
    
    for (const auto &part : parts) {
        if (part.codeBlock) {
            addCodeBlockWidget(part.language, part.content);
        } else {
            addTextWidget(part.content);
        }
    }
}

void MessageWidget::addCodeBlockWidget(
    const QString &language, const QString &code)
{
    auto *container = new QFrame(this);
    container->setObjectName("codeBlockContainer");
    
    // 顶部栏：语言标签 + 复制按钮
    auto *header = new QHBoxLayout();
    auto *langLabel = new QLabel(language.isEmpty() 
        ? "code" : language);
    langLabel->setObjectName("codeLangLabel");
    auto *copyBtn = new QPushButton("复制");
    copyBtn->setObjectName("codeCopyButton");
    header->addWidget(langLabel);
    header->addStretch();
    header->addWidget(copyBtn);
    
    // 代码区
    auto *codeArea = new QPlainTextEdit();
    codeArea->setReadOnly(true);
    codeArea->setPlainText(code);
    codeArea->setObjectName("codeArea");
    // 设置等宽字体
    QFont monoFont("Consolas", 12);
    monoFont.setStyleHint(QFont::Monospace);
    codeArea->setFont(monoFont);
    
    // 行号区域
    // 继承 QPlainTextEdit 重写 lineNumberAreaPaintEvent
    
    connect(copyBtn, &QPushButton::clicked, this, [code]() {
        QApplication::clipboard()->setText(code);
    });
}
```

**对应 .qss 样式：**

```css
QFrame#codeBlockContainer {
    background: #f8f9fb;
    border: 1px solid #d8dee6;
    border-radius: 8px;
    margin: 8px 0;
}

QLabel#codeLangLabel {
    color: #6b7280;
    font-size: 12px;
    font-family: "Consolas", "Cascadia Mono", monospace;
    padding: 6px 10px;
}

QPushButton#codeCopyButton {
    min-height: 26px;
    padding: 2px 10px;
    font-size: 12px;
    border: 1px solid #c8d0da;
    border-radius: 4px;
    background: #ffffff;
}

QPlainTextEdit#codeArea {
    background: #ffffff;
    border: none;
    border-top: 1px solid #e5e7eb;
    font-family: "Consolas", "Cascadia Mono", monospace;
    font-size: 13px;
    padding: 10px;
    selection-background-color: #bfdbfe;
}
```

### 3.2 消息淡入动画

**效果**：新消息从透明渐变到不透明，而不是突然出现。

```cpp
// ChatView::addMessageWidget(MessageWidget *widget)
void ChatView::addMessageWidget(MessageWidget *widget)
{
    // 添加到布局
    m_layout->addWidget(widget);
    
    // 创建透明度动画
    auto *effect = new QGraphicsOpacityEffect(widget);
    widget->setGraphicsEffect(effect);
    effect->setOpacity(0.0);
    
    auto *animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(300);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    
    // 动画结束后移除 effect（避免性能影响）
    connect(animation, &QPropertyAnimation::finished, widget, [widget]() {
        widget->setGraphicsEffect(nullptr);
    });
}
```

### 3.3 AI 回复打字机效果（滚动优化）

**当前问题：** 流式响应时，每次文本更新都 `scrollToBottom()`，滚动生硬。

```cpp
// ChatView::updateLastAiMessage(const QString &content)
void ChatView::updateLastAiMessage(const QString &content)
{
    // 更新消息内容
    m_lastAiWidget->setContent(content);
    
    // 平滑滚动到底部
    QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
    
    // 如果用户已经在底部附近，自动跟随
    // 如果用户手动滚上去了，不强制跟随
    bool atBottom = scrollBar->value() >= 
                    scrollBar->maximum() - 50;
    
    if (atBottom) {
        // 用动画平滑滚动
        auto *anim = new QPropertyAnimation(scrollBar, "value");
        anim->setDuration(150);
        anim->setStartValue(scrollBar->value());
        anim->setEndValue(scrollBar->maximum());
        anim->setEasingCurve(QEasingCurve::OutQuad);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}
```

---

## 四、P1 详细方案

### 4.1 暗色主题

**方案：** 不修改 C++ 代码，纯 `.qss` 切换。

当前代码结构已经支持——`main.cpp` 里加载 `.qss` 文件：

```cpp
void loadApplicationStyle(QApplication &app)
{
    QFile styleFile(QStringLiteral(":/styles/app.qss"));
    // ...
}
```

**改造：增加暗色 .qss + 切换逻辑**

```
resources/styles/
├── app.qss          # 亮色主题（当前）
├── app-dark.qss     # 暗色主题（新增）
```

**亮色 vs 暗色主题色板：**

| 元素 | 亮色 | 暗色 |
|---|---|---|
| 主背景 | `#f7f8fa` | `#1a1b2e` |
| 侧栏背景 | `#eef1f4` | `#232438` |
| 卡片/气泡 | `#ffffff` | `#2d2f45` |
| 主文字 | `#1f2933` | `#e2e4ed` |
| 次要文字 | `#6b7280` | `#8b8fa6` |
| 按钮背景 | `#ffffff` | `#363852` |
| 输入框 | `#ffffff` | `#252742` |
| AI 气泡 | `#f0f4ff` | `#2a2c40` |
| 代码块 | `#f8f9fb` | `#1e2038` |
| 分隔线 | `#d8dee6` | `#3a3d56` |
| 强调色（蓝色） | `#2563eb` | `#5b8def` |

**切换逻辑：**

```cpp
// AppConfig 新增
struct AppConfig {
    // ...
    bool darkMode = false;
};

// main.cpp 或 MainWindow 中
void MainWindow::applyTheme(bool dark)
{
    QString qssPath = dark 
        ? ":/styles/app-dark.qss" 
        : ":/styles/app.qss";
    
    QFile styleFile(qssPath);
    styleFile.open(QFile::ReadOnly | QFile::Text);
    qApp->setStyleSheet(QTextStream(&styleFile).readAll());
}
```

### 4.2 侧栏收起/展开动画

```cpp
// 收起侧栏 → 只显示图标
void MainWindow::toggleSidebar()
{
    auto *anim = new QPropertyAnimation(m_sidebar, "maximumWidth");
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    
    if (m_sidebarExpanded) {
        // 收起: 240 → 48
        anim->setStartValue(240);
        anim->setEndValue(48);
        // 隐藏文字，只保留图标
    } else {
        anim->setStartValue(48);
        anim->setEndValue(240);
    }
    
    anim->start(QAbstractAnimation::DeleteWhenStopped);
    m_sidebarExpanded = !m_sidebarExpanded;
}
```

### 4.3 消息状态指示器

在 `MessageWidget` 中增加状态标识：

```cpp
enum class MessageStatus {
    Pending,     // 等待发送
    Sending,     // 发送中（旋转动画）
    Generating,  // AI 生成中（打字动画）
    Success,     // 成功
    Failed       // 失败（红色标记 + 重试按钮）
};
```

**AI 生成中：三点跳动动画**

```cpp
// 在消息底部显示 "AI 正在生成..." + 跳动点
// 用 QLabel + QTimer 实现三点循环动画："." → ".." → "..." → "."
```

### 4.4 代码块行号 + 语法高亮

```cpp
class CodeEditor : public QPlainTextEdit {
public:
    CodeEditor(QWidget *parent = nullptr) : QPlainTextEdit(parent) {
        m_lineNumberArea = new LineNumberArea(this);
        connect(this, &QPlainTextEdit::blockCountChanged,
                this, &CodeEditor::updateLineNumberAreaWidth);
        connect(this, &QPlainTextEdit::updateRequest,
                this, &CodeEditor::updateLineNumberArea);
        updateLineNumberAreaWidth(0);
    }

    void lineNumberAreaPaintEvent(QPaintEvent *event) {
        QPainter painter(m_lineNumberArea);
        painter.fillRect(event->rect(), QColor("#f1f5f9"));
        
        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = (int)blockBoundingGeometry(
            block).translated(contentOffset()).top();
        int bottom = top + (int)blockBoundingRect(block).height();
        
        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(QColor("#9ca3af"));
                painter.drawText(0, top, 
                    m_lineNumberArea->width() - 4, 
                    fontMetrics().height(),
                    Qt::AlignRight | Qt::AlignVCenter, number);
            }
            block = block.next();
            top = bottom;
            bottom = top + (int)blockBoundingRect(block).height();
            ++blockNumber;
        }
    }

private:
    QWidget *m_lineNumberArea;
    
    class LineNumberArea : public QWidget {
    public:
        LineNumberArea(CodeEditor *editor) 
            : QWidget(editor), m_editor(editor) {}
        QSize sizeHint() const override {
            return QSize(m_editor->lineNumberAreaWidth(), 0);
        }
    protected:
        void paintEvent(QPaintEvent *e) override {
            m_editor->lineNumberAreaPaintEvent(e);
        }
    private:
        CodeEditor *m_editor;
    };
};
```

---

## 五、优化前后代码量预估

| 组件 | 当前行数 | 优化后 | 变化 |
|---|---|---|---|
| `MessageWidget` | 299 | ~400 | +101（代码块独立渲染 + 状态指示器） |
| `ChatView` | 91 | ~180 | +89（动画 + 滚动优化） |
| `MainWindow` | 767 | ~820 | +53（侧栏动画 + 主题切换） |
| `app.qss` | 345 | ~700 | +355（暗色主题 + 组件细化样式） |
| 新增：`CodeEditor` | 0 | ~100 | 代码块行号 + 高亮 |
| 新增：`app-dark.qss` | 0 | ~300 | 暗色主题样式 |

---

## 六、推荐执行顺序

```
Day 1-2: P0 核心体验
  → MessageWidget 代码块独立渲染
  → 消息淡入动画
  → AI 回复滚动优化

Day 3-5: P1 主题 + 动画
  → 暗色主题（app-dark.qss）
  → 侧栏收起/展开动画
  → 消息状态指示器

Day 6-7: P1 代码增强
  → 代码块行号 + 语法高亮 (CodeEditor)

Day 8+: P2 细节
  → 自定义滚动条
  → 拖拽文件
  → 右键菜单
  → Toast 通知
```

---

> 本文档专注 Qt Widgets 框架内的优化。如需 QML 迁移评估，参见 [优化方向.md 第十二章](优化方向.md)。
