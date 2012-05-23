#include <QtGui>

#include "osmapp.hpp"

OsmApp::OsmApp() {
  osmWidget = new OsmWidget;
  setCentralWidget(osmWidget);

}

void OsmApp::closeEvent(QCloseEvent *event) {
//   osmWidget->destroy();
  event->accept();
}

