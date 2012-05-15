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
#include <QNetworkRequest>
#include <QUrl>
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

// Node::Node(unsigned long long idin, double latin, double lonin) {
//   id = idin;
//   lat = latin;
//   lon = lonin;
// }
// 
// Node::Node() {
//   id = 0;
//   lat = 0;
//   lon = 0;
// }

Way::Way(unsigned long long idin) {
  id = idin;
//   nodes.clear();
}

Way::Way() {
  id = 0;
//   nodes.clear();
}

OsmDataSource::OsmDataSource() {
  latStep = 0.2;
  lonStep = 0.2;
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
      query.prepare("CREATE TABLE cache_contents (latid INTEGER, lonid INTEGER, status INTEGER, PRIMARY KEY(latid, lonid))");
      warn_query(&query);
    }
    query.exec("SELECT * FROM nodes");
    if(!query.last()) {
      std::cout << "Database contains 0 nodes" << std::endl;
    } else {
      std::cout << "Database contains " << query.at() << " nodes" << std::endl;
    }
//     std::cout << query.next() << std::endl;
  
  } else {
    std::cout << "failed to open SQLite database" << std::endl;
  }
  
  net = new QNetworkAccessManager(this);
  
//   std::cout << "Things: " << net << std::endl;
  
  connect(net, SIGNAL(finished(QNetworkReply *)), this, SLOT(parseData(QNetworkReply *)));
//   std::cout << this << std::endl;
}

void OsmDataSource::fetchData(double minlat, double minlon, double maxlat, double maxlon) {
  std::cout << "fetchData" << std::endl;
//   std::cout << this << std::endl;
//   QNetworkAccessManager *  net = new QNetworkAccessManager();
//   connect(net, SIGNAL(finished(QNetworkReply *)), this, SLOT(parseData(QNetworkReply *)));

  QString url = QString("http://www.overpass-api.de/api/xapi?map?bbox=%1,%2,%3,%4")
                        .arg(minlon).arg(minlat).arg(maxlon).arg(maxlat);  //-2.4,51.3,-2.2,51.5
//   std::cout << "network request" << std::endl;
//   QNetworkRequest request = ;
  net->get(QNetworkRequest(QUrl(url))); //"http://www.overpass-api.de/api/xapi?map?bbox=-2.4,51.3,-2.2,51.5")));
}

void OsmDataSource::parseData(QNetworkReply *reply) {
  QSqlQuery query(db);
  
  // need to figure out what area this data corresponds to
  if(reply->url().hasQueryItem("bbox")) {
    QStringList bbox = reply->url().queryItemValue("bbox").split(",");
    double minlon = bbox[0].toDouble();
    double minlat = bbox[1].toDouble();
//     double maxlon = bbox[2].toDouble();
//     double maxlat = bbox[3].toDouble();
    
    query.prepare("UPDATE cache_contents SET status=:status WHERE latid=:latid AND lonid=:lonid");
    query.bindValue(":status", STATUS_FETCHED);
    query.bindValue(":latid", (int)((minlon + lonStep/2) * 5));
    query.bindValue(":lonid", (int)((minlat + latStep/2) * 5));
    warn_query(&query);
    
//     std::cout << reply->url().queryItemValue("bbox").toStdString() << std::endl;
  }
  
  // in theory it's quicker to drop the index and then re-populate??
  query.prepare("DROP INDEX IF EXISTS wayIndex");
  warn_query(&query);
  
  OsmParser *parser = new OsmParser(&db);
  QXmlInputSource *source = new QXmlInputSource(reply);
  QXmlSimpleReader *reader = new QXmlSimpleReader;
  reader->setContentHandler(parser);
  
  reader->parse(source);
  
  delete reader;
  delete source;
//   delete file;
  delete parser;
  
  query.prepare("CREATE INDEX wayIndex ON wayNodes (wid ASC, weight ASC)");
  warn_query(&query);
  
}

QVector<Way> *OsmDataSource::getWays(QString byTag, QString value) {
//   db.transaction();
  QSqlQuery query(db);
  QVector<Way> *ways = new QVector<Way>;
  
//   std::cout << "Finding tagged ways" << std::endl;
  query.prepare("SELECT kid FROM wayKeys WHERE name=:name");
  query.bindValue(":name", byTag);
  warn_query(&query);
  query.next();
  unsigned long long kid = query.value(0).toULongLong();
  query.prepare("SELECT wid FROM wayTags WHERE kid=:kid AND value=:value");
  query.bindValue(":kid", kid);
  query.bindValue(":value", value);
  warn_query(&query);
  if(query.last()) {
    ways->resize(query.at()+1);
    query.first();
    int w = 0;
    do {
//         std::cout << query.value(0).toULongLong() << std::endl;
      (*ways)[w++] = Way(query.value(0).toULongLong());
    } while(query.next());
  } else {
    std::cout << "no ways found :(" << std::endl;
    ways->clear();
  }
  
//   std::cout << "Fetching nodes of these ways" << std::endl;
  for(QVector<Way>::iterator w=ways->begin();w!=ways->end();++w) {
    query.prepare("SELECT w.nid, n.lat, n.lon FROM wayNodes w INNER JOIN nodes n ON w.nid=n.id WHERE w.wid=:wid ORDER BY w.weight ASC");
    query.bindValue(":wid", w->id);
    warn_query(&query);
    if(query.last()) {
      w->nodes.resize(query.at()+1);
      query.first();
      int n = 0;
      do {
        w->nodes[n].id = query.value(0).toULongLong();
        w->nodes[n].lat = query.value(1).toDouble();
        w->nodes[n++].lon = query.value(2).toDouble();
      } while(query.next());
    } else {
      w->nodes.clear();
    }
  }
//   std::cout << "done" << std::endl;
//   db.commit();
  return ways;
}

bool OsmDataSource::cacheTile(int lon, int lat) {
  std::cout << "cacheTile" << std::endl;
  QSqlQuery query(db);
  
  std::cout << "Caching tile (" << lat << ", " << lon << ")" << std::endl;
  
  query.prepare("SELECT latid FROM cache_contents WHERE latid=:latid AND lonid=:lonid");
  query.bindValue(":latid", lat);
  query.bindValue(":lonid", lon);
  warn_query(&query);
  
  if(!query.next()) {
    // the tile isn't cached.  Fetch it
    query.prepare("INSERT INTO cache_contents (latid, lonid, status) VALUES (:latid, :lonid, :status)");
    query.bindValue(":latid", lat);
    query.bindValue(":lonid", lon);
    query.bindValue(":status", STATUS_PENDING);
    warn_query(&query);
    fetchData(lat/5.0-0.2, lon/5.0-0.2, lat/5.0+0.2, lon/5.0+0.2);
  }
  return true;
}

OsmDataSource::~OsmDataSource() {
  if(db.isOpen())
    db.close();
}

/**
 * XapiFetcher - a wrapper for accessing OSM extended API
 */
// XapiFetcher::XapiFetcher() {
//   net = new QNetworkAccessManager(this);
//   busy = false;
//   connect(net, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinished(QNetworkReply *)));
// }
// 
// XapiFetcher::fetch(double minlat, double minlon, double maxlat, double maxlon) {
//   QString url = QString("http://www.overpass-api.de/api/xapi?map?bbox=%1,%2,%3,%4")
//                         .arg(minlon).arg(minlat).arg(maxlon).arg(maxlat);  //-2.4,51.3,-2.2,51.5
//   busy = true;
//   net->get(QNetworkRequest(QUrl(url)));
// }
// 
// XapiFetcher::replyFinished(QNetworkReply *xapiReply) {
//   QByteArray data = xapiReply->readAll();
//   QString str(data);
//   
//   busy = false;
// }


/**
 * OsmParser - an XML parser for OSM data
 **/
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
