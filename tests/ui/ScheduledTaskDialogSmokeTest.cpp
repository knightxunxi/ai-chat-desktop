#include "ui/ScheduledTaskDialog.h"

#include <QApplication>
#include <QDialog>
#include <QTableWidget>

#include <cassert>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 1. Create ScheduledTaskDialog with null controller (smoke test)
    ScheduledTaskDialog dialog(nullptr);

    // 2. Verify it is a QDialog
    assert(qobject_cast<QDialog *>(&dialog) != nullptr);

    // 3. Verify table starts empty
    auto *table = dialog.findChild<QTableWidget *>();
    assert(table != nullptr);
    assert(table->rowCount() == 0);
    assert(table->columnCount() == 6);

    return 0;
}
