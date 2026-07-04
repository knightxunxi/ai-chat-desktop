#include "ui/CommandPalette.h"

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

CommandPalette::CommandPalette(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("commandPalette"));
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setFixedSize(520, 360);
    setAttribute(Qt::WA_ShowWithoutActivating, false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_searchInput = new QLineEdit(this);
    m_searchInput->setObjectName(QStringLiteral("commandPaletteInput"));
    m_searchInput->setPlaceholderText(QStringLiteral("Type a command..."));
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->installEventFilter(this);
    layout->addWidget(m_searchInput);

    m_resultList = new QListWidget(this);
    m_resultList->setObjectName(QStringLiteral("commandPaletteList"));
    layout->addWidget(m_resultList);

    connect(m_searchInput, &QLineEdit::textChanged, this, &CommandPalette::filterItems);
    connect(m_resultList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!item) return;
        const QString id = item->data(Qt::UserRole).toString();
        const QVariant data = item->data(Qt::UserRole + 1);
        hidePalette();
        emit commandSelected(id, data);
    });
}

void CommandPalette::setCommands(const QVector<Command> &commands)
{
    m_commands = commands;
    m_searchInput->clear();
    filterItems(QString());
}

void CommandPalette::showAtCenter()
{
    if (auto *p = parentWidget()) {
        QPoint center = p->mapToGlobal(p->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
    m_searchInput->clear();
    filterItems(QString());
    setVisible(true);
    raise();
    m_searchInput->setFocus();
}

void CommandPalette::hidePalette()
{
    setVisible(false);
    if (auto *p = parentWidget()) {
        p->setFocus();
    }
}

bool CommandPalette::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_searchInput && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            hidePalette();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Tab) {
            int next = m_resultList->currentRow() + 1;
            if (next < m_resultList->count()) m_resultList->setCurrentRow(next);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Up || (keyEvent->key() == Qt::Key_Tab && keyEvent->modifiers() & Qt::ShiftModifier)) {
            int prev = m_resultList->currentRow() - 1;
            if (prev >= 0) m_resultList->setCurrentRow(prev);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            auto *item = m_resultList->currentItem();
            if (item) {
                const QString id = item->data(Qt::UserRole).toString();
                const QVariant data = item->data(Qt::UserRole + 1);
                hidePalette();
                emit commandSelected(id, data);
            }
            return true;
        }
    }
    return QFrame::eventFilter(obj, event);
}

void CommandPalette::filterItems(const QString &text)
{
    m_resultList->clear();
    const QString lower = text.toLower();

    for (const auto &cmd : m_commands) {
        // 模糊匹配：label 或 id 包含输入文字
        if (!lower.isEmpty() &&
            !cmd.label.toLower().contains(lower) &&
            !cmd.id.toLower().contains(lower)) {
            continue;
        }

        QString display = cmd.label;
        if (!cmd.shortcut.isEmpty()) {
            display += QStringLiteral("  [%1]").arg(cmd.shortcut);
        }

        auto *item = new QListWidgetItem(display, m_resultList);
        item->setData(Qt::UserRole, cmd.id);
        item->setData(Qt::UserRole + 1, cmd.data);
    }

    if (m_resultList->count() > 0) {
        m_resultList->setCurrentRow(0);
    }
}
