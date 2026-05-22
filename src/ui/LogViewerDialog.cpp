#include "ui/LogViewerDialog.h"

#include "support/LogFileReader.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QUrl>
#include <QVBoxLayout>

LogViewerDialog::LogViewerDialog(const QString &logFilePath, AppLanguage language, QWidget *parent)
    : QDialog(parent)
    , m_logFilePath(logFilePath)
    , m_language(language)
{
    setupUi();
    applyLanguage();
    refreshLogs();
}

void LogViewerDialog::setupUi()
{
    setMinimumSize(760, 520);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(12);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("logViewerTitle"));

    m_pathLabel = new QLabel(this);
    m_pathLabel->setObjectName(QStringLiteral("logPathLabel"));
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_pathLabel->setWordWrap(true);

    m_logTextEdit = new QPlainTextEdit(this);
    m_logTextEdit->setObjectName(QStringLiteral("logViewerText"));
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setObjectName(QStringLiteral("refreshLogsButton"));

    m_openDirectoryButton = new QPushButton(this);
    m_openDirectoryButton->setObjectName(QStringLiteral("openLogDirectoryButton"));

    m_closeButton = new QPushButton(this);
    m_closeButton->setObjectName(QStringLiteral("closeLogsButton"));

    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_openDirectoryButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_closeButton);

    rootLayout->addWidget(m_titleLabel);
    rootLayout->addWidget(m_pathLabel);
    rootLayout->addWidget(m_logTextEdit, 1);
    rootLayout->addLayout(buttonLayout);

    connect(m_refreshButton, &QPushButton::clicked, this, &LogViewerDialog::refreshLogs);
    connect(m_openDirectoryButton, &QPushButton::clicked, this, &LogViewerDialog::openLogDirectory);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void LogViewerDialog::applyLanguage()
{
    setWindowTitle(text(QStringLiteral("Application Logs"), QStringLiteral("应用日志")));
    m_titleLabel->setText(text(QStringLiteral("Application Logs"), QStringLiteral("应用日志")));
    m_pathLabel->setText(text(QStringLiteral("Log file: %1"), QStringLiteral("日志文件：%1")).arg(m_logFilePath));
    m_refreshButton->setText(text(QStringLiteral("Refresh"), QStringLiteral("刷新")));
    m_openDirectoryButton->setText(text(QStringLiteral("Open Folder"), QStringLiteral("打开目录")));
    m_closeButton->setText(text(QStringLiteral("Close"), QStringLiteral("关闭")));
}

void LogViewerDialog::refreshLogs()
{
    QString error;
    const QString content = LogFileReader::readLastLines(m_logFilePath, 500, &error);
    if (!error.isEmpty()) {
        m_logTextEdit->setPlainText(text(QStringLiteral("Log file is not available.\n\n%1"),
                                         QStringLiteral("日志文件不可用。\n\n%1"))
                                        .arg(error));
        return;
    }

    m_logTextEdit->setPlainText(content.isEmpty()
                                    ? text(QStringLiteral("No log entries yet."), QStringLiteral("暂无日志记录。"))
                                    : content);
    m_logTextEdit->moveCursor(QTextCursor::End);
}

void LogViewerDialog::openLogDirectory()
{
    const QString directoryPath = QFileInfo(m_logFilePath).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(directoryPath));
}

QString LogViewerDialog::text(const QString &english, const QString &chinese) const
{
    return m_language == AppLanguage::English ? english : chinese;
}
