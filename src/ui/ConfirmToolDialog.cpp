#include "ui/ConfirmToolDialog.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ConfirmToolDialog::ConfirmToolDialog(const ConfirmToolInfo &info, QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("confirmToolDialog"));
    setWindowTitle(tr("Confirm Action", "确认执行操作"));
    setMinimumWidth(420);
    setMinimumHeight(300);
    setupUi(info);
}

bool ConfirmToolDialog::confirm(QWidget *parent, const ConfirmToolInfo &info)
{
    // 低风险直接放行
    if (info.riskLevel == 0) {
        return true;
    }

    ConfirmToolDialog dialog(info, parent);
    return dialog.exec() == QDialog::Accepted;
}

QString ConfirmToolDialog::riskLabel(int level) const
{
    switch (level) {
    case 0:
        return tr("Low Risk", "低风险");
    case 1:
        return tr("Medium Risk", "中风险");
    case 2:
        return tr("High Risk", "高风险");
    default:
        return tr("Unknown", "未知");
    }
}

QString ConfirmToolDialog::riskColor(int level) const
{
    switch (level) {
    case 0:
        return QStringLiteral("#047857");  // 绿色
    case 1:
        return QStringLiteral("#d97706");  // 橙色
    case 2:
        return QStringLiteral("#dc2626");  // 红色
    default:
        return QStringLiteral("#6b7280");
    }
}

void ConfirmToolDialog::setupUi(const ConfirmToolInfo &info)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    // 风险等级标签
    auto *riskFrame = new QFrame(this);
    riskFrame->setObjectName(QStringLiteral("confirmRiskBadge"));
    riskFrame->setStyleSheet(
        QStringLiteral("background: %1; border-radius: 4px; padding: 2px 10px;")
            .arg(riskColor(info.riskLevel)));
    auto *riskLayout = new QHBoxLayout(riskFrame);
    riskLayout->setContentsMargins(8, 4, 8, 4);
    auto *riskLabelWidget = new QLabel(riskLabel(info.riskLevel), riskFrame);
    riskLabelWidget->setStyleSheet(QStringLiteral("color: #ffffff; font-weight: 600; font-size: 13px;"));
    riskLayout->addWidget(riskLabelWidget);

    // 工具名 + 描述
    auto *titleLabel = new QLabel(info.toolName, this);
    titleLabel->setObjectName(QStringLiteral("confirmToolTitle"));
    auto *descLabel = new QLabel(info.description, this);
    descLabel->setObjectName(QStringLiteral("confirmToolDesc"));
    descLabel->setWordWrap(true);

    // 操作预览
    auto *actionFrame = new QFrame(this);
    actionFrame->setObjectName(QStringLiteral("confirmActionPreview"));
    auto *actionLayout = new QVBoxLayout(actionFrame);
    actionLayout->setContentsMargins(10, 8, 10, 8);

    auto *actionTitle = new QLabel(tr("Action Preview", "操作预览"), actionFrame);
    actionTitle->setObjectName(QStringLiteral("confirmSectionTitle"));
    auto *actionContent = new QLabel(info.action, actionFrame);
    actionContent->setObjectName(QStringLiteral("confirmActionBody"));
    actionContent->setWordWrap(true);
    actionContent->setTextInteractionFlags(Qt::TextSelectableByMouse);

    actionLayout->addWidget(actionTitle);
    actionLayout->addWidget(actionContent);

    // 参数详情（可折叠，默认折叠）
    auto *paramFrame = new QFrame(this);
    paramFrame->setObjectName(QStringLiteral("confirmParamPreview"));
    auto *paramLayout = new QVBoxLayout(paramFrame);
    paramLayout->setContentsMargins(10, 8, 10, 8);

    auto *paramToggle = new QPushButton(QStringLiteral("\u25BC ") + tr("Parameter Details", "参数详情"), paramFrame);
    paramToggle->setObjectName(QStringLiteral("confirmParamToggle"));
    paramToggle->setFlat(true);
    paramToggle->setCursor(Qt::PointingHandCursor);

    auto *paramBody = new QLabel(info.parameters, paramFrame);
    paramBody->setObjectName(QStringLiteral("confirmParamBody"));
    paramBody->setWordWrap(true);
    paramBody->setTextFormat(Qt::PlainText);
    paramBody->setTextInteractionFlags(Qt::TextSelectableByMouse);
    paramBody->setVisible(false);

    connect(paramToggle, &QPushButton::clicked, this, [paramToggle, paramBody]() {
        bool visible = paramBody->isVisible();
        paramBody->setVisible(!visible);
        paramToggle->setText((visible ? QStringLiteral("\u25B6 ") : QStringLiteral("\u25BC "))
                             + tr("Parameter Details", "参数详情"));
    });

    paramLayout->addWidget(paramToggle);
    paramLayout->addWidget(paramBody);

    // 按钮
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();

    auto *cancelBtn = new QPushButton(tr("Cancel", "取消"), this);
    cancelBtn->setObjectName(QStringLiteral("confirmCancelBtn"));
    auto *confirmBtn = new QPushButton(tr("Confirm", "确认执行"), this);
    confirmBtn->setObjectName(QStringLiteral("confirmExecuteBtn"));

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(confirmBtn);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(confirmBtn, &QPushButton::clicked, this, &QDialog::accept);

    layout->addWidget(riskFrame);
    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);
    layout->addWidget(actionFrame);
    layout->addWidget(paramFrame);
    layout->addStretch();
    layout->addLayout(btnLayout);
}
