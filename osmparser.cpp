#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <cmath>
#include <iostream>

#include "osmparser.hpp"

Node::Node(long idin, double latin, double lonin) {
  id = idin;
  lat = latin;
  lon = lonin;
}

Way::Way(long idin) {
  id = idin;
  nodes.clear();
}

OsmDataSource::OsmDataSource() {
  QSqlQuery query;
  db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName("osm.local");
  bool ok = db.open();
  
  std::cout << db.driver()->hasFeature(QSqlDriver::Transactions) << std::endl;
  
  if(ok) {
    std::cout << "opened SQLite database okay" << std::endl;
    query = db.exec(QString("SELECT name FROM sqlite_master WHERE type='table' AND name='nodes';"));
    if(query.size() == -1) {
      // it's an empty (i.e. new) cache, make the tables
      std::cout << query.isActive() << " " << query.isValid() << std::endl;
      query.finish();
      std::cout << query.isActive() << std::endl;
      std::cout << "initialising a new cache in osm.local" << std::endl;
      query = db.exec(QString("CREATE TABLE nodes (id INTEGER PRIMARY KEY, lat REAL, lon REAL)"));
      if(!db.commit()) {
        std::cout <<"well that didn't work" << std::endl;
        std::cout << query.lastError().text().toStdString() << std::endl;
      }
      db.exec(QString("CREATE TABLE ways (id INTEGER PRIMARY KEY, node INTEGER, order INTEGER)"));
      if(!db.commit()) {
        std::cout << "that didn't either" << std::endl;
      }
    }
    query = db.exec(QString("SELECT name FROM sqlite_master"));
    std::cout << query.size() << std::endl;
  } else {
    std::cout << "failed to open SQLite database" << std::endl;
  }
}

OsmDataSource::~OsmDataSource() {
  if(db.isOpen())
    db.close();
}

bool OsmParser::startDocument() {
  inMarkup = false;
  nodes.clear();
  return true;
}

bool OsmParser::startElement(const QString &, const QString &, const QString &name, const QXmlAttributes &attrs) {
  if(inMarkup) {
    if(name == "node") {
      double lat = NAN, lon = NAN;
      unsigned long id = -1;
    
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "id")
          id = attrs.value(i).toLong();
        if(attrs.localName(i) == "lat")
          lat = attrs.value(i).toDouble();
        if(attrs.localName(i) == "lon")
          lon = attrs.value(i).toDouble();
      }
//     std::cout << "Node [" << id << "] = (" << lat << ", " << lon << ")" << std::endl;
      nodes.append(Node(id, lat, lon));
    } else if(name == "way") {
      unsigned long id = -1;
      
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "id")
          id = attrs.value(i).toLong();
      }
      
      ways.append(Way(id));
      inWay = true;
    } else if(name == "nd" && inWay) {
      unsigned long ref = -1;
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "ref")
          ref = attrs.value(i).toLong();
      }
      ways.last().nodes.append(ref);
    }
    
  } else if(name == "osm") {
    inMarkup = true;
  }
  
  return true;
}

bool OsmParser::endElement(const QString&, const QString &, const QString &name) {
  if(name == "osm") {
    inMarkup = false;
  } else if(name == "way") {
    inWay = false;
  }
  return true;
}

int OsmParser::node_count() {
  return nodes.size();
}

Node *OsmParser::node(int indx) {
  return &nodes[indx];
}