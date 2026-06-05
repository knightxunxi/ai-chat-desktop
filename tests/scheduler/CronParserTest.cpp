#include "scheduler/ScheduledTask.h"
#include "scheduler/TaskScheduler.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <QStringView>
#include <QTimeZone>

#include <cassert>

namespace {

QDateTime nextRunTime(const QString &cron, const QDateTime &from)
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

    QDateTime cursor = from.addSecs(60);
    cursor = QDateTime(cursor.date(),
                        QTime(cursor.time().hour(), cursor.time().minute(), 0),
                        QTimeZone::UTC);

    const QDateTime deadline = from.addYears(5);
    const int maxIterations = 5 * 366 * 24 * 60;

    auto fieldMatches = [](const QString &field, int value, int minVal, int maxVal) -> bool {
        if (field == QStringLiteral("*"))
            return true;
        for (const QString &part : field.split(QChar(','))) {
            QString stepToken = part;
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
            int rangeStart = minVal;
            int rangeEnd = maxVal;
            int hyphenIdx = stepToken.indexOf(QChar('-'));
            if (hyphenIdx != -1) {
                bool ok = false;
                int lo = QStringView(stepToken).left(hyphenIdx).toInt(&ok);
                if (!ok || lo < minVal || lo > maxVal) return false;
                int hi = QStringView(stepToken).mid(hyphenIdx + 1).toInt(&ok);
                if (!ok || hi < minVal || hi > maxVal) return false;
                rangeStart = lo;
                rangeEnd = hi;
            } else if (stepToken == QStringLiteral("*")) {
                rangeStart = minVal;
                rangeEnd = maxVal;
            } else {
                bool ok = false;
                int single = stepToken.toInt(&ok);
                if (!ok || single < minVal || single > maxVal) return false;
                rangeStart = single;
                rangeEnd = single;
            }
            for (int v = rangeStart; v <= rangeEnd; v += step) {
                if (v == value) return true;
            }
        }
        return false;
    };

    auto maxDays = [](int month, int year) -> int {
        static const int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
        if (month == 2) {
            bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            return leap ? 29 : 28;
        }
        return days[month];
    };

    for (int i = 0; i < maxIterations && cursor <= deadline; ++i) {
        int minute = cursor.time().minute();
        int hour = cursor.time().hour();
        int day = cursor.date().day();
        int month = cursor.date().month();
        int year = cursor.date().year();
        int dow = cursor.date().dayOfWeek() % 7;

        if (!fieldMatches(minField, minute, 0, 59)) { cursor = cursor.addSecs(60); continue; }
        if (!fieldMatches(hourField, hour, 0, 23)) {
            cursor = QDateTime(cursor.date(), QTime((hour + 1) % 24, 0, 0), QTimeZone::UTC);
            if (hour == 23) cursor = cursor.addDays(1);
            continue;
        }
        if (!fieldMatches(monthField, month, 1, 12)) {
            cursor = QDateTime(QDate(year, month, 1).addMonths(1), QTime(0, 0, 0), QTimeZone::UTC);
            continue;
        }
        bool dayOk = fieldMatches(dayField, day, 1, maxDays(month, year));
        bool weekOk = fieldMatches(weekField, dow, 0, 6);
        bool dayMatch = (dayField != QLatin1String("*") && weekField != QLatin1String("*"))
                            ? (dayOk || weekOk)
                            : (dayOk && weekOk);
        if (!dayMatch) { cursor = cursor.addDays(1); cursor.setTime(QTime(0, 0, 0)); continue; }
        return cursor;
    }
    return QDateTime();
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int passed = 0;
    int failed = 0;

    auto check = [&](bool condition, const char *name) {
        if (condition) {
            ++passed;
            qDebug() << "PASS:" << name;
        } else {
            ++failed;
            qDebug() << "FAIL:" << name;
        }
    };

    // 测试 1: cron-daily
    {
        QDateTime now(QDate(2025, 6, 1), QTime(8, 0, 0), QTimeZone::UTC);
        QDateTime result = nextRunTime("0 9 * * *", now);
        check(result.isValid() && result.time().hour() == 9
                   && result.time().minute() == 0,
              "cron-daily: 0 9 * * * -> 9:00 today");
    }

    // 测试 2: cron-daily-from-after
    {
        QDateTime now(QDate(2025, 6, 1), QTime(10, 0, 0), QTimeZone::UTC);
        QDateTime result = nextRunTime("0 9 * * *", now);
        check(result.isValid() && result.date().day() == 2
                   && result.time().hour() == 9 && result.time().minute() == 0,
              "cron-daily-after: 0 9 * * * past 9:00 -> next day 9:00");
    }

    // 测试 3: cron-weekly
    {
        // Friday = 5 in cron (0=Sunday). Qt: 2025-06-06 is Friday
        QDateTime now(QDate(2025, 6, 6), QTime(16, 0, 0), QTimeZone::UTC);
        QDateTime result = nextRunTime("30 17 * * 5", now);
        check(result.isValid() && result.time().hour() == 17
                   && result.time().minute() == 30,
              "cron-weekly: 30 17 * * 5 -> today 17:30");
    }

    // 测试 4: cron-hourly (every 2 hours)
    {
        QDateTime now(QDate(2025, 6, 1), QTime(9, 0, 0), QTimeZone::UTC);
        QDateTime result = nextRunTime("0 */2 * * *", now);
        check(result.isValid() && result.time().hour() == 10
                   && result.time().minute() == 0,
              "cron-hourly: 0 */2 * * * from 9:00 -> 10:00");
    }

    // 测试 5: cron-invalid
    {
        QDateTime now = QDateTime::currentDateTimeUtc();
        QDateTime result = nextRunTime("abc def ghi", now);
        check(!result.isValid(), "cron-invalid: bogus expression -> invalid");
    }

    // 测试 6: cron-midnight
    {
        QDateTime now(QDate(2025, 6, 1), QTime(8, 0, 0), QTimeZone::UTC);
        QDateTime result = nextRunTime("0 0 * * *", now);
        // Should be midnight of next day (June 2)
        check(result.isValid() && result.date().day() == 2
                   && result.time().hour() == 0 && result.time().minute() == 0,
              "cron-midnight: 0 0 * * * -> midnight next day");
    }

    // 测试 7: cron-single-day (Monday at 8:00)
    {
        // 2025-06-02 is Monday
        QDateTime now(QDate(2025, 6, 2), QTime(7, 0, 0), QTimeZone::UTC);
        QDateTime result = nextRunTime("0 8 * * 1", now);
        check(result.isValid() && result.date().dayOfWeek() == 1
                   && result.time().hour() == 8 && result.time().minute() == 0,
              "cron-single-day: 0 8 * * 1 -> Monday 8:00");
    }

    // 测试 8: cron-step-minute
    {
        QDateTime now(QDate(2025, 6, 1), QTime(10, 0, 0), QTimeZone::UTC);
        QDateTime result = nextRunTime("*/15 10 * * *", now);
        check(result.isValid() && result.time().minute() == 15,
              "cron-step-minute: */15 10 * * * -> 10:15");
    }

    qDebug() << "\nResults:" << passed << "passed," << failed << "failed";
    if (failed > 0)
        qFatal("Some tests failed");
    return 0;
}
