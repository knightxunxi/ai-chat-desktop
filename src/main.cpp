#include "ui/MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>

namespace {

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
    QApplication::setApplicationName(QStringLiteral("AI Chat Desktop"));
    QApplication::setOrganizationName(QStringLiteral("AIChatDesktop"));

    loadApplicationStyle(app);

    MainWindow window;
    window.show();

    return app.exec();
}
