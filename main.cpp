#include <QApplication>

#include "osmapp.hpp"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  app.setApplicationName("OsmApp");
  OsmApp osmApp;

  osmApp.show();
  return app.exec();
}
