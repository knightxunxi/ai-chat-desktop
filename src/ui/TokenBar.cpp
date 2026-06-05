#include "ui/TokenBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

TokenBar::TokenBar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("tokenBar"));
    setFixedHeight(28);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 2, 24, 2);
    layout->setSpacing(10);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName(QStringLiteral("tokenProgress"));
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(10);

    m_countLabel = new QLabel(QStringLiteral("0 / 0 tokens"), this);
    m_countLabel->setObjectName(QStringLiteral("tokenCountLabel"));

    layout->addWidget(m_progressBar, 1);
    layout->addWidget(m_countLabel);
}

void TokenBar::updateTokens(int used, int limit)
{
    if (limit <= 0) {
        m_progressBar->setValue(0);
        m_countLabel->setText(QString::number(used));
        return;
    }

    int percentage = qMin(100, static_cast<int>((static_cast<long long>(used) * 100) / limit));
    m_progressBar->setValue(percentage);

    // 根据用量切换进度条颜色
    QString barColor;
    if (percentage >= 90) {
        barColor = QStringLiteral("#dc2626"); // 红色：即将超限
    } else if (percentage >= 70) {
        barColor = QStringLiteral("#f59e0b"); // 橙色：注意
    } else {
        barColor = QStringLiteral("#10b981"); // 绿色：正常
    }

    m_progressBar->setStyleSheet(
        QStringLiteral("QProgressBar#tokenProgress {"
                       "  background: #e5e7eb; border: none; border-radius: 5px;"
                       "}"
                       "QProgressBar#tokenProgress::chunk {"
                       "  background: %1; border-radius: 5px;"
                       "}")
            .arg(barColor));

    // 格式化数字：1,234
    auto formatNumber = [](int n) -> QString {
        QString s = QString::number(n);
        for (int i = s.length() - 3; i > 0; i -= 3) {
            s.insert(i, QLatin1Char(','));
        }
        return s;
    };

    m_countLabel->setText(QStringLiteral("%1 / %2 tokens")
                              .arg(formatNumber(used), formatNumber(limit)));
}
