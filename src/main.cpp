#include "widget.hpp"
#include <QApplication>
#include <QScreen>
#include <vector>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    auto* widget = new PriceWidget(PriceWidget::loadSymbolsFromConfig());
    widget->show();

    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    widget->move(screenGeometry.width() - widget->width() - 96, 96);
    
    return app.exec();
}
