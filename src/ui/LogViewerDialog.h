#pragma once

#include "core/AppLanguage.h"

#include <QDialog>

class QLabel;
class QPushButton;
class QPlainTextEdit;

class LogViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogViewerDialog(const QString &logFilePath, AppLanguage language, QWidget *parent = nullptr);

private:
    void setupUi();
    void applyLanguage();
    void refreshLogs();
    void openLogDirectory();
    QString text(const QString &english, const QString &chinese) const;

    QString m_logFilePath;
    AppLanguage m_language = AppLanguage::Chinese;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_pathLabel = nullptr;
    QPlainTextEdit *m_logTextEdit = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_openDirectoryButton = nullptr;
    QPushButton *m_closeButton = nullptr;
};
