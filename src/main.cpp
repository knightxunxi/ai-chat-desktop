#include "ui/MainWindow.h"

#include "support/AppLogger.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QPalette>
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

    return app.exec();
}
