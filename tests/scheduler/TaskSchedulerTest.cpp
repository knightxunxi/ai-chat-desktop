#include "scheduler/ScheduledTask.h"
#include "scheduler/TaskScheduler.h"
#include "scheduler/TaskStorage.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTimer>
#include <QTimeZone>

#include <cassert>

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

    // ---- 测试 1: add-task ----
    {
        TaskScheduler scheduler;
        auto task = ScheduledTask::create("TestTask", "0 9 * * *", "run test");
        scheduler.addTask(task);
        auto all = scheduler.allTasks();
        check(all.size() == 1 && all[0].name == "TestTask", "add-task");
    }

    // ---- 测试 2: remove-task ----
    {
        TaskScheduler scheduler;
        auto task = ScheduledTask::create("TestTask", "0 9 * * *", "run test");
        scheduler.addTask(task);
        scheduler.removeTask(task.id);
        check(scheduler.allTasks().isEmpty(), "remove-task");
    }

    // ---- 测试 3: disabled-task 不触发 ----
    {
        TaskScheduler scheduler;
        auto task = ScheduledTask::create("Disabled", "* * * * *", "disabled");
        task.enabled = false;
        scheduler.addTask(task);

        QSignalSpy spy(&scheduler, &TaskScheduler::taskTriggered);
        scheduler.start();

        // 等待 2 秒确认不会触发
        QTimer::singleShot(2000, &app, &QCoreApplication::quit);
        app.exec();

        scheduler.stop();
        check(spy.count() == 0, "disabled-task: no trigger for disabled task");
    }

    // ---- 测试 4: persist (save -> load) ----
    {
        const QString testFile = QDir::tempPath() + "/test_scheduled_tasks.json";
        QFile::remove(testFile);

        TaskStorage storage(testFile);
        QVector<ScheduledTask> input;
        auto t1 = ScheduledTask::create("Task One", "0 9 * * *", "morning");
        auto t2 = ScheduledTask::create("Task Two", "30 17 * * 5", "friday");
        t1.lastRun = QDateTime(QDate(2026, 6, 5), QTime(8, 30, 0), QTimeZone::UTC);
        t1.nextRun = QDateTime(QDate(2026, 6, 5), QTime(9, 0, 0), QTimeZone::UTC);
        input.append(t1);
        input.append(t2);

        check(storage.save(input), "persist: save ok");

        auto loaded = storage.load();
        check(loaded.size() == 2, "persist: load count");
        if (loaded.size() == 2) {
            check(loaded[0].name == "Task One", "persist: name match 1");
            check(loaded[1].cronExpression == "30 17 * * 5", "persist: cron match 2");
            check(loaded[0].lastRun.isValid() && loaded[0].lastRun.toUTC() == t1.lastRun.toUTC(),
                  "persist: lastRun roundtrip");
            check(loaded[0].nextRun.isValid() && loaded[0].nextRun.toUTC() == t1.nextRun.toUTC(),
                  "persist: nextRun roundtrip");
        }

        QFile::remove(testFile);
    }

    // ---- 测试 5: task-valid ----
    {
        ScheduledTask valid;
        valid.name = "Test";
        valid.cronExpression = "0 9 * * *";
        check(valid.isValid(), "task-valid: proper data -> valid");

        ScheduledTask noName;
        noName.cronExpression = "0 9 * * *";
        check(!noName.isValid(), "task-valid: empty name -> invalid");

        ScheduledTask noCron;
        noCron.name = "Test";
        check(!noCron.isValid(), "task-valid: empty cron -> invalid");

        ScheduledTask badCron;
        badCron.name = "Test";
        badCron.cronExpression = "0 9 *";
        check(!badCron.isValid(), "task-valid: 4-field cron -> invalid");
    }

    // ---- 测试 6: cron-nextrun ----
    {
        // 使用一个包含 TaskScheduler 的临时对象来测试
        // 由于 nextRunTime 是私有的，我们通过间接方式验证
        auto task = ScheduledTask::create("Daily", "0 9 * * *", "morning");
        QDateTime now = QDateTime(QDate(2025, 6, 1), QTime(8, 0, 0), QTimeZone::UTC);

        TaskScheduler scheduler;
        scheduler.addTask(task);
        // 启动 tick 会设置 nextRun
        scheduler.start();

        // 不等待，立即停止
        QTimer::singleShot(10, &app, &QCoreApplication::quit);
        app.exec();

        scheduler.stop();

        auto tasks = scheduler.allTasks();
        check(tasks.size() == 1, "cron-nextrun: task present");
        if (tasks.size() == 1) {
            QDateTime next = tasks[0].nextRun;
            check(next.isValid() && next.time().hour() == 9 && next.time().minute() == 0,
                  "cron-nextrun: nextRun set to 9:00");
        }
    }

    // ---- 测试 7: start-stop 状态 ----
    {
        TaskScheduler scheduler;
        check(!scheduler.isRunning(), "start-stop: initially not running");
        scheduler.start();
        check(scheduler.isRunning(), "start-stop: running after start");
        scheduler.stop();
        check(!scheduler.isRunning(), "start-stop: not running after stop");
    }

    // ---- 测试 8: enabled-task 立即按当前分钟触发 ----
    {
        TaskScheduler scheduler;
        auto task = ScheduledTask::create("EveryMinute", "* * * * *", "run now");
        scheduler.addTask(task);

        QSignalSpy spy(&scheduler, &TaskScheduler::taskTriggered);
        scheduler.start();
        scheduler.stop();

        check(spy.count() == 1, "enabled-task: current minute triggers once");
        const auto tasks = scheduler.allTasks();
        check(tasks.size() == 1 && tasks[0].lastRun.isValid(),
              "enabled-task: lastRun set after trigger");
    }

    // ---- 测试 9: update-task ----
    {
        TaskScheduler scheduler;
        auto task = ScheduledTask::create("Original", "0 9 * * *", "orig");
        scheduler.addTask(task);

        task.name = "Updated";
        task.cronExpression = "30 17 * * 5";
        scheduler.updateTask(task);

        auto all = scheduler.allTasks();
        check(all.size() == 1 && all[0].name == "Updated"
                   && all[0].cronExpression == "30 17 * * 5",
              "update-task");
    }

    qDebug() << "\nResults:" << passed << "passed," << failed << "failed";
    if (failed > 0)
        qFatal("Some tests failed");
    return 0;
}
