#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QVariant>
#include <QFile>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
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
  db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName("osm.local");
  bool ok = db.open();
  
//   std::cout << db.driver()->hasFeature(QSqlDriver::Transactions) << std::endl;
  
  if(ok) {
    QSqlQuery query;
    std::cout << "opened SQLite database okay" << std::endl;
    query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='nodes'");
//     std::cout << query.lastError().text().toStdString() << std::endl;
    if(!query.next()) {
      // it's an empty (i.e. new) cache, make the tables
//       std::cout << query.isActive() << " " << query.isValid() << std::endl;
//       query.finish();
//       std::cout << query.isActive() << std::endl;
      std::cout << "initialising a new cache in osm.local" << std::endl;
      query.exec("CREATE TABLE nodes (id INTEGER PRIMARY KEY, lat REAL, lon REAL)");
//       query.finish();
//       if(!db.commit()) {
//         std::cout << "well that didn't work" << std::endl;
//         std::cout << query.lastError().text().toStdString() << std::endl;
//       }
      query.exec("CREATE TABLE ways (id INTEGER PRIMARY KEY, node INTEGER, order INTEGER)");
//       query.finish();
//       if(!db.commit()) {
//         std::cout << "that didn't either" << std::endl;
//       }
    }
    query.exec("SELECT name FROM sqlite_master");
//     std::cout << query.next() << std::endl;
  } else {
    std::cout << "failed to open SQLite database" << std::endl;
  }
}

void OsmDataSource::fetchData() {
  OsmParser *parser = new OsmParser(&db);
  
  QFile *file = new QFile("bath_osm.xml");
  QXmlInputSource *source = new QXmlInputSource(file);
  
  QXmlSimpleReader *reader = new QXmlSimpleReader;
  reader->setContentHandler(parser);
  
  reader->parse(source);
  
  delete reader;
  delete source;
  delete file;
  delete parser;
}

OsmDataSource::~OsmDataSource() {
  if(db.isOpen())
    db.close();
}

OsmParser::OsmParser(QSqlDatabase *pdb) : QXmlDefaultHandler() {
  db = pdb;
}

bool OsmParser::startDocument() {
  inMarkup = false;
//   nodes.clear();
  db->transaction();
  return true;
}

bool OsmParser::startElement(const QString &, const QString &, const QString &name, const QXmlAttributes &attrs) {
  if(inMarkup) {
    if(name == "node") {
      double lat = NAN, lon = NAN;
      unsigned long long id = -1;
    
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "id")
          id = attrs.value(i).toLong();
        if(attrs.localName(i) == "lat")
          lat = attrs.value(i).toDouble();
        if(attrs.localName(i) == "lon")
          lon = attrs.value(i).toDouble();
      }
//     std::cout << "Node [" << id << "] = (" << lat << ", " << lon << ")" << std::endl;
//       nodes.append(Node(id, lat, lon));
      QSqlQuery query;
      query.prepare("SELECT * FROM nodes WHERE id=:id");
      query.bindValue(":id", id);
      query.exec();
      if(query.next()) {
        query.prepare("UPDATE nodes SET lat=:lat, lon=:lon WHERE id=:id");
        query.bindValue(":lat", lat);
        query.bindValue(":lon", lon);
        query.bindValue(":id", id);
        query.exec();
      } else {
        query.prepare("INSERT INTO nodes (id, lat, lon) VALUES (:id, :lat, :lon)");
        query.bindValue(":id", id);
        query.bindValue(":lat", lat);
        query.bindValue(":lon", lon);
        query.exec();
      }
    } else if(name == "way") {
//       unsigned long id = -1;
      wayNodeOrder = 0;
      
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "id")
          currentWay = attrs.value(i).toLong();
      }
      
//       ways.append(Way(id));
      inWay = true;
    } else if(name == "nd" && inWay) {
      unsigned long long ref = -1;
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "ref")
          ref = attrs.value(i).toLong();
      }
//       ways.last().nodes.append(ref);
      QSqlQuery query;
      query.prepare("SELECT * FROM ways WHERE id=:id AND node=:node");
      query.bindValue(":id", currentWay);
      query.bindValue(":node", ref);
      query.exec();
      if(query.next()) {
        query.prepare("UPDATE ways SET order=:order WHERE id=:id AND node=:node");
        query.bindValue(":order", wayNodeOrder);
        query.bindValue(":id", currentWay);
        query.bindValue(":node", ref);
        query.exec();
      } else {
        query.prepare("INSERT INTO ways (id, node, order) VALUES (:id, :node, :order)");
        query.bindValue(":id", currentWay);
        query.bindValue(":node", ref);
        query.bindValue(":order", wayNodeOrder);
        query.exec();
      }
      wayNodeOrder++;
    }
    
  } else if(name == "osm") {
    inMarkup = true;
  }
  
  return true;
}

bool OsmParser::endElement(const QString&, const QString &, const QString &name) {
  if(name == "osm") {
    inMarkup = false;
    db->commit();
  } else if(name == "way") {
    inWay = false;
  }
  return true;
}

int OsmParser::node_count() {
//   return nodes.size();
  return 0;
}

Node *OsmParser::node(int indx) {
//   return &nodes[indx];
  return new Node(0,0,0);
}