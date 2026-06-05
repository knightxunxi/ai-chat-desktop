#include "scheduler/TaskScheduler.h"

#include <QTimeZone>
#include <QUuid>

#include <algorithm>

ScheduledTask ScheduledTask::create(const QString &name, const QString &cron,
                                     const QString &prompt)
{
    ScheduledTask task;
    task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.name = name;
    task.cronExpression = cron.trimmed();
    task.agentPrompt = prompt;
    return task;
}

bool ScheduledTask::isValid() const
{
    if (name.trimmed().isEmpty())
        return false;
    if (cronExpression.trimmed().isEmpty())
        return false;
    QStringList parts = cronExpression.trimmed().split(QChar(' '));
    if (parts.size() != 5)
        return false;
    return true;
}

// ---------- Cron 字段匹配 ----------

namespace {

bool fieldMatches(const QString &field, int value, int minVal, int maxVal)
{
    if (field == QStringLiteral("*"))
        return true;

    for (const QString &part : field.split(QChar(','))) {
        QString stepToken = part;

        // 步进值 /step
        int step = 1;
        int slashIdx = part.indexOf(QChar('/'));
        if (slashIdx != -1) {
            stepToken = part.left(slashIdx);
            bool ok = false;
            int parsed = QStringView(part).mid(slashIdx + 1).toInt(&ok);
            if (ok && parsed > 0)
                step = parsed;
            else
                return false;
        }

        // 范围 a-b
        int rangeStart = minVal;
        int rangeEnd = maxVal;
        int hyphenIdx = stepToken.indexOf(QChar('-'));
        if (hyphenIdx != -1) {
            bool ok = false;
            int lo = QStringView(stepToken).left(hyphenIdx).toInt(&ok);
            if (!ok || lo < minVal || lo > maxVal)
                return false;
            int hi = QStringView(stepToken).mid(hyphenIdx + 1).toInt(&ok);
            if (!ok || hi < minVal || hi > maxVal)
                return false;
            rangeStart = lo;
            rangeEnd = hi;
        } else if (stepToken == QStringLiteral("*")) {
            rangeStart = minVal;
            rangeEnd = maxVal;
        } else {
            bool ok = false;
            int single = stepToken.toInt(&ok);
            if (!ok || single < minVal || single > maxVal)
                return false;
            rangeStart = single;
            rangeEnd = single;
        }

        // 检查步进
        for (int v = rangeStart; v <= rangeEnd; v += step) {
            if (v == value)
                return true;
        }
    }

    return false;
}

int maxDaysInMonth(int month, int year)
{
    static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return leap ? 29 : 28;
    }
    return days[month];
}

QDateTime utcDate(int year, int month, int day, int hour, int minute)
{
    return QDateTime(QDate(year, month, day), QTime(hour, minute, 0), QTimeZone::UTC);
}

QDateTime utcDateOnly(QDate date, int hour, int minute)
{
    return QDateTime(date, QTime(hour, minute, 0), QTimeZone::UTC);
}

QDateTime minuteStart(const QDateTime &dateTime)
{
    return QDateTime(dateTime.date(),
        QTime(dateTime.time().hour(), dateTime.time().minute(), 0),
        QTimeZone::UTC);
}

bool sameMinute(const QDateTime &left, const QDateTime &right)
{
    return left.isValid() && right.isValid() && minuteStart(left) == minuteStart(right);
}

bool cronMatchesDateTime(const QString &cron, const QDateTime &dateTime)
{
    if (!dateTime.isValid())
        return false;

    const QStringList parts = cron.trimmed().split(QChar(' '));
    if (parts.size() != 5)
        return false;

    const QString &minField = parts[0];
    const QString &hourField = parts[1];
    const QString &dayField = parts[2];
    const QString &monthField = parts[3];
    const QString &weekField = parts[4];

    const int minute = dateTime.time().minute();
    const int hour = dateTime.time().hour();
    const int day = dateTime.date().day();
    const int month = dateTime.date().month();
    const int year = dateTime.date().year();
    const int dayOfWeek = dateTime.date().dayOfWeek() % 7;

    if (!fieldMatches(minField, minute, 0, 59))
        return false;
    if (!fieldMatches(hourField, hour, 0, 23))
        return false;
    if (!fieldMatches(monthField, month, 1, 12))
        return false;

    const bool dayOk = fieldMatches(dayField, day, 1, maxDaysInMonth(month, year));
    const bool weekOk = fieldMatches(weekField, dayOfWeek, 0, 6);

    if (dayField != QStringLiteral("*") && weekField != QStringLiteral("*"))
        return dayOk || weekOk;

    return dayOk && weekOk;
}

} // namespace

// ---------- TaskScheduler ----------

TaskScheduler::TaskScheduler(QObject *parent)
    : QObject(parent)
{
}

void TaskScheduler::addTask(const ScheduledTask &task)
{
    m_tasks.append(task);
}

void TaskScheduler::removeTask(const QString &taskId)
{
    m_tasks.erase(
        std::remove_if(m_tasks.begin(), m_tasks.end(),
                        [&](const ScheduledTask &t) { return t.id == taskId; }),
        m_tasks.end());
}

void TaskScheduler::updateTask(const ScheduledTask &task)
{
    for (auto &t : m_tasks) {
        if (t.id == task.id) {
            t = task;
            return;
        }
    }
}

QVector<ScheduledTask> TaskScheduler::allTasks() const
{
    return m_tasks;
}

void TaskScheduler::start()
{
    if (m_timer)
        return;

    // 立即 tick 一次，对齐分钟边界
    tick();

    m_timer = new QTimer(this);
    m_timer->setInterval(60000); // 60 秒
    connect(m_timer, &QTimer::timeout, this, &TaskScheduler::tick);
    m_timer->start();
}

void TaskScheduler::stop()
{
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}

bool TaskScheduler::isRunning() const
{
    return m_timer != nullptr && m_timer->isActive();
}

void TaskScheduler::tick()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();

    for (auto &task : m_tasks) {
        if (!task.enabled)
            continue;
        if (!task.isValid())
            continue;

        // UI 展示下一次未来运行时间；触发判断单独基于当前分钟匹配。
        const QDateTime next = nextRunTime(task.cronExpression, now);
        if (!next.isValid())
            continue;

        task.nextRun = next;

        if (!cronMatchesDateTime(task.cronExpression, now))
            continue;

        if (sameMinute(task.lastRun, now))
            continue;

        task.lastRun = now;
        emit taskTriggered(task);
    }
}

QDateTime TaskScheduler::nextRunTime(const QString &cron, const QDateTime &from)
{
    if (!from.isValid())
        return QDateTime();

    const QString trimmed = cron.trimmed();
    const QStringList parts = trimmed.split(QChar(' '));
    if (parts.size() != 5)
        return QDateTime();

    const QString &minField = parts[0];
    const QString &hourField = parts[1];
    const QString &dayField = parts[2];
    const QString &monthField = parts[3];
    const QString &weekField = parts[4];

    // 从 from 的下一个分钟开始搜索
    QDateTime cursor = from.addSecs(60);
    cursor = QDateTime(cursor.date(),
                        QTime(cursor.time().hour(), cursor.time().minute(), 0),
                        QTimeZone::UTC);

    // 搜索上限：未来 5 年
    const QDateTime deadline = from.addYears(5);
    const int maxIterations = 5 * 366 * 24 * 60;

    for (int i = 0; i < maxIterations && cursor <= deadline; ++i) {
        int minute = cursor.time().minute();
        int hour = cursor.time().hour();
        int day = cursor.date().day();
        int month = cursor.date().month();
        int year = cursor.date().year();
        int dayOfWeek = cursor.date().dayOfWeek() % 7; // Qt: 1=Monday → 转为 0=Sunday

        if (!fieldMatches(minField, minute, 0, 59)) {
            cursor = cursor.addSecs(60);
            continue;
        }
        if (!fieldMatches(hourField, hour, 0, 23)) {
            cursor = utcDateOnly(cursor.date(), (hour + 1) % 24, 0);
            if (hour == 23)
                cursor = cursor.addDays(1);
            continue;
        }
        if (!fieldMatches(monthField, month, 1, 12)) {
            const QDate nextMonth = QDate(year, month, 1).addMonths(1);
            cursor = utcDate(nextMonth.year(), nextMonth.month(), 1, 0, 0);
            continue;
        }

        bool dayOk = fieldMatches(dayField, day, 1, maxDaysInMonth(month, year));
        bool weekOk = fieldMatches(weekField, dayOfWeek, 0, 6);

        // Cron 标准：day 和 week 均为非 * 时为 OR 关系
        bool dayMatch = false;
        if (dayField != QStringLiteral("*") && weekField != QStringLiteral("*"))
            dayMatch = dayOk || weekOk;
        else
            dayMatch = dayOk && weekOk;

        if (!dayMatch) {
            cursor = cursor.addDays(1);
            cursor.setTime(QTime(0, 0, 0));
            continue;
        }

        return cursor;
    }

    return QDateTime();
}
