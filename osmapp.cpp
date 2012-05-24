#include <QtGui>

#include "osmapp.hpp"

OsmApp::OsmApp() {
  osmWidget = new OsmWidget;
  latLonLabel = new QLabel;
  setCentralWidget(osmWidget);
  
  latLonLabel->setText("51.4,-2.3");
  
  connect(osmWidget, SIGNAL(locationUpdateText(QString)), latLonLabel, SLOT(setText(QString)));

  statusBar()->addPermanentWidget(latLonLabel);
}

void OsmApp::closeEvent(QCloseEvent *event) {
//   osmWidget->destroy();
  event->accept();
}

