#include "ui/MainWindow.h"

#include "support/AppLogger.h"

#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QStringList>
#include <QTimer>
#include <QStyleFactory>
#include <QTextStream>

namespace {

void configureLightTheme(QApplication &app)
{
    if (auto *fusionStyle = QStyleFactory::create(QStringLiteral("Fusion"))) {
        app.setStyle(fusionStyle);
    }

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#f7f8fa")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#111827")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#eef1f4")));
    palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#111827")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#111827")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#1f2933")));
    palette.setColor(QPalette::BrightText, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#2563eb")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#7b8794")));
    app.setPalette(palette);
}

void loadApplicationStyle(QApplication &app)
{
    QFile styleFile(QStringLiteral(":/styles/app.qss"));
    if (!styleFile.open(QFile::ReadOnly | QFile::Text)) {
        return;
    }

    QTextStream stream(&styleFile);
    app.setStyleSheet(stream.readAll());
}

QString argumentValue(const QStringList &arguments, const QString &name)
{
    const int index = arguments.indexOf(name);
    if (index < 0 || index + 1 >= arguments.size()) {
        return {};
    }

    return arguments.at(index + 1);
}

void writeSmokeTestReadyFile(const QString &filePath)
{
    if (filePath.trimmed().isEmpty()) {
        return;
    }

    QDir directory(QFileInfo(filePath).absolutePath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        return;
    }

    QTextStream stream(&file);
    stream << "ready\n";
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);
    QApplication::setApplicationName(QStringLiteral("AI Chat Desktop"));
    QApplication::setOrganizationName(QStringLiteral("AIChatDesktop"));

    configureLightTheme(app);
    QString loggerError;
    if (AppLogger::initialize(&loggerError)) {
        AppLogger::info(QStringLiteral("Application"), QStringLiteral("Application started. logFile=%1").arg(AppLogger::logFilePath()));
    }
    loadApplicationStyle(app);

    MainWindow window;
    window.show();

    const QStringList arguments = QCoreApplication::arguments();
    if (arguments.contains(QStringLiteral("--smoke-test"))) {
        const QString readyFilePath = argumentValue(arguments, QStringLiteral("--smoke-test-ready-file"));
        QTimer::singleShot(0, &window, [readyFilePath]() {
            writeSmokeTestReadyFile(readyFilePath);
        });
        QTimer::singleShot(5000, &window, [&window]() {
            window.close();
        });
        QTimer::singleShot(15000, &app, [&app]() {
            app.quit();
        });
    }

    return app.exec();
}
