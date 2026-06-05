#pragma once

#include <QDialog>
#include <QString>

class QLabel;

// AG-6: 工具确认弹窗数据结构
struct ConfirmToolInfo {
    QString toolName;       // 工具名称
    QString description;    // 工具描述
    QString action;         // 将要执行的操作摘要
    QString parameters;     // JSON 参数格式化展示
    int riskLevel = 0;      // 0=低, 1=中, 2=高
};

// AG-6: 高危操作确认弹窗 — 展示风险颜色、操作预览、参数详情。
class ConfirmToolDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfirmToolDialog(const ConfirmToolInfo &info, QWidget *parent = nullptr);

    // 功能：便捷静态方法，低风险直接返回 true，中/高风险弹出确认；使用模块：ApplicationController 工具执行前。
    static bool confirm(QWidget *parent, const ConfirmToolInfo &info);

private:
    void setupUi(const ConfirmToolInfo &info);
    QString riskLabel(int level) const;
    QString riskColor(int level) const;
};
