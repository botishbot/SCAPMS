#include "gui/MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("COS202");
    app.setApplicationName("SCAPMS");
    app.setStyle(QStyleFactory::create("Fusion"));

    QFile qss(":/style.qss");
    if (qss.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    scapms::gui::MainWindow window;
    window.show();
    return app.exec();
}
