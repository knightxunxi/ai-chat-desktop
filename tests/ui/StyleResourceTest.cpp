#include <QCoreApplication>
#include <QFile>

#include <cassert>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QFile styleFile(QStringLiteral(":/styles/app.qss"));
    assert(styleFile.open(QFile::ReadOnly | QFile::Text));

    const QString style = QString::fromUtf8(styleFile.readAll());
    assert(style.contains(QStringLiteral("QWidget#centralRoot")));
    assert(style.contains(QStringLiteral("QPushButton#sendButton[stopMode=\"true\"]")));
    assert(style.contains(QStringLiteral("QFrame#sessionFilterBar")));
    assert(style.contains(QStringLiteral("QFrame#sessionFilterBar QPushButton:checked")));

    return 0;
}
