#include <QApplication>
#include <QPainter>
#include <QXmlSimpleReader>
#include <QXmlInputSource>
#include <QFile>
#include <QHBoxLayout>
#include <QSlider>
#include <QWidget>
#include <QGLWidget>
#include <QPaintEvent>
#include <QImage>

#include <iostream>
#include <cmath>

#include "osmwidget.hpp"
#include "osmparser.hpp"

OsmWidget::OsmWidget(QWidget *parent) : QGLWidget(QGLFormat(QGL::SampleBuffers), parent) {
  lonCentre = -2.3;
  latCentre = 51.4;
  setZoom(20);
}

QSize OsmWidget::minimumSizeHint() const {
  return QSize(100, 100);
}

QSize OsmWidget::sizeHint() const {
  return QSize(300, 200);
}

void OsmWidget::paintEvent(QPaintEvent *) {
  glEnable(GL_MULTISAMPLE);
  QPainter painter(this);

//   double wibble;
//   double xWidthTop, xWidthBottom;
  int sf = 10000;
  
//   calc_dist(51.5,-2.4, 51.5,-2.2, &xWidthTop, &wibble);
//   calc_dist(51.3,-2.4, 51.3,-2.2, &xWidthBottom, &wibble);
//   
//   if((fabs(xWidthTop - xWidthBottom) / qMax(xWidthTop, xWidthBottom)) > 0.01) {
//     std::cout << "Warning map aberration too large!!!" << std::endl;
//   }

//   double xWidth = (xWidthTop + xWidthBottom) / 2.0;
//   double yHeight;
//   calc_dist(51.3,-2.4, 51.5,-2.4, &yHeight, &wibble);
  
  double lonc, latc;
//   std::cout << "old: " << zoom * 0.2 / xWidth << " x " << zoom * 0.2 / yHeight << std::endl;
//   wDegrees = zoom * 0.2 / xWidth;
//   hDegrees = zoom * -0.2 / yHeight;
  
  if(renderFast) {
    lonc = lonCentre + wDegrees * -tempMoveX;
    latc = latCentre + hDegrees * -tempMoveY;
  } else {
    lonc = lonCentre;//-2.3;
    latc = latCentre;//51.4;
  }
  
  painter.scale(1.0 / (wDegrees * sf), 1.0 / (hDegrees * sf));
  painter.translate(-(lonc-wDegrees*width()/2)*sf, -(latc-hDegrees*height()/2)*sf);

  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(QBrush(QColor(0xff, 0xff, 0xee)));
  painter.setPen(QPen(QBrush(QColor(0x20,0x20,0xff)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.fillRect(QRect((int)round(-2.4*sf), (int)round(51.5*sf), (int)round(0.2 * sf), (int)round(-0.2*sf)),QBrush(QColor(0xf1,0xee,0xe8)));
  painter.drawRect(QRect(-20, 10, 300, -300));
  painter.setBrush(QBrush(QColor(0,0,0), Qt::NoBrush));
  
  QVector<Way> *ways;
  if(renderFast) {
    QVector<QPainterPath> path(2);
    ways = osm->getWays("highway", "trunk");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[0].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[0].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    ways = osm->getWays("highway", "primary");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[0].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[0].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    ways = osm->getWays("waterway", "riverbank");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[1].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[1].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    painter.setPen(QPen(QBrush(QColor(0x20,0x20,0xff)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(0x40,0x40,0xff)));
    painter.drawPath(path[1].simplified());
    painter.setPen(QPen(QBrush(QColor(0,0,0)), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(0,0,0), Qt::NoBrush));
    painter.drawPath(path[0]);
  } else {
    QVector<QPainterPath> path(6);
    ways = osm->getWays("highway", "trunk");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[0].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[0].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    ways = osm->getWays("highway", "primary");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[1].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[1].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    ways = osm->getWays("highway", "secondary");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[2].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[2].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    ways = osm->getWays("highway", "tertiary");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[3].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[3].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    ways = osm->getWays("highway", "residential");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[4].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[4].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    ways = osm->getWays("highway", "unclassified");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[4].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[4].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    ways = osm->getWays("waterway", "riverbank");
    for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
      path[5].moveTo((int)round(w->nodes.at(0).lon * sf), (int)round(w->nodes.at(0).lat * sf));
      for(int n=1;n<w->nodes.size();n++) {
        path[5].lineTo((int)round(w->nodes.at(n).lon * sf), (int)round(w->nodes.at(n).lat * sf));
      }
    }
    delete ways;
    
    painter.setPen(QPen(QBrush(QColor(0x20,0x20,0xff)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(0x40,0x40,0xff)));
    painter.drawPath(path[5].simplified());
    painter.setPen(QPen(QBrush(QColor(0xc0,0xc0,0xc0)), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(0,0,0), Qt::NoBrush));
    for(int i=0;i<5;i++) {
      painter.drawPath(path[i]);
    }
    painter.setPen(QPen(QBrush(QColor(0xff,0xff,0xff)), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[4]);
    painter.setPen(QPen(QBrush(QColor(0xff,0xff,0x80)), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[3]);
    painter.setPen(QPen(QBrush(QColor(0xff,0x80,0x20)), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[2]);
    painter.setPen(QPen(QBrush(QColor(0x80,0,0)), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[1]);
    painter.setPen(QPen(QBrush(QColor(0x40,0x80,0x40)), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[0]);
  }
  painter.setTransform(QTransform());   // set identity
  
  painter.setPen(QPen(QBrush(QColor(0, 0, 0)), 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
  
  // log code makes sure the scale marker is always 1xxx or 5xxx regardless of absolute magnitude
  int dist = 50 * zoom;
  if(log10(dist) - floor(log10(dist)) < 0.875) {
    if(log10(dist) - floor(log10(dist)) > 0.3979) {
      dist = (int)round(pow(10, floor(log10(dist)) + 0.69897));
    } else {
      dist = (int)pow(10, floor(log10(dist)));
    }
  } else {
    dist = (int)pow(10, ceil(log10(dist)));
  }
  
  double distdraw = round(dist/ (double)zoom);
  
  QPainterPath scaleMark;
  scaleMark.moveTo(20, height()-25);
  scaleMark.lineTo(20, height()-20);
  scaleMark.lineTo(20 + distdraw, height()-20);
  scaleMark.lineTo(20 + distdraw, height()-25);
  
  painter.drawPath(scaleMark);
  
  if(dist >= 1000) {
    painter.drawText(QPoint(20, height()-10), QString("%1 km").arg(dist/1000));
  } else {
    painter.drawText(QPoint(20, height()-10), QString("%1 m").arg(dist));
  }
  
  painter.end();
  glDisable(GL_MULTISAMPLE);
//   std::cout << "Done." << std::endl;
}

void OsmWidget::mousePressEvent(QMouseEvent *event) {
  if(event->button() == Qt::LeftButton) {
    dragStartX = event->x();
    dragStartY = event->y();
    tempMoveX = 0;
    tempMoveY = 0;
    renderFast = true;
    update();
  }
}

void OsmWidget::mouseMoveEvent(QMouseEvent *event) {
  if(event->buttons() & Qt::LeftButton) {
    tempMoveX = event->x() - dragStartX;
    tempMoveY = event->y() - dragStartY;
    update();
  }
}

void OsmWidget::mouseReleaseEvent(QMouseEvent *event) {
  if(event->button() == Qt::LeftButton) {
    renderFast=false;
    translateView(event->x() - dragStartX, event->y() - dragStartY);
    update();
  }
}

void OsmWidget::setOsmSource(OsmDataSource *p) {
  osm = p;
}

void OsmWidget::setZoom(int value) {
  double wibble, latTop, lonRight;
  zoom = value;
  
  geodesic_fwd(latCentre, lonCentre, 0, 1000, &latTop, &wibble);
  geodesic_fwd(latCentre, lonCentre, 90, 1000, &wibble, &lonRight);
  
  wDegrees = (lonRight - lonCentre) * zoom / 1000.0;
  hDegrees = -(latTop - latCentre) * zoom / 1000.0;          // flip sign so origin is bottom left
  
  update();
}

void OsmWidget::translateView(int x, int y) {
  lonCentre += -x * wDegrees;
  latCentre += -y * hDegrees;
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  
  OsmDataSource *osm = new OsmDataSource();
  
  QWidget *win = new QWidget;
  
  QHBoxLayout *layout = new QHBoxLayout;
  
//   osm->fetchData();
  
//   osm->listWayTags();

  OsmWidget *surface;
  surface = new OsmWidget;
  surface->setOsmSource(osm);
  
  QSlider *slider = new QSlider(Qt::Vertical);
  slider->setMaximum(20);
  slider->setMinimum(1);
  slider->setValue(20);
  win->connect(slider, SIGNAL(sliderMoved(int)), surface, SLOT(setZoom(int)));
  
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

/**
 *  geodesic_fwd - function to find the lat/lon of a point given a start, heading and range.
 *
 * PARAMETERS
 * slat - origin latitude
 * slon - origin longitude
 * heading - bearing from the origin to the end point
 * range - distance in metres from the origin to the end point
 * 
 * RETURNS
 * elat - latitude of end point
 * elon - longitude of end point
 *
 * NOTES
 * Only works for co-ordinates on a WGS84 reference elipsoid, for other geoids parameters
 * f, a and b in the code would need to be altered.
 **/
void geodesic_fwd(double slat, double slon, double heading, double range, double *elat, double *elon) {
  double f, a, b;
  double r, baz, cu, su, sa, cf, sf, tu, y;
  double tol;
  double c2a, cy, sy, e, c, d, cz, x;
  
  slat = slat * M_PI / 180.0;
  slon = slon * M_PI / 180.0;

  heading = heading * M_PI / 180.0;

  // tolerance and relevant geoid parameters for WGS84
  tol = 0.5e-13;
  f = 1 / 298.257223563;
  a = 6378137.0;
  b = 6356752.3142;

  r = 1.0 - f;
  tu = r * sin(slat) / cos(slat);
  sf = sin(heading);
  cf = cos(heading);

  baz = 2.0 * atan2(tu, cf);
  cu = 1.0/sqrt(pow(tu, 2.0) + 1);
  su = tu * cu;
  sa = cu * sf;
  c2a = -1.0 * pow(sa, 2.0) + 1;
  x = sqrt((1.0 / r / r - 1) * c2a + 1) + 1;
  x = (x - 2) / x;
  c = 1 - x;
  c = (1 + pow(x, 2.0) / 4.0) / c;
  d = (0.375 * pow(x, 2.0) - 1) * x;
  tu = range / r / a / c;
  y = tu;

  do {
    sy = sin(y);
    cy = cos(y);
    cz = cos(baz + y);
    e = 2.0 * pow(cz, 2.0) - 1;
    c = y;
    x = e * cy;
    y = 2.0 * e - 1;
    y = (((4.0 * pow(sy, 2.0) - 3) * y * cz * d/6.0 + x) * d/4.0 - cz) * sy * d + tu;
  } while(fabs(y - c) > tol);

  baz = cu * cy * cf - su * sy;
  c = r * sqrt(pow(sa, 2.0) + pow(baz, 2));
  d = su * cy + cu * sy * cf;
  *elat = atan2(d, c);
  c = cu * cy - su * sy * cf;
  x = atan2(sy * sf, c);
  c = ((-3.0 * c2a + 4.0) * f + 4.0) *c2a * f/16.0;
  d = ((e * cy * c + cz) * sy * c + y) * sa;
  *elon = slon + x - (1 - c) * d * f;
  baz = atan2(sa, baz) + M_PI;

  *elat = (*elat) / M_PI * 180.0;
  *elon = (*elon) / M_PI * 180.0;
  return;
}