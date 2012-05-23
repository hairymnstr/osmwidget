#include "osmwidget.hpp"
#include <QMainWindow>

class OsmApp : public QMainWindow {
  Q_OBJECT

  public:
    OsmApp();
    
  protected:
    void closeEvent(QCloseEvent *event);
    
  private:
    OsmWidget *osmWidget;
};