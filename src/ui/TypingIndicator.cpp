#include "ui/TypingIndicator.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

TypingIndicator::TypingIndicator(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("typingIndicator"));
    setFixedHeight(24);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 0, 0, 0);
    layout->setSpacing(0);

    m_dotLabel = new QLabel(QStringLiteral("\u00B7"), this); // middle dot
    m_dotLabel->setObjectName(QStringLiteral("typingDots"));

    layout->addWidget(m_dotLabel);
    layout->addStretch();

    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(400);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        m_dotCount = (m_dotCount + 1) % 4;
        if (m_dotCount == 0) {
            m_dotLabel->setText(QStringLiteral("\u00B7"));           // ·
        } else if (m_dotCount == 1) {
            m_dotLabel->setText(QStringLiteral("\u00B7\u00B7"));    // ··
        } else if (m_dotCount == 2) {
            m_dotLabel->setText(QStringLiteral("\u00B7\u00B7\u00B7")); // ···
        } else {
            m_dotLabel->setText(QString()); // 短暂空白
        }
    });

    setVisible(false);
}

void TypingIndicator::startAnimation()
{
    m_dotCount = 0;
    m_dotLabel->setText(QStringLiteral("\u00B7"));
    setVisible(true);
    m_animTimer->start();
}

void TypingIndicator::stopAnimation()
{
    m_animTimer->stop();
    setVisible(false);
}
