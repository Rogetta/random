#include "MainWindow.h"
#include <QApplication>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("Rogetta");
    QApplication::setApplicationName("random");
    QApplication::setApplicationVersion("1.0.0");
    MainWindow window;
    window.show();
    return app.exec();
}
