#ifndef OSM_WIDGET_H
#define OSM_WIDGET_H

#include <QWidget>
#include "osmparser.hpp"

class OsmWidget : public QWidget {
  Q_OBJECT

  private:
    OsmParser *osm;
    int zoom;

  public:
    OsmWidget(QWidget *parent=0);
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setOsmParser(OsmParser *);
    
  public slots:
    void set_zoom(int);

  protected:
    void paintEvent(QPaintEvent *event);
};

int calc_dist(double, double, double, double, double *, double *);
#endif // ifndef OSM_WIDGET_H