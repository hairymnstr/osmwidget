#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QVariant>
#include <QFile>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QHash>
#include <QList>
#include <cmath>
#include <iostream>

#include "osmparser.hpp"

bool warn_query(QSqlQuery *query) {
  if(!query->exec()) {
    std::cout << "SQL: \"" << query->executedQuery().toStdString() << "\" Failed" << std::endl;
    std::cout << "  " << query->lastError().text().toStdString() << std::endl << std::endl;
    return false;
  }
  return true;
}

Node::Node(unsigned long long idin, double latin, double lonin) {
  id = idin;
  lat = latin;
  lon = lonin;
}

Way::Way(unsigned long long idin) {
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
      query.prepare("CREATE TABLE nodes (id INTEGER PRIMARY KEY, lat REAL, lon REAL)");
      warn_query(&query);
//       query.finish();
//       if(!db.commit()) {
//         std::cout << "well that didn't work" << std::endl;
//         std::cout << query.lastError().text().toStdString() << std::endl;
//       }
      query.prepare("CREATE TABLE ways (wid INTEGER PRIMARY KEY)");
      warn_query(&query);
      
      query.prepare("CREATE TABLE wayNodes (wid INTEGER, nid INTEGER, weight INTEGER)");
      warn_query(&query);
//       query.finish();
//       if(!db.commit()) {
//         std::cout << "that didn't either" << std::endl;
//       }
      query.prepare("CREATE TABLE wayTags (tid INTEGER PRIMARY KEY, wid INTEGER, kid INTEGER, value TEXT)");
      warn_query(&query);
      query.prepare("CREATE TABLE wayKeys (kid INTEGER PRIMARY KEY, name TEXT)");
      warn_query(&query);
    }
    query.exec("SELECT * FROM nodes");
    int i = 0;
    while(query.next())
      i++;
    std::cout << "Database contains " << i << " nodes" << std::endl;
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
  QSqlQuery query(db);
  
  query.prepare("CREATE INDEX wayIndex ON wayNodes (wid ASC, weight ASC)");
  warn_query(&query);
  
}

int OsmDataSource::selectArea(double minlat, double minlon, double maxlat, double maxlon) {
  std::cout << minlat << " < lat < " << maxlat << "  " << minlon << " < lon < " << maxlon << std::endl;
  QSqlQuery query(db);
  query.prepare("SELECT * FROM nodes WHERE lat>:minlat AND lat<:maxlat AND lon>:minlon AND lon<:maxlon");
  query.bindValue(":minlat", minlat);
  query.bindValue(":maxlat", maxlat);
  query.bindValue(":minlon", minlon);
  query.bindValue(":maxlon", maxlon);
  query.exec();
  int i=0;
  while(query.next())
    i++;
  return i;
}

int OsmDataSource::listWayTags() {
  QSqlQuery query(db);
  
  query.exec("SELECT kid FROM wayKeys WHERE name='highway'");
  unsigned long long kid = query.value(0).toULongLong();
  query.prepare("SELECT value FROM wayTags WHERE kid=:kid");
  query.bindValue(":kid", kid);
  warn_query(&query);
  while(query.next()) {
    std::cout << query.value(0).toString().toStdString() << std::endl;
  }
  return 1;
}

QList<Way> *OsmDataSource::getWays(QString byTag, QString value) {
  db.transaction();
  QSqlQuery query(db);
  QList<Way> *ways = new QList<Way>;
  
  std::cout << "Finding tagged ways" << std::endl;
  query.prepare("SELECT kid FROM wayKeys WHERE name=:name");
  query.bindValue(":name", byTag);
  warn_query(&query);
  query.next();
  unsigned long long kid = query.value(0).toULongLong();
  query.prepare("SELECT wid FROM wayTags WHERE kid=:kid AND value=:value");
  query.bindValue(":kid", kid);
  query.bindValue(":value", value);
  warn_query(&query);
  while(query.next()) {
//     std::cout << query.value(0).toULongLong() << std::endl;
    ways->append(Way(query.value(0).toULongLong()));
  }
  
  std::cout << "Fetching nodes of these ways" << std::endl;
  for(QList<Way>::iterator w=ways->begin();w!=ways->end();++w) {
    query.prepare("SELECT w.nid, n.lat, n.lon FROM wayNodes w INNER JOIN nodes n ON w.nid=n.id WHERE w.wid=:wid ORDER BY w.weight ASC");
    query.bindValue(":wid", w->id);
    warn_query(&query);
    while(query.next()) {
      w->nodes.append(Node(query.value(0).toULongLong(), query.value(1).toDouble(), query.value(2).toDouble()));
    }
  }
  std::cout << "done" << std::endl;
  db.commit();
  return ways;
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
  wayTagCount = 0;
  wayTagKeyCount = 0;
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
      warn_query(&query);
      if(query.next()) {
        query.prepare("UPDATE nodes SET lat=:lat, lon=:lon WHERE id=:id");
        query.bindValue(":lat", lat);
        query.bindValue(":lon", lon);
        query.bindValue(":id", id);
        warn_query(&query);
      } else {
        query.prepare("INSERT INTO nodes (id, lat, lon) VALUES (:id, :lat, :lon)");
        query.bindValue(":id", id);
        query.bindValue(":lat", lat);
        query.bindValue(":lon", lon);
        warn_query(&query);
//         std::cout << query.executedQuery().toStdString() << std::endl;
      }
    } else if(name == "way") {
//       unsigned long id = -1;
      wayNodeOrder = 0;
      
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "id")
          currentWay = attrs.value(i).toLong();
      }
      
      QSqlQuery query(*db);
      
      query.prepare("INSERT INTO ways (wid) VALUES (:wid)");
      query.bindValue(":wid", currentWay);
      warn_query(&query);
      
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
      
      query.prepare("INSERT INTO wayNodes (wid, nid, weight) VALUES (:wid, :nid, :weight)");
      query.bindValue(":wid", currentWay);
      query.bindValue(":nid", ref);
      query.bindValue(":weight", wayNodeOrder++);
      warn_query(&query);
    } else if(name == "tag") {
      QString key, val;
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "k")
          key = attrs.value(i);
        else if(attrs.localName(i) == "v")
          val = attrs.value(i);
      }
      
      if(inWay) {
        unsigned long long tagid = 0;
        QSqlQuery query(*db);
        if(wayTagNames.find(key) == wayTagNames.end()) {
          tagid = wayTagKeyCount;
          wayTagNames.insert(key, wayTagKeyCount++);
          query.prepare("INSERT INTO wayKeys (kid, name) VALUES (:keyid, :name)");
          query.bindValue(":keyid", tagid);
          query.bindValue(":name", key);
          warn_query(&query);
        } else {
          tagid = wayTagNames.find(key).value();
        }
        query.prepare("INSERT INTO wayTags (tid, wid, kid, value) VALUES (:tagid, :wayid, :keyid, :value)");
        query.bindValue(":tagid", wayTagCount++);
        query.bindValue(":wayid", currentWay);
        query.bindValue(":keyid", tagid);
        query.bindValue(":value", val);
        warn_query(&query);
      }
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