#ifndef OSM_WIDGET_H
#define OSM_WIDGET_H

#include <QWidget>
#include <QGLWidget>
#include "osmparser.hpp"

class OsmWidget : public QGLWidget {
  Q_OBJECT

  private:
    OsmDataSource *osm;
    int zoom;

  public:
    OsmWidget(QWidget *parent=0);
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setOsmSource(OsmDataSource *);
    
  public slots:
    void set_zoom(int);

  protected:
    void paintEvent(QPaintEvent *event);
};

int calc_dist(double, double, double, double, double *, double *);
#endif // ifndef OSM_WIDGET_H