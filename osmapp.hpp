#include "osmwidget.hpp"
#include <QMainWindow>
#include <QLabel>

class OsmApp : public QMainWindow {
  Q_OBJECT

  public:
    OsmApp();
    
  protected:
    void closeEvent(QCloseEvent *event);
    
  private:
    OsmWidget *osmWidget;
    QLabel *latLonLabel;
};