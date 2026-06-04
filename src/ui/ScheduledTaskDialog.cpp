#include "ui/ScheduledTaskDialog.h"

#include "app/ApplicationController.h"
#include "scheduler/ScheduledTask.h"

#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

bool validateCronExpression(const QString &cron, QString *errorOut = nullptr)
{
    const QStringList fields = cron.trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (fields.size() != 5) {
        if (errorOut)
            *errorOut = QStringLiteral("Cron 表达式需要 5 个字段，当前有 %1 个").arg(fields.size());
        return false;
    }

    const int maxValues[5] = {59, 23, 31, 12, 7};
    const QString fieldNames[5] = {
        QStringLiteral("分钟(0-59)"), QStringLiteral("小时(0-23)"),
        QStringLiteral("日(1-31)"), QStringLiteral("月(1-12)"), QStringLiteral("星期(0-7)")
    };

    for (int i = 0; i < 5; ++i) {
        const QString &f = fields[i];
        if (f == QLatin1String("*")) {
            continue;
        }
        // Handle lists like 1,2,3 or ranges like 1-5 or steps like */5
        const QStringList parts = f.split(QLatin1Char(','));
        for (const QString &part : parts) {
            QString val = part.trimmed();
            // Handle step: */N or 1/5
            QString stepPart;
            int slashIdx = val.indexOf(QLatin1Char('/'));
            if (slashIdx >= 0) {
                stepPart = val.mid(slashIdx + 1);
                val = val.left(slashIdx);
            }
            if (val.isEmpty() && !stepPart.isEmpty()) {
                // */5 case
                bool ok = false;
                stepPart.toInt(&ok);
                if (!ok || stepPart.toInt() < 1) {
                    if (errorOut)
                        *errorOut = QStringLiteral("第 %1 个字段(%2)步长无效: %3").arg(i + 1).arg(fieldNames[i]).arg(stepPart);
                    return false;
                }
                continue;
            }
            // Handle range: 1-5
            QString rangeEnd;
            int dashIdx = val.indexOf(QLatin1Char('-'));
            if (dashIdx >= 0) {
                rangeEnd = val.mid(dashIdx + 1);
                val = val.left(dashIdx);
            }
            if (val.isEmpty() && !rangeEnd.isEmpty()) {
                if (errorOut)
                    *errorOut = QStringLiteral("第 %1 个字段(%2)范围起始值缺失").arg(i + 1).arg(fieldNames[i]);
                return false;
            }
            bool ok = false;
            int num = val.toInt(&ok);
            if (!ok || num < 0 || num > maxValues[i]) {
                if (errorOut)
                    *errorOut = QStringLiteral("第 %1 个字段(%2)数值无效: %3").arg(i + 1).arg(fieldNames[i]).arg(val);
                return false;
            }
            if (!rangeEnd.isEmpty()) {
                int endNum = rangeEnd.toInt(&ok);
                if (!ok || endNum < 0 || endNum > maxValues[i] || endNum < num) {
                    if (errorOut)
                        *errorOut = QStringLiteral("第 %1 个字段(%2)范围无效: %3-%4").arg(i + 1).arg(fieldNames[i]).arg(val).arg(rangeEnd);
                    return false;
                }
            }
        }
    }

    return true;
}

QString formatDateTime(const QDateTime &dt)
{
    if (!dt.isValid())
        return QString();
    return dt.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

} // namespace

ScheduledTaskDialog::ScheduledTaskDialog(ApplicationController *controller, QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
{
    setupUi();
    loadTasks();
}

void ScheduledTaskDialog::setupUi()
{
    setWindowTitle(QStringLiteral("\u8C03\u5EA6\u4EFB\u52A1")); // 调度任务
    setMinimumSize(800, 480);
    setModal(true);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 12);
    rootLayout->setSpacing(12);

    // -- Button row --
    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);

    m_addButton = new QPushButton(QStringLiteral("\u6DFB\u52A0"), this); // 添加
    m_editButton = new QPushButton(QStringLiteral("\u7F16\u8F91"), this); // 编辑
    m_removeButton = new QPushButton(QStringLiteral("\u5220\u9664"), this); // 删除
    m_refreshButton = new QPushButton(QStringLiteral("\u5237\u65B0"), this); // 刷新

    buttonRow->addWidget(m_addButton);
    buttonRow->addWidget(m_editButton);
    buttonRow->addWidget(m_removeButton);
    buttonRow->addWidget(m_refreshButton);
    buttonRow->addStretch();

    rootLayout->addLayout(buttonRow);

    // -- Table --
    m_table = new QTableWidget(0, 6, this);
    m_table->setObjectName(QStringLiteral("scheduledTaskTable"));
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("\u540D\u79F0"),    // 名称
        QStringLiteral("Cron\u8868\u8FBE\u5F0F"), // Cron表达式
        QStringLiteral("Agent\u63D0\u793A\u8BCD"), // Agent提示词
        QStringLiteral("\u542F\u7528"),    // 启用
        QStringLiteral("\u4E0A\u6B21\u6267\u884C"), // 上次执行
        QStringLiteral("\u4E0B\u6B21\u6267\u884C"), // 下次执行
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(kColumnPrompt, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);

    rootLayout->addWidget(m_table, 1);

    // -- Bottom button box --
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &ScheduledTaskDialog::reject);
    rootLayout->addWidget(m_buttonBox);

    // -- Connections --
    connect(m_addButton, &QPushButton::clicked, this, &ScheduledTaskDialog::onAdd);
    connect(m_editButton, &QPushButton::clicked, this, &ScheduledTaskDialog::onEdit);
    connect(m_removeButton, &QPushButton::clicked, this, &ScheduledTaskDialog::onRemove);
    connect(m_refreshButton, &QPushButton::clicked, this, &ScheduledTaskDialog::loadTasks);
    connect(m_table, &QTableWidget::cellClicked, this, &ScheduledTaskDialog::onToggle);
}

void ScheduledTaskDialog::loadTasks()
{
    m_table->setRowCount(0);

    if (!m_controller)
        return;

    const QVector<ScheduledTask> tasks = m_controller->scheduledTasks();
    for (const ScheduledTask &task : tasks) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        auto *nameItem = new QTableWidgetItem(task.name);
        nameItem->setData(Qt::UserRole, task.id);
        m_table->setItem(row, kColumnName, nameItem);

        m_table->setItem(row, kColumnCron, new QTableWidgetItem(task.cronExpression));

        auto *promptItem = new QTableWidgetItem(task.agentPrompt);
        promptItem->setToolTip(task.agentPrompt);
        m_table->setItem(row, kColumnPrompt, promptItem);

        auto *enabledItem = new QTableWidgetItem();
        enabledItem->setFlags(enabledItem->flags() | Qt::ItemIsUserCheckable);
        enabledItem->setCheckState(task.enabled ? Qt::Checked : Qt::Unchecked);
        m_table->setItem(row, kColumnEnabled, enabledItem);

        m_table->setItem(row, kColumnLastRun, new QTableWidgetItem(formatDateTime(task.lastRun)));
        m_table->setItem(row, kColumnNextRun, new QTableWidgetItem(formatDateTime(task.nextRun)));
    }
}

void ScheduledTaskDialog::onAdd()
{
    QDialog form(this);
    form.setWindowTitle(QStringLiteral("\u6DFB\u52A0\u4EFB\u52A1")); // 添加任务
    form.setMinimumWidth(480);

    auto *layout = new QVBoxLayout(&form);
    auto *fLayout = new QFormLayout();
    fLayout->setSpacing(10);

    auto *nameEdit = new QLineEdit(&form);
    auto *cronEdit = new QLineEdit(&form);
    cronEdit->setPlaceholderText(QStringLiteral("0 9 * * 1")); // 每天9点
    auto *promptEdit = new QTextEdit(&form);
    promptEdit->setMinimumHeight(100);

    fLayout->addRow(QStringLiteral("\u540D\u79F0:"), nameEdit);         // 名称
    fLayout->addRow(QStringLiteral("Cron\u8868\u8FBE\u5F0F:"), cronEdit); // Cron表达式
    fLayout->addRow(QStringLiteral("Agent\u63D0\u793A\u8BCD:"), promptEdit); // Agent提示词

    layout->addLayout(fLayout);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &form);
    connect(btnBox, &QDialogButtonBox::accepted, &form, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &form, &QDialog::reject);
    layout->addWidget(btnBox);

    if (form.exec() != QDialog::Accepted)
        return;

    const QString name = nameEdit->text().trimmed();
    const QString cron = cronEdit->text().trimmed();
    const QString prompt = promptEdit->toPlainText().trimmed();

    if (name.isEmpty() || cron.isEmpty() || prompt.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("\u586B\u5199\u4E0D\u5B8C\u6574"), // 填写不完整
                             QStringLiteral("\u540D\u79F0\u3001Cron \u8868\u8FBE\u5F0F\u548C Agent \u63D0\u793A\u8BCD\u90FD\u4E0D\u80FD\u4E3A\u7A7A\u3002"));
        return;
    }

    QString error;
    if (!validateCronExpression(cron, &error)) {
        QMessageBox::warning(this, QStringLiteral("Cron \u683C\u5F0F\u9519\u8BEF"), error);
        return;
    }

    m_controller->addScheduledTask(name, cron, prompt);
    loadTasks();
}

void ScheduledTaskDialog::onEdit()
{
    const int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this,
                                 QStringLiteral("\u672A\u9009\u4E2D"), // 未选中
                                 QStringLiteral("\u8BF7\u5148\u9009\u4E2D\u4E00\u6761\u4EFB\u52A1\u3002")); // 请先选中一条任务
        return;
    }

    QTableWidgetItem *nameItem = m_table->item(row, kColumnName);
    if (!nameItem)
        return;

    const QString taskId = nameItem->data(Qt::UserRole).toString();

    QDialog form(this);
    form.setWindowTitle(QStringLiteral("\u7F16\u8F91\u4EFB\u52A1")); // 编辑任务
    form.setMinimumWidth(480);

    auto *layout = new QVBoxLayout(&form);
    auto *fLayout = new QFormLayout();
    fLayout->setSpacing(10);

    auto *nameEdit = new QLineEdit(&form);
    nameEdit->setText(nameItem->text());

    auto *cronEdit = new QLineEdit(&form);
    QTableWidgetItem *cronItem = m_table->item(row, kColumnCron);
    if (cronItem)
        cronEdit->setText(cronItem->text());

    auto *promptEdit = new QTextEdit(&form);
    QTableWidgetItem *promptItem = m_table->item(row, kColumnPrompt);
    if (promptItem)
        promptEdit->setPlainText(promptItem->text());
    promptEdit->setMinimumHeight(100);

    fLayout->addRow(QStringLiteral("\u540D\u79F0:"), nameEdit);
    fLayout->addRow(QStringLiteral("Cron\u8868\u8FBE\u5F0F:"), cronEdit);
    fLayout->addRow(QStringLiteral("Agent\u63D0\u793A\u8BCD:"), promptEdit);

    layout->addLayout(fLayout);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &form);
    connect(btnBox, &QDialogButtonBox::accepted, &form, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &form, &QDialog::reject);
    layout->addWidget(btnBox);

    if (form.exec() != QDialog::Accepted)
        return;

    const QString name = nameEdit->text().trimmed();
    const QString cron = cronEdit->text().trimmed();
    const QString prompt = promptEdit->toPlainText().trimmed();

    if (name.isEmpty() || cron.isEmpty() || prompt.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("\u586B\u5199\u4E0D\u5B8C\u6574"),
                             QStringLiteral("\u540D\u79F0\u3001Cron \u8868\u8FBE\u5F0F\u548C Agent \u63D0\u793A\u8BCD\u90FD\u4E0D\u80FD\u4E3A\u7A7A\u3002"));
        return;
    }

    QString error;
    if (!validateCronExpression(cron, &error)) {
        QMessageBox::warning(this, QStringLiteral("Cron \u683C\u5F0F\u9519\u8BEF"), error);
        return;
    }

    // Build updated task
    ScheduledTask updated;
    updated.id = taskId;
    updated.name = name;
    updated.cronExpression = cron;
    updated.agentPrompt = prompt;
    // Preserve enabled state
    QTableWidgetItem *enabledItem = m_table->item(row, kColumnEnabled);
    if (enabledItem)
        updated.enabled = (enabledItem->checkState() == Qt::Checked);

    m_controller->updateScheduledTask(updated);
    loadTasks();
}

void ScheduledTaskDialog::onRemove()
{
    const int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this,
                                 QStringLiteral("\u672A\u9009\u4E2D"),
                                 QStringLiteral("\u8BF7\u5148\u9009\u4E2D\u4E00\u6761\u4EFB\u52A1\u3002"));
        return;
    }

    QTableWidgetItem *nameItem = m_table->item(row, kColumnName);
    if (!nameItem)
        return;

    const QString taskId = nameItem->data(Qt::UserRole).toString();

    int result = QMessageBox::question(this,
                                       QStringLiteral("\u786E\u8BA4\u5220\u9664"), // 确认删除
                                       QStringLiteral("\u786E\u5B9A\u8981\u5220\u9664\u4EFB\u52A1 \u201C%1\u201D \u5417\uFF1F").arg(nameItem->text()),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);

    if (result != QMessageBox::Yes)
        return;

    m_controller->removeScheduledTask(taskId);
    loadTasks();
}

void ScheduledTaskDialog::onToggle(int row, int column)
{
    if (column != kColumnEnabled)
        return;

    QTableWidgetItem *nameItem = m_table->item(row, kColumnName);
    QTableWidgetItem *enabledItem = m_table->item(row, kColumnEnabled);
    if (!nameItem || !enabledItem)
        return;

    const QString taskId = nameItem->data(Qt::UserRole).toString();
    const bool currentlyEnabled = (enabledItem->checkState() == Qt::Checked);

    // Build task with toggled state
    ScheduledTask updated;
    updated.id = taskId;
    updated.name = nameItem->text();
    QTableWidgetItem *cronItem = m_table->item(row, kColumnCron);
    if (cronItem)
        updated.cronExpression = cronItem->text();
    QTableWidgetItem *promptItem = m_table->item(row, kColumnPrompt);
    if (promptItem)
        updated.agentPrompt = promptItem->text();
    updated.enabled = !currentlyEnabled;

    m_controller->updateScheduledTask(updated);

    // Update the checkbox in the table (it already toggled visually due to click)
    // Just reload to be safe
    loadTasks();
}
