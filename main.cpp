#include "wClient.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("fusion");
    wClient window;
    window.show();
    return app.exec();
}
