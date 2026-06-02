#include "ui/FileToolsDialog.h"

#include "support/AppLogger.h"
#include "tools/FileInteractionService.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

FileToolsDialog::FileToolsDialog(AppLanguage language, QWidget *parent)
    : QDialog(parent)
    , m_language(language)
{
    setupUi();
    applyLanguage();
    updateOutputActions(false);
}

void FileToolsDialog::setupUi()
{
    setMinimumSize(780, 560);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(12);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("fileToolsTitleLabel"));

    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setObjectName(QStringLiteral("fileToolsDescriptionLabel"));
    m_descriptionLabel->setWordWrap(true);

    auto *primaryButtonLayout = new QHBoxLayout();
    primaryButtonLayout->setContentsMargins(0, 0, 0, 0);
    primaryButtonLayout->setSpacing(10);

    m_readFileButton = new QPushButton(this);
    m_readFileButton->setObjectName(QStringLiteral("readFileButton"));

    m_listDirectoryButton = new QPushButton(this);
    m_listDirectoryButton->setObjectName(QStringLiteral("listDirectoryButton"));

    m_saveOutputButton = new QPushButton(this);
    m_saveOutputButton->setObjectName(QStringLiteral("saveFileOutputButton"));

    primaryButtonLayout->addWidget(m_readFileButton);
    primaryButtonLayout->addWidget(m_listDirectoryButton);
    primaryButtonLayout->addWidget(m_saveOutputButton);
    primaryButtonLayout->addStretch(1);

    auto *openButtonLayout = new QHBoxLayout();
    openButtonLayout->setContentsMargins(0, 0, 0, 0);
    openButtonLayout->setSpacing(10);

    m_openFileButton = new QPushButton(this);
    m_openFileButton->setObjectName(QStringLiteral("openFileButton"));

    m_openDirectoryButton = new QPushButton(this);
    m_openDirectoryButton->setObjectName(QStringLiteral("openDirectoryButton"));

    openButtonLayout->addWidget(m_openFileButton);
    openButtonLayout->addWidget(m_openDirectoryButton);
    openButtonLayout->addStretch(1);

    m_outputEdit = new QPlainTextEdit(this);
    m_outputEdit->setObjectName(QStringLiteral("fileToolOutputEdit"));
    m_outputEdit->setReadOnly(true);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("fileToolStatusLabel"));
    m_statusLabel->setWordWrap(true);

    auto *footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(0, 0, 0, 0);
    footerLayout->setSpacing(10);

    m_copyButton = new QPushButton(this);
    m_copyButton->setObjectName(QStringLiteral("copyFileToolOutputButton"));

    m_insertButton = new QPushButton(this);
    m_insertButton->setObjectName(QStringLiteral("insertFileToolOutputButton"));

    m_closeButton = new QPushButton(this);
    m_closeButton->setObjectName(QStringLiteral("closeFileToolsButton"));

    footerLayout->addWidget(m_copyButton);
    footerLayout->addWidget(m_insertButton);
    footerLayout->addStretch(1);
    footerLayout->addWidget(m_closeButton);

    rootLayout->addWidget(m_titleLabel);
    rootLayout->addWidget(m_descriptionLabel);
    rootLayout->addLayout(primaryButtonLayout);
    rootLayout->addLayout(openButtonLayout);
    rootLayout->addWidget(m_outputEdit, 1);
    rootLayout->addWidget(m_statusLabel);
    rootLayout->addLayout(footerLayout);

    connect(m_readFileButton, &QPushButton::clicked, this, &FileToolsDialog::readSelectedFile);
    connect(m_listDirectoryButton, &QPushButton::clicked, this, &FileToolsDialog::listSelectedDirectory);
    connect(m_saveOutputButton, &QPushButton::clicked, this, &FileToolsDialog::saveOutputToFile);
    connect(m_openFileButton, &QPushButton::clicked, this, &FileToolsDialog::openSelectedFile);
    connect(m_openDirectoryButton, &QPushButton::clicked, this, &FileToolsDialog::openSelectedDirectory);
    connect(m_copyButton, &QPushButton::clicked, this, &FileToolsDialog::copyOutput);
    connect(m_insertButton, &QPushButton::clicked, this, &FileToolsDialog::insertOutput);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void FileToolsDialog::applyLanguage()
{
    setWindowTitle(text(QStringLiteral("File Tools"), QStringLiteral("文件工具")));
    m_titleLabel->setText(text(QStringLiteral("Controlled File Tools"), QStringLiteral("受控文件工具")));
    m_descriptionLabel->setText(text(
        QStringLiteral("Choose files or folders explicitly. File content is not written to logs, and tool output is not sent automatically."),
        QStringLiteral("请显式选择文件或文件夹。文件正文不会写入日志，工具输出也不会自动发送。")));
    m_readFileButton->setText(text(QStringLiteral("Read File"), QStringLiteral("读取文件")));
    m_listDirectoryButton->setText(text(QStringLiteral("List Folder"), QStringLiteral("列出文件夹")));
    m_saveOutputButton->setText(text(QStringLiteral("Save Output"), QStringLiteral("保存输出")));
    m_openFileButton->setText(text(QStringLiteral("Open File"), QStringLiteral("打开文件")));
    m_openDirectoryButton->setText(text(QStringLiteral("Open Folder"), QStringLiteral("打开文件夹")));
    m_copyButton->setText(text(QStringLiteral("Copy Output"), QStringLiteral("复制输出")));
    m_insertButton->setText(text(QStringLiteral("Insert to Chat Input"), QStringLiteral("插入聊天输入框")));
    m_closeButton->setText(text(QStringLiteral("Close"), QStringLiteral("关闭")));
    m_outputEdit->setPlaceholderText(text(QStringLiteral("File tool output will appear here."),
                                          QStringLiteral("文件工具输出会显示在这里。")));
    m_statusLabel->setText(text(QStringLiteral("Ready."), QStringLiteral("准备就绪。")));
}

void FileToolsDialog::readSelectedFile()
{
    const QString filePath = QFileDialog::getOpenFileName(this, text(QStringLiteral("Read file"), QStringLiteral("读取文件")));
    if (filePath.isEmpty()) {
        return;
    }

    const ToolResult result = FileInteractionService::readTextFile(filePath);
    if (!result.ok) {
        setOutput(QString(), result.error);
        return;
    }

    setOutput(result.output, text(QStringLiteral("File read completed."), QStringLiteral("文件读取完成。")));
}

void FileToolsDialog::listSelectedDirectory()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(this, text(QStringLiteral("List folder"), QStringLiteral("列出文件夹")));
    if (directoryPath.isEmpty()) {
        return;
    }

    const ToolResult result = FileInteractionService::listDirectory(directoryPath);
    if (!result.ok) {
        setOutput(QString(), result.error);
        return;
    }

    setOutput(result.output, text(QStringLiteral("Folder listed."), QStringLiteral("文件夹已列出。")));
}

void FileToolsDialog::saveOutputToFile()
{
    const QString output = m_outputEdit->toPlainText();
    if (output.isEmpty()) {
        m_statusLabel->setText(text(QStringLiteral("There is no output to save."), QStringLiteral("当前没有可保存的输出。")));
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        text(QStringLiteral("Save output"), QStringLiteral("保存输出")),
        QString(),
        QStringLiteral("Text (*.txt);;Markdown (*.md);;All files (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    bool allowOverwrite = false;
    if (QFileInfo::exists(filePath)) {
        const QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            text(QStringLiteral("Overwrite file"), QStringLiteral("覆盖文件")),
            text(QStringLiteral("The selected file already exists. Overwrite it?"),
                 QStringLiteral("选择的文件已存在。是否覆盖？")),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            m_statusLabel->setText(text(QStringLiteral("Save canceled."), QStringLiteral("已取消保存。")));
            return;
        }
        allowOverwrite = true;
    }

    const ToolResult result = FileInteractionService::saveTextFile(filePath, output, allowOverwrite);
    m_statusLabel->setText(result.ok ? result.output : result.error);
}

void FileToolsDialog::openSelectedFile()
{
    const QString filePath = QFileDialog::getOpenFileName(this, text(QStringLiteral("Open file"), QStringLiteral("打开文件")));
    if (filePath.isEmpty()) {
        return;
    }

    confirmAndOpenPath(filePath);
}

void FileToolsDialog::openSelectedDirectory()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(this, text(QStringLiteral("Open folder"), QStringLiteral("打开文件夹")));
    if (directoryPath.isEmpty()) {
        return;
    }

    confirmAndOpenPath(directoryPath);
}

void FileToolsDialog::copyOutput()
{
    const QString output = m_outputEdit->toPlainText();
    if (output.isEmpty()) {
        return;
    }

    QApplication::clipboard()->setText(output);
    m_statusLabel->setText(text(QStringLiteral("Output copied."), QStringLiteral("输出已复制。")));
}

void FileToolsDialog::insertOutput()
{
    const QString output = m_outputEdit->toPlainText();
    if (output.isEmpty()) {
        return;
    }

    emit outputInsertionRequested(output);
    m_statusLabel->setText(text(QStringLiteral("Output inserted into chat input."), QStringLiteral("输出已插入聊天输入框。")));
}

void FileToolsDialog::updateOutputActions(bool enabled)
{
    m_copyButton->setEnabled(enabled);
    m_insertButton->setEnabled(enabled);
    m_saveOutputButton->setEnabled(enabled);
}

void FileToolsDialog::setOutput(const QString &output, const QString &status)
{
    m_outputEdit->setPlainText(output);
    m_statusLabel->setText(status);
    updateOutputActions(!output.isEmpty());
}

void FileToolsDialog::confirmAndOpenPath(const QString &path)
{
    const ToolResult validation = FileInteractionService::validateOpenPath(path);
    if (!validation.ok) {
        m_statusLabel->setText(validation.error);
        return;
    }

    const QMessageBox::StandardButton choice = QMessageBox::question(
        this,
        text(QStringLiteral("Open path"), QStringLiteral("打开路径")),
        text(QStringLiteral("Open this file or folder?\n\n%1"), QStringLiteral("打开这个文件或文件夹？\n\n%1")).arg(path),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice != QMessageBox::Yes) {
        m_statusLabel->setText(text(QStringLiteral("Open canceled."), QStringLiteral("已取消打开。")));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        AppLogger::warning(QStringLiteral("FileInteraction"),
                           QStringLiteral("open_path failed. path=%1")
                               .arg(FileInteractionService::pathSummary(path)));
        m_statusLabel->setText(text(QStringLiteral("Failed to open the selected path."),
                                    QStringLiteral("打开所选路径失败。")));
        return;
    }

    AppLogger::info(QStringLiteral("FileInteraction"),
                    QStringLiteral("open_path requested. path=%1")
                        .arg(FileInteractionService::pathSummary(path)));
    m_statusLabel->setText(text(QStringLiteral("Open request sent."), QStringLiteral("已请求打开。")));
}

QString FileToolsDialog::text(const QString &english, const QString &chinese) const
{
    return m_language == AppLanguage::English ? english : chinese;
}
