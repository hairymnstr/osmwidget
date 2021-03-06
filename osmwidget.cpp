#include <QApplication>
#include <QPainter>
#include <QXmlSimpleReader>
#include <QXmlInputSource>
#include <QFile>
#include <QHBoxLayout>
#include <QSlider>
#include <QWidget>
#include <QGLWidget>
#include <QFont>
#include <QToolTip>
#include <QPaintEvent>
#include <QImage>

#include <iostream>
#include <cmath>

#include "osmwidget.hpp"
#include "osmparser.hpp"
#include "progresswidget.hpp"

OsmWidget::OsmWidget(QWidget *parent) : QGLWidget(QGLFormat(QGL::SampleBuffers), parent) {
  lonCentre = -2.3;
  latCentre = 51.4;
  renderFast = false;
  drawGrid = true;
  gridType = LATLONGRID;
  gridLatStep = 0.1;
  gridLonStep = 0.1;
  path.resize(8);
  
  osm = new OsmDataSource(this);
  std::cout <<"Setting zoom state..." << std::endl;
  std::cout << "Widget size " << width() << " x " << height() << std::endl;
  setZoom(20);
  std::cout << "Zoom done" << std::endl;
}

QSize OsmWidget::minimumSizeHint() const {
  return QSize(100, 100);
}

QSize OsmWidget::sizeHint() const {
  return QSize(300, 200);
}

QPointF OsmWidget::toScreenCoordinates(double lat, double lon) {
  double y = (lat - latCentre) / degreePerPixelY + height()/2;
  double x = (lon - lonCentre) / degreePerPixelX + width()/2;
  return QPointF(x, y);
}

void OsmWidget::paintEvent(QPaintEvent *) {
  glEnable(GL_MULTISAMPLE);
  QPainter painter(this);

//   int sf = 10000;
  
  double lonc, latc;

  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(QBrush(QColor(0xff, 0xff, 0xee)));
//   painter.fillRect(QRect((int)round((lonc-degreePerPixelX *width()/2)*sf), (int)round((latc-degreePerPixelY*height()/2)*sf), (int)round(degreePerPixelX*width() * sf), (int)round(degreePerPixelY*height()*sf)),QBrush(QColor(0xf1,0xee,0xe8)));
  painter.fillRect(0, 0, width(), height(), QBrush(QColor(0xf1,0xee,0xe8)));

  if(renderFast) {
    lonc = lonCentre + degreePerPixelX * -tempMoveX;
    latc = latCentre + degreePerPixelY * -tempMoveY;
    painter.translate(tempMoveX, tempMoveY);
  } else {
    lonc = lonCentre;
    latc = latCentre;
  }
  
//   painter.scale(1.0 / (degreePerPixelX * sf), 1.0 / (degreePerPixelY * sf));
//   painter.translate(-(lonc-degreePerPixelX*width()/2)*sf, -(latc-degreePerPixelY*height()/2)*sf);


  painter.setBrush(QBrush(QColor(0,0,0), Qt::NoBrush));

  if(drawGrid) {
    if(gridType == LATLONGRID) {
      double latmin = latc + (height() * degreePerPixelY) / 2.0;
      double lonmin = lonc - (width() * degreePerPixelX) / 2.0;
      double latmax = latc - (height() * degreePerPixelY) / 2.0;
      double lonmax = lonc + (width() * degreePerPixelX) / 2.0;
//       std::cout << latmin << " " << latmax << "  " << lonmin << " " << lonmax << std::endl;
      double la = ceil(latmin / gridLatStep) * gridLatStep;
      while(la < latmax) {
        painter.drawLine(toScreenCoordinates(la, lonmin), toScreenCoordinates(la, lonmax));
        la += gridLatStep;
      }
      double lo = ceil(lonmin / gridLonStep) * gridLonStep;
      while(lo < lonmax) {
        painter.drawLine(toScreenCoordinates(latmin, lo), toScreenCoordinates(latmax, lo));
        lo += gridLonStep;
      }
    }
  }
  
  if(renderFast) {
    painter.setPen(QPen(QBrush(QColor(0x20,0x20,0xff)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(0x40,0x40,0xff)));
    painter.drawPath(path[7]);
    painter.setBrush(QBrush(QColor(0,0,0), Qt::NoBrush));
    painter.setPen(QPen(QBrush(QColor(0,0,0)), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[1]);
    painter.setPen(QPen(QBrush(QColor(0,0,0)), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[0]);
  } else {
    
    painter.setPen(QPen(QBrush(QColor(0x40,0x40,0xff)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(0x80,0x80,0xff)));
    painter.drawPath(path[7]);
    painter.setBrush(QBrush(QColor(0,0,0), Qt::NoBrush));
    painter.setPen(QPen(QBrush(QColor(0x80,0x80,0xff)), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[6]);
    painter.setPen(QPen(QBrush(QColor(0xc0,0xc0,0xc0)), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for(int i=0;i<6;i++) {
      painter.drawPath(path[i]);
    }
    painter.setPen(QPen(QBrush(QColor(0xff,0xff,0xff)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[5]);
    painter.setPen(QPen(QBrush(QColor(0xff,0xff,0x80)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[4]);
    painter.setPen(QPen(QBrush(QColor(0xff,0x80,0x20)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[3]);
    painter.setPen(QPen(QBrush(QColor(0x80,0,0)), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[2]);
    painter.setPen(QPen(QBrush(QColor(0x40,0x80,0x40)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[1]);
    painter.setPen(QPen(QBrush(QColor(0x00,0x00,0xd0)), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[0]);
    painter.setPen(QPen(QBrush(QColor(0xc0,0xc0,0xc0)), 0.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path[0]);
    
    painter.setPen(QPen(QBrush(QColor(0xff,0x00,0x00)), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for(int i=0;i<pointLayers.size();i++) {
      for(int j=0;j<pointLayers[i]->size();j++) {
        painter.drawEllipse(toScreenCoordinates((*pointLayers[i])[j].x(),(*pointLayers[i])[j].y()),5,5);
      }
    }
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
  
  painter.setFont(QFont("sans-serif", 6));
  painter.drawText(QPoint(width()-250, height()-10), QString("Map data Copyright OpenStreetMap contributors, CC BY-SA"));
  
  painter.end();
  glDisable(GL_MULTISAMPLE);
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
  if(tipFlags) {
    double lonc = lonCentre + degreePerPixelX * (event->x() - width()/2);
    double latc = latCentre + degreePerPixelY * (event->y() - height()/2);
    QString tip = QString("%1, %2").arg(latc).arg(lonc);
    QToolTip::showText(QPoint(event->x()+16, event->y()+16), tip, this);
  }
}

void OsmWidget::mouseReleaseEvent(QMouseEvent *event) {
  if(event->button() == Qt::LeftButton) {
    renderFast=false;
    translateView(event->x() - dragStartX, event->y() - dragStartY);
    updateCache();
    updatePaths();
    update();
  }
}

void OsmWidget::resizeEvent(QResizeEvent *) {
  updateCache();
  updatePaths();
}

void OsmWidget::setZoom(int value) {
  double wibble, latTop, lonRight;
  zoom = value;
//   std::cout << "Calculating scaling factors" << std::endl;
  geodesic_fwd(latCentre, lonCentre, 0, 1000, &latTop, &wibble);
  geodesic_fwd(latCentre, lonCentre, 90, 1000, &wibble, &lonRight);
  
  degreePerPixelX = (lonRight - lonCentre) * zoom / 1000.0;
  degreePerPixelY = -(latTop - latCentre) * zoom / 1000.0;          // flip sign so origin is bottom left
  
  updateCache();
  updatePaths();
  
  update();
}

void OsmWidget::translateView(int x, int y) {
  lonCentre += -x * degreePerPixelX;
  latCentre += -y * degreePerPixelY;
  emit locationUpdateText(QString("%1, %2").arg(latCentre).arg(lonCentre));
}

void OsmWidget::updateCache() {
  // check and see what tiles will be displayed now
  double lon = degreePerPixelX * width();
  double lat = degreePerPixelY * width();
  double lonMin = lonCentre - lon/2.0;
  double lonMax = lonCentre + lon/2.0;
  double latMin = latCentre - lat/2.0;
  double latMax = latCentre + lat/2.0;
  
  int lonMinCache = (int)floor(lonMin * 5);
  int lonMaxCache = (int)ceil(lonMax * 5);
  int latMinCache = (int)floor(latMin * 5);
  int latMaxCache = (int)ceil(latMax * 5);
  
  for(int i = lonMinCache;i<=lonMaxCache;i++) {
    for(int j = latMinCache;j<=latMaxCache;j++) {
      osm->cacheTile(i,j);
    }
  }
}

void OsmWidget::updatePaths() {
//   int sf = 10000;
  QVector<Way> *ways;
  osm->selectArea(latCentre+degreePerPixelY*height(),lonCentre-degreePerPixelX*width(), latCentre-degreePerPixelY*height(),lonCentre+degreePerPixelX*width());
  ways = osm->getWays("highway", "motorway");
  path[0] = QPainterPath();
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[0].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[0].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
  ways = osm->getWays("highway", "trunk");
  path[1] = QPainterPath();
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[1].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[1].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
  ways = osm->getWays("highway", "primary");
  path[2] = QPainterPath();
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[2].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[2].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
  ways = osm->getWays("highway", "secondary");
  path[3] = QPainterPath();
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[3].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[3].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
  ways = osm->getWays("highway", "tertiary");
  path[4] = QPainterPath();
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[4].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[4].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
  ways = osm->getWays("highway", "residential");
  path[5] = QPainterPath();
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[5].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[5].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
  ways = osm->getWays("highway", "unclassified");
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[5].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[5].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
  ways = osm->getWays("waterway", "canal");
  path[6] = QPainterPath();
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[6].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[6].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
  ways = osm->getWays("waterway", "riverbank");
  path[7] = QPainterPath();
  for(QVector<Way>::iterator w=ways->begin();w != ways->end();++w) {
    path[7].moveTo(toScreenCoordinates(w->nodes[0].lat, w->nodes[0].lon));
    for(int n=1;n<w->nodes.size();n++) {
      path[7].lineTo(toScreenCoordinates(w->nodes[n].lat, w->nodes[n].lon));
    }
  }
  delete ways;
}

void OsmWidget::addPointLayer(QList<QPointF> *points) {
  pointLayers.append(points);
  update();
}

void OsmWidget::enableCursorInfo(int flags) {
  setMouseTracking(flags);
  tipFlags = flags;
}

void OsmWidget::wheelEvent(QWheelEvent *event) {
  int numDegrees = event->delta() / 8;
  int numSteps = numDegrees / 15;
  int z;
  
  if(event->orientation() == Qt::Vertical) {
    z = qMin(20, qMax(0, zoom + numSteps));
    setZoom(z);
  }
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