#include "ui/ToolsDialog.h"

#include "tools/JsonCompactTool.h"
#include "tools/JsonFormatTool.h"
#include "tools/MarkdownCleanupTool.h"
#include "tools/TextCleanupTool.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ToolsDialog::ToolsDialog(AppLanguage language, QWidget *parent)
    : QDialog(parent)
    , m_language(language)
{
    registerTools();
    setupUi();
    applyLanguage();
    updateSelectedToolDescription();
    updateOutputActions(false);
}

void ToolsDialog::setupUi()
{
    setMinimumSize(760, 560);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(12);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("toolsTitleLabel"));

    m_toolComboBox = new QComboBox(this);
    m_toolComboBox->setObjectName(QStringLiteral("toolComboBox"));
    for (int index = 0; index < static_cast<int>(m_tools.size()); ++index) {
        const LocalTool *tool = m_tools[static_cast<size_t>(index)].get();
        m_toolComboBox->addItem(tool->displayName(m_language), tool->id());
    }

    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setObjectName(QStringLiteral("toolDescriptionLabel"));
    m_descriptionLabel->setWordWrap(true);

    m_inputEdit = new QPlainTextEdit(this);
    m_inputEdit->setObjectName(QStringLiteral("toolInputEdit"));

    m_outputEdit = new QPlainTextEdit(this);
    m_outputEdit->setObjectName(QStringLiteral("toolOutputEdit"));
    m_outputEdit->setReadOnly(true);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("toolStatusLabel"));
    m_statusLabel->setWordWrap(true);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);

    m_runButton = new QPushButton(this);
    m_runButton->setObjectName(QStringLiteral("runToolButton"));

    m_copyButton = new QPushButton(this);
    m_copyButton->setObjectName(QStringLiteral("copyToolOutputButton"));

    m_insertButton = new QPushButton(this);
    m_insertButton->setObjectName(QStringLiteral("insertToolOutputButton"));

    m_closeButton = new QPushButton(this);
    m_closeButton->setObjectName(QStringLiteral("closeToolsButton"));

    buttonLayout->addWidget(m_runButton);
    buttonLayout->addWidget(m_copyButton);
    buttonLayout->addWidget(m_insertButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_closeButton);

    rootLayout->addWidget(m_titleLabel);
    rootLayout->addWidget(m_toolComboBox);
    rootLayout->addWidget(m_descriptionLabel);
    rootLayout->addWidget(m_inputEdit, 1);
    rootLayout->addWidget(m_outputEdit, 1);
    rootLayout->addWidget(m_statusLabel);
    rootLayout->addLayout(buttonLayout);

    connect(m_toolComboBox, &QComboBox::currentIndexChanged, this, &ToolsDialog::updateSelectedToolDescription);
    connect(m_runButton, &QPushButton::clicked, this, &ToolsDialog::runSelectedTool);
    connect(m_copyButton, &QPushButton::clicked, this, &ToolsDialog::copyOutput);
    connect(m_insertButton, &QPushButton::clicked, this, &ToolsDialog::insertOutput);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ToolsDialog::registerTools()
{
    m_tools.push_back(std::make_unique<JsonFormatTool>());
    m_tools.push_back(std::make_unique<JsonCompactTool>());
    m_tools.push_back(std::make_unique<MarkdownCleanupTool>());
    m_tools.push_back(std::make_unique<TextCleanupTool>());
}

void ToolsDialog::applyLanguage()
{
    setWindowTitle(text(QStringLiteral("Tools"), QStringLiteral("工具")));
    m_titleLabel->setText(text(QStringLiteral("Local Tools"), QStringLiteral("本地工具")));
    m_inputEdit->setPlaceholderText(text(QStringLiteral("Paste input text here..."), QStringLiteral("在这里粘贴输入文本...")));
    m_outputEdit->setPlaceholderText(text(QStringLiteral("Tool output will appear here."), QStringLiteral("工具输出会显示在这里。")));
    m_runButton->setText(text(QStringLiteral("Run"), QStringLiteral("运行")));
    m_copyButton->setText(text(QStringLiteral("Copy Output"), QStringLiteral("复制输出")));
    m_insertButton->setText(text(QStringLiteral("Insert to Chat Input"), QStringLiteral("插入聊天输入框")));
    m_closeButton->setText(text(QStringLiteral("Close"), QStringLiteral("关闭")));
    m_statusLabel->setText(text(QStringLiteral("Ready."), QStringLiteral("准备就绪。")));
}

void ToolsDialog::updateSelectedToolDescription()
{
    const LocalTool *tool = selectedTool();
    if (tool == nullptr) {
        m_descriptionLabel->clear();
        return;
    }

    m_descriptionLabel->setText(tool->description(m_language));
}

void ToolsDialog::runSelectedTool()
{
    const LocalTool *tool = selectedTool();
    if (tool == nullptr) {
        return;
    }

    const ToolResult result = tool->run(m_inputEdit->toPlainText());
    if (!result.ok) {
        m_outputEdit->clear();
        m_statusLabel->setText(result.error);
        updateOutputActions(false);
        return;
    }

    m_outputEdit->setPlainText(result.output);
    m_statusLabel->setText(text(QStringLiteral("Completed."), QStringLiteral("已完成。")));
    updateOutputActions(!result.output.isEmpty());
}

void ToolsDialog::copyOutput()
{
    const QString output = m_outputEdit->toPlainText();
    if (output.isEmpty()) {
        return;
    }

    QApplication::clipboard()->setText(output);
    m_statusLabel->setText(text(QStringLiteral("Output copied."), QStringLiteral("输出已复制。")));
}

void ToolsDialog::insertOutput()
{
    const QString output = m_outputEdit->toPlainText();
    if (output.isEmpty()) {
        return;
    }

    emit outputInsertionRequested(output);
    m_statusLabel->setText(text(QStringLiteral("Output inserted into chat input."), QStringLiteral("输出已插入聊天输入框。")));
}

QString ToolsDialog::text(const QString &english, const QString &chinese) const
{
    return m_language == AppLanguage::English ? english : chinese;
}

const LocalTool *ToolsDialog::selectedTool() const
{
    const int index = m_toolComboBox->currentIndex();
    if (index < 0 || index >= static_cast<int>(m_tools.size())) {
        return nullptr;
    }

    return m_tools[static_cast<size_t>(index)].get();
}

void ToolsDialog::updateOutputActions(bool enabled)
{
    m_copyButton->setEnabled(enabled);
    m_insertButton->setEnabled(enabled);
}
