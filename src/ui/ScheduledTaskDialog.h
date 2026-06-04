#pragma once

#include <QDialog>

class ApplicationController;
class QDialogButtonBox;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTextEdit;

class ScheduledTaskDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScheduledTaskDialog(ApplicationController *controller, QWidget *parent = nullptr);

private:
    void setupUi();
    void loadTasks();
    void onAdd();
    void onEdit();
    void onRemove();
    void onToggle(int row, int column);

    ApplicationController *m_controller = nullptr;
    QTableWidget *m_table = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;

    static constexpr int kColumnName = 0;
    static constexpr int kColumnCron = 1;
    static constexpr int kColumnPrompt = 2;
    static constexpr int kColumnEnabled = 3;
    static constexpr int kColumnLastRun = 4;
    static constexpr int kColumnNextRun = 5;
};
