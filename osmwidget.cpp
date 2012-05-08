#include <QApplication>
#include <QPainter>
#include <QXmlSimpleReader>
#include <QXmlInputSource>
#include <QFile>
#include <QHBoxLayout>
#include <QSlider>
#include <QWidget>
#include <QPaintEvent>

#include <iostream>
#include <cmath>

#include "osmwidget.hpp"
#include "osmparser.hpp"

OsmWidget::OsmWidget(QWidget *parent) : QWidget(parent) {
  zoom = 20;
}

QSize OsmWidget::minimumSizeHint() const {
  return QSize(100, 100);
}

QSize OsmWidget::sizeHint() const {
  return QSize(300, 200);
}

void OsmWidget::paintEvent(QPaintEvent *) {
  QPainter painter(this);

  double wibble;
  double xWidthTop, xWidthBottom;
  int sf = 10000;
  
  calc_dist(51.5,-2.4, 51.5,-2.2, &xWidthTop, &wibble);
  calc_dist(51.3,-2.4, 51.3,-2.2, &xWidthBottom, &wibble);
  
  if((fabs(xWidthTop - xWidthBottom) / qMax(xWidthTop, xWidthBottom)) > 0.01) {
    std::cout << "Warning map aberration too large!!!" << std::endl;
  }

  double xWidth = (xWidthTop + xWidthBottom) / 2.0;
  double yHeight;
  calc_dist(51.3,-2.4, 51.5,-2.4, &yHeight, &wibble);
  
  std::cout << xWidth / 1000.0 << "km x " << yHeight / 1000.0 << "km" << std::endl;
  
//   xWidth = xWidth * zoom;
//   xHeight = xHeight * zoom;
  
//   painter.setWindow(QRect(-24000,515000,2000,-2000));
//   painter.setWindow(-100, -100, 200, 200);
//   int w, h;
  double wMetres, hMetres;
  double wDegrees, hDegrees;
  double lonc = -2.3, latc = 51.4;
  
  wMetres = width() * zoom;
  hMetres = height() * zoom;
  
  wDegrees = wMetres * 0.2 / xWidth;
  hDegrees = hMetres * -0.2 / yHeight;
  
  std::cout << wDegrees << " x " << hDegrees << std::endl;
  painter.setWindow(QRect((int)round((lonc - wDegrees/2) * sf), (int)round((latc - hDegrees/2) * sf),
                          (int)round(wDegrees * sf), (int)round(hDegrees * sf)));
//   painter.setWindow(QRect(-24000, 515000, (int)round(wDegrees), (int)round(hDegrees)));
  
  
//   if((height() * (xWidth / yHeight)) < width()) {
//     w = qMax((int)round(height() * xWidth / yHeight), 1);
//     h = height();
//   } else {
//     w = width();
//     h = qMax((int)round(width() * yHeight / xWidth), 1);
//   }
  int w = width();
  int h = height();
  int x = 0;//(width() - w) / 2;
  int y = 0;//(height() - h) / 2;
  std::cout << width() << ", " << height() << std::endl;
  painter.setViewport(x, y, w, h);
  painter.setPen(QPen(Qt::blue, 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
  painter.setBrush(QBrush(Qt::green, Qt::NoBrush));
  painter.setRenderHint(QPainter::Antialiasing, true);
//   for(int i=0;i<osm->node_count();i++) {
//     painter.drawEllipse(int(osm->node(i)->lon * sf), int(osm->node(i)->lat*sf), 2, 2);
//   }
}

void OsmWidget::setOsmSource(OsmDataSource *p) {
  osm = p;
}

void OsmWidget::set_zoom(int value) {
  zoom = value;
//   paintEvent(new QPaintEvent(QRect(0,0,width(),height())));
  update();
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  
  OsmDataSource *osm = new OsmDataSource();
  
  QWidget *win = new QWidget;
  
  QHBoxLayout *layout = new QHBoxLayout;
  
  osm->fetchData();

  OsmWidget *surface;
  surface = new OsmWidget;
  surface->setOsmSource(osm);
  
  QSlider *slider = new QSlider(Qt::Vertical);
  slider->setMaximum(20);
  slider->setMinimum(1);
  slider->setValue(20);
  win->connect(slider, SIGNAL(sliderMoved(int)), surface, SLOT(set_zoom(int)));
  
  layout->addWidget(surface);
  layout->addWidget(slider);
  
  win->setLayout(layout);
  win->show();
//   surface->show();

  int ret = app.exec();
  
  delete osm;
  
  return ret;
}

/**
 * calc_dist - find the range and heading between two pairs of WGS84 co-ordinates
 *
 * PARAMETERS:
 *  lat1,lon1: Lat/Long for the first point
 *  lat2,lon2: Lat/Long for the second point
 * RETURNS:
 *  0: success range and bearing should be valid
 *  -1: formula failed to work, points are on opposite sides of the planet.
 *  range: distance in metres between the two points
 *  bearing: a heading from point 1 to point 2 (degrees)
 **/
int calc_dist(double lat1, double lon1, double lat2, double lon2, double *range, double *bearing) {
  double a = 6378137;
  double b = 6356752.3142;
  double f = 1.0 / 298.257223563;       // WGS-84
  
  double L = (lon2 - lon1) * M_PI / 180.0;
  double U1 = atan((1.0 - f) * tan(lat1 * M_PI / 180.0));
  double U2 = atan((1.0 - f) * tan(lat2 * M_PI / 180.0));
  double sinU1 = sin(U1);
  double sinU2 = sin(U2);
  double cosU1 = cos(U1);
  double cosU2 = cos(U2);
  double sinSigma, sinLambda, cosLambda, cosSigma, sigma, sinAlpha;
  double cosSqAlpha, cos2SigmaM;
  double C;
  double lambdaP;
  double uSq, A, B, deltaSigma;
  
  double lmda = L;
  int iterLimit = 100;
  do {
    sinLambda = sin(lmda);
    cosLambda = cos(lmda);
    sinSigma = sqrt((cosU2*sinLambda) * (cosU2 * sinLambda) + (cosU1 *sinU2 - sinU1*cosU2*cosLambda) * (cosU1*sinU2-sinU1*cosU2*cosLambda));
    if(sinSigma == 0) {
      return 0; // co-incident points
    }
    cosSigma = sinU1 * sinU2 + cosU1*cosU2*cosLambda;
    sigma = atan2(sinSigma, cosSigma);
    sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
    cosSqAlpha = 1.0 - sinAlpha * sinAlpha;
    if(cosSqAlpha == 0) {
      cos2SigmaM = 0; // equatorial line
    } else {
      cos2SigmaM = cosSigma - 2*sinU1*sinU2/cosSqAlpha;
    }
    C = f/16.0*cosSqAlpha * (4.0 + f * (4.0 - 3.0*cosSqAlpha));
    lambdaP = lmda;
    lmda = (L + (1.0 - C) * f * sinAlpha * (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1+2*cos2SigmaM*cos2SigmaM))));
  } while((fabs(lmda-lambdaP) > 1e-12) && (--iterLimit > 0));
  
  if(iterLimit == 0) {
    return -1;
  }
  
  uSq = cosSqAlpha * (a*a - b*b) / (b*b);
  A = 1.0 + uSq / 16384 * (4096 + uSq * (-768+uSq*(320-175*uSq)));
  B = uSq/1024 * (256+uSq*(-128+uSq*(74-47*uSq)));
  deltaSigma = (B*sinSigma*(cos2SigmaM+B/4*(cosSigma*(-1+2*cos2SigmaM) - B/6 * cos2SigmaM*(-3+4*sinSigma*sinSigma)*(-3+4*cos2SigmaM*cos2SigmaM))));
  *range = b*A*(sigma-deltaSigma);
  
  *bearing = atan2(cosU1 * sinLambda, sinU1 * cosU2*-1 + cosU1 * sinU2 * cosLambda) * 180.0 / M_PI;
  return 0;
}