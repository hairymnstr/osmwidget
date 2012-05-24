#ifndef OSM_WIDGET_H
#define OSM_WIDGET_H

#include <QWidget>
#include <QGLWidget>
#include <QMouseEvent>
#include "osmparser.hpp"

#define LATLONGRID 1

class OsmWidget : public QGLWidget {
  Q_OBJECT

  private:
    OsmDataSource *osm;
    int zoom;
    int dragStartX;
    int dragStartY;
    int tempMoveX;
    int tempMoveY;
    bool renderFast;
    double lonCentre;
    double latCentre;
    double wDegrees;
    double hDegrees;
    QVector<QPainterPath> path;
    bool drawGrid;
    int gridType;
    double gridLatStep;
    double gridLonStep;
    
    void translateView(int x, int y);
    void updateCache();
    void updatePaths();
    
  public:
    OsmWidget(QWidget *parent=0);
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setOsmSource(OsmDataSource *);
    
  public slots:
    void setZoom(int);
//     void destroy(QObject *);

  signals:
    void locationUpdateText(QString loc);
    
  protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
};

int calc_dist(double, double, double, double, double *, double *);
void geodesic_fwd(double, double, double, double, double *, double *);
#endif // ifndef OSM_WIDGET_H