#include "gui/MainWindow.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("COS202");
    app.setApplicationName("SCAPMS");
    app.setStyle(QStyleFactory::create("Fusion"));

    // Always use the application's own light colours, even when the computer uses a dark theme.
    // Without this, drop-down lists, the calendar and menus take dark backgrounds from the system
    // while the style sheet keeps the text dark, so the text cannot be read.
    QPalette light;
    light.setColor(QPalette::Window, QColor("#F4F7F9"));
    light.setColor(QPalette::WindowText, QColor("#1F2A33"));
    light.setColor(QPalette::Base, QColor("#FFFFFF"));
    light.setColor(QPalette::AlternateBase, QColor("#F4F7F9"));
    light.setColor(QPalette::Text, QColor("#1F2A33"));
    light.setColor(QPalette::PlaceholderText, QColor("#8A97A0"));
    light.setColor(QPalette::Button, QColor("#FFFFFF"));
    light.setColor(QPalette::ButtonText, QColor("#1F2A33"));
    light.setColor(QPalette::Highlight, QColor("#1F8A94"));
    light.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    light.setColor(QPalette::ToolTipBase, QColor("#12343B"));
    light.setColor(QPalette::ToolTipText, QColor("#FFFFFF"));
    light.setColor(QPalette::Disabled, QPalette::Text, QColor("#9AA8B0"));
    light.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#9AA8B0"));
    light.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#9AA8B0"));
    app.setPalette(light);

    QFile qss(":/style.qss");
    if (qss.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    scapms::gui::MainWindow window;
    window.show();
    return app.exec();
}
