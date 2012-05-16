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
      query.prepare("CREATE TABLE nodes (id INTEGER PRIMARY KEY, lat INTEGER, lon INTEGER)");
      warn_query(&query);
//       query.finish();
//       if(!db.commit()) {
//         std::cout << "well that didn't work" << std::endl;
//         std::cout << query.lastError().text().toStdString() << std::endl;
//       }
      query.prepare("CREATE TABLE ways (wid INTEGER PRIMARY KEY, minlat INTEGER, minlon INTEGER, maxlat INTEGER, maxlon INTEGER)");
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

bool OsmDataSource::cacheTile(int lon, int lat) {
//   std::cout << "cacheTile" << std::endl;
  QSqlQuery query(db);
  
  
  query.prepare("SELECT latid FROM cache_contents WHERE latid=:latid AND lonid=:lonid");
  query.bindValue(":latid", lat);
  query.bindValue(":lonid", lon);
  warn_query(&query);
  
  if(!query.next()) {
    // the tile isn't cached.  Fetch it
    query.prepare("SELECT latid FROM cache_contents WHERE status=1");
    warn_query(&query);
    if(query.next())
      return false;  // only send one request to the server at a time
//     std::cout << "Caching tile (" << lat << ", " << lon << ")" << std::endl;
    query.prepare("INSERT INTO cache_contents (latid, lonid, status) VALUES (:latid, :lonid, :status)");
    query.bindValue(":latid", lat);
    query.bindValue(":lonid", lon);
    query.bindValue(":status", STATUS_PENDING);
    warn_query(&query);
    fetchData(lat/5.0-latStep/2, lon/5.0-lonStep/2, lat/5.0+latStep/2, lon/5.0+lonStep/2);
  }
  return true;
}

void OsmDataSource::fetchData(double minlat, double minlon, double maxlat, double maxlon) {
//   std::cout << "fetchData" << std::endl;

  QString url = QString("http://www.overpass-api.de/api/xapi?map?bbox=%1,%2,%3,%4")
                        .arg(minlon).arg(minlat).arg(maxlon).arg(maxlat);  //-2.4,51.3,-2.2,51.5
//   std::cout << "network request" << std::endl;
//   QNetworkRequest request = ;
  std::cout << "Requesting " << url.toStdString() << std::endl;
  net->get(QNetworkRequest(QUrl(url))); //"http://www.overpass-api.de/api/xapi?map?bbox=-2.4,51.3,-2.2,51.5")));
}

void OsmDataSource::parseData(QNetworkReply *reply) {
  QSqlQuery query(db);
  QString url = reply->url().toString();
  QStringList bbox;
  if(url.lastIndexOf("?bbox") > -1) {
    bbox = url.mid(url.lastIndexOf("?bbox") + 6).split(",");
  
    if(reply->error() == QNetworkReply::NoError) {
      // need to figure out what area this data corresponds to
      double minlon = bbox[0].toDouble();
      double minlat = bbox[1].toDouble();
//       std::cout << minlat << " " << minlon << std::endl;
//       std::cout << (minlat + latStep/2.0) << " " << (minlon + lonStep/2.0) << std::endl;
//       std::cout << (minlat + latStep/2.0)*5.0 << " " << (minlon + lonStep/2.0) * 5.0 << std::endl;
//       std::cout << (int)round((minlat + latStep/2.0)*5.0) << " " << (int)round((minlon + lonStep/2.0) * 5.0) << std::endl;
      
      
      query.prepare("UPDATE cache_contents SET status=:status WHERE latid=:latid AND lonid=:lonid");
      query.bindValue(":status", STATUS_FETCHED);
      query.bindValue(":latid", (int)round((minlat + latStep/2.0) * 5));
      query.bindValue(":lonid", (int)round((minlon + lonStep/2.0) * 5));
      warn_query(&query);
//       std::cout << "latid: " << (int)round((minlat + latStep/2.0) * 5) << " lonid: " << (int)round((minlon + lonStep/2.0) * 5) << std::endl;
//       query.prepare("SELECT latid, lonid, status FROM cache_contents");
//       warn_query(&query);
//       while(query.next()) {
//         std::cout << "latid: " << query.value(0).toInt() << " lonid: " << query.value(1).toInt() << " status: " << query.value(2).toInt() << std::endl;
//       }
      
      std::cout << "Received XML for " << reply->url().toString().toStdString() << std::endl;
      
      OsmParser *parser = new OsmParser(&db);
      QXmlInputSource *source = new QXmlInputSource(reply);
      QXmlSimpleReader *reader = new QXmlSimpleReader;
      reader->setContentHandler(parser);
      
      reader->parse(source);
      
      delete reader;
      delete source;
      delete parser;
      
      
      std::cout << "Parsed XML for " << reply->url().toString().toStdString() << std::endl;
    } else {
      std::cout << "Request for " << reply->url().toString().toStdString() << " Failed" << std::endl;
      std::cout << "  " << reply->error() << ": " << reply->errorString().toStdString() << std::endl;
      std::cout << "  Reply length: " << reply->size() << " bytes" << std::endl;
      
      double minlon = bbox[0].toDouble();
      double minlat = bbox[1].toDouble();
      
      query.prepare("DELETE FROM cache_contents WHERE latid=:latid AND lonid=:lonid");
      query.bindValue(":latid", (int)round((minlat + latStep/2)*5));
      query.bindValue(":lonid", (int)round((minlon + lonStep/2)*5));
      warn_query(&query);
    }
  } else {
    std::cout << "Error handling URL, couldn't determine bounding box" << std::endl;
  }
}

void OsmDataSource::selectArea(double minlati, double minloni, double maxlati, double maxloni) {
  maxlat = round(maxlati * 1e6);
  maxlon = round(maxloni * 1e6);
  minlat = round(minlati * 1e6);
  minlon = round(minloni * 1e6);
  
//   std::cout << minlat << ", " << minlon << ", " << maxlat << ", " << maxlon << std::endl;
  
  QSqlQuery query(db);
  
  query.prepare("SELECT wid FROM ways WHERE minlon<:maxlon AND maxlon>:minlon AND minlat<:maxlat AND maxlat>:minlat");
  query.bindValue(":minlon", minlon);
  query.bindValue(":maxlon", maxlon);
  query.bindValue(":minlat", minlat);
  query.bindValue(":maxlat", maxlat);
  warn_query(&query);
  
  if(query.last()) {
    std::cout << "Total of " << query.at()+1 << " ways cross the selected area" << std::endl;
  } else {
    std::cout << "Total of " << 0 << " ways cross the selected area" << std::endl;
  }
  
}

QVector<Way> *OsmDataSource::getWays(QString byTag, QString value) {
  QSqlQuery query(db);
  QVector<unsigned long long> wayids;
  QVector<Way> *ways = new QVector<Way>;
  int w;
  
  std::cout << "getWays(" << byTag.toStdString() << ", " << value.toStdString() << ");" << std::endl;
  
  query.prepare("SELECT kid FROM wayKeys WHERE name=:name");
  query.bindValue(":name", byTag);
  warn_query(&query);
  if(!query.next()) {
    std::cout << "no ways with " << byTag.toStdString() << "=" << value.toStdString() << " found" << std::endl;
    return ways;
  }
  unsigned long long kid = query.value(0).toULongLong();
  query.prepare("SELECT wid FROM wayTags WHERE kid=:kid AND value=:value");
  query.bindValue(":kid", kid);
  query.bindValue(":value", value);
  warn_query(&query);
  if(query.last()) {
    wayids.resize(query.at()+1);
    query.first();
    w = 0;
    do {
      wayids[w++] = query.value(0).toULongLong();
    } while(query.next());
  } else {
    std::cout << "no ways found :(" << std::endl;
    wayids.clear();
  }
  
  w = 0;
  ways->resize(wayids.size());
  for(int i=0;i<wayids.size();i++) {
    query.prepare("SELECT w.nid, n.lat, n.lon FROM wayNodes w INNER JOIN nodes n ON w.nid=n.id INNER JOIN ways wa ON w.wid=wa.wid WHERE w.wid=:wid AND wa.maxlat>:minlat AND wa.minlat<:maxlat AND wa.minlon<:maxlon AND wa.maxlon>:minlon ORDER BY w.weight ASC");
    query.bindValue(":wid", wayids[i]);
    query.bindValue(":minlat", minlat);
    query.bindValue(":maxlat", maxlat);
    query.bindValue(":minlon", minlon);
    query.bindValue(":maxlon", maxlon);
    warn_query(&query);
    if(query.last()) {
      (*ways)[w].nodes.resize(query.at()+1);
      query.first();
      int n = 0;
      do {
        (*ways)[w].nodes[n].id = query.value(0).toULongLong();
        (*ways)[w].nodes[n].lat = query.value(1).toInt() / 1e6;
        (*ways)[w].nodes[n++].lon = query.value(2).toInt() / 1e6;
      } while(query.next());
      w++;
    }
  }
  ways->resize(w);
  return ways;
}

OsmDataSource::~OsmDataSource() {
  if(db.isOpen())
    db.close();
}

/**
 * OsmParser - an XML parser for OSM data
 **/
OsmParser::OsmParser(QSqlDatabase *pdb) : QXmlDefaultHandler() {
  db = pdb;
}

bool OsmParser::startDocument() {
  inMarkup = false;
//   wayTagCount = 0;
  wayTagKeyCount = 0;
  newWays.clear();
  db->transaction();
  
  std::cout << "DB transaction..." << std::endl;
  
  QSqlQuery query(*db);
  // make a hash table of all the node key to integer values to save time querying the database
  // and the local hash because the query won't show new keys until commit() and running that for
  // every record makes this painfully slow.
  query.prepare("SELECT kid, name FROM wayKeys");
  warn_query(&query);
  wayTagNames.clear();
  while(query.next()) {
    wayTagNames.insert(query.value(1).toString(), query.value(1).toULongLong());
    wayTagKeyCount = qMax(wayTagKeyCount, query.value(1).toULongLong());
  }
  
  query.prepare("SELECT tid FROM wayTags");
  warn_query(&query);
  if(query.last()) {
    wayTagCount = query.at();
  } else {
    wayTagCount = 0;
  }
  
  // in theory it's quicker to drop the index and then re-populate??
  query.prepare("DROP INDEX IF EXISTS wayIndex");
  warn_query(&query);
  query.prepare("DROP INDEX IF EXISTS latind");
  warn_query(&query);
  query.prepare("DROP INDEX IF EXISTS lonind");
  warn_query(&query);
  
  return true;
}

bool OsmParser::startElement(const QString &, const QString &, const QString &name, const QXmlAttributes &attrs) {
//   std::cout << "Start element " << name.toStdString() << std::endl;
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
      
      QSqlQuery query;
      query.prepare("SELECT * FROM nodes WHERE id=:id");
      query.bindValue(":id", id);
      warn_query(&query);
      if(query.next()) {
        query.prepare("UPDATE nodes SET lat=:lat, lon=:lon WHERE id=:id");
        query.bindValue(":lat", (int)round(lat*1e6));
        query.bindValue(":lon", (int)round(lon*1e6));
        query.bindValue(":id", id);
        warn_query(&query);
      } else {
        query.prepare("INSERT INTO nodes (id, lat, lon) VALUES (:id, :lat, :lon)");
        query.bindValue(":id", id);
        query.bindValue(":lat", (int)round(lat*1e6));
        query.bindValue(":lon", (int)round(lon*1e6));
        warn_query(&query);
      }
    } else if(name == "way") {
      wayNodeOrder = 0;
      
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "id")
          currentWay = attrs.value(i).toLong();
      }
      
      QSqlQuery query(*db);
      
      query.prepare("SELECT wid FROM ways WHERE wid=:wid");
      query.bindValue(":wid", currentWay);
      warn_query(&query);
      if(!query.next()) {
        // only save this way if it's a new one, might over-lap from a neighbouring tile we have
        // already fetched.
        
        // keep track of ways we've added this time and we'll go through and generate the min and
        // max lat and lon after the node changes have been committed
        newWays.append(currentWay);
      
        inWay = true;
      }
    } else if(name == "nd" && inWay) {
      unsigned long long ref = -1;
      for(int i=0;i<attrs.count();i++) {
        if(attrs.localName(i) == "ref")
          ref = attrs.value(i).toLong();
      }
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
//     db->commit();
  } else if(name == "way") {
    inWay = false;
  }
  return true;
}

bool OsmParser::endDocument() {
  QSqlQuery query(*db);
  db->commit();
  std::cout << "Commit!" << std::endl;
  std::cout << "Make indices" << std::endl;
  query.prepare("CREATE INDEX wayIndex ON wayNodes (wid ASC, weight ASC)");
  warn_query(&query);
  
  query.prepare("CREATE INDEX latind ON nodes (lat)");
  warn_query(&query);
  
  query.prepare("CREATE INDEX lonind ON nodes (lon)");
  warn_query(&query);
  
  db->transaction();
  std::cout << "Getting bounds of " << newWays.size() << " new ways" << std::endl;
  for(int i=0;i<newWays.size();i++) {
    int wayMinLon, wayMinLat, wayMaxLon, wayMaxLat;
    query.prepare("SELECT MAX(n.lat) FROM nodes n INNER JOIN wayNodes w ON w.nid=n.id WHERE w.wid=:wid");
    query.bindValue(":wid", newWays[i]);
    warn_query(&query);
    query.next();
    wayMaxLat = query.value(0).toInt();
    
    query.prepare("SELECT MIN(n.lat) FROM nodes n INNER JOIN wayNodes w ON w.nid=n.id WHERE w.wid=:wid");
    query.bindValue(":wid", newWays[i]);
    warn_query(&query);
    query.next();
    wayMinLat = query.value(0).toInt();
    
    query.prepare("SELECT MAX(n.lon) FROM nodes n INNER JOIN wayNodes w ON w.nid=n.id WHERE w.wid=:wid");
    query.bindValue(":wid", newWays[i]);
    warn_query(&query);
    query.next();
    wayMaxLon = query.value(0).toInt();
    
    query.prepare("SELECT MIN(n.lon) FROM nodes n INNER JOIN wayNodes w ON w.nid=n.id WHERE w.wid=:wid");
    query.bindValue(":wid", newWays[i]);
    warn_query(&query);
    query.next();
    wayMinLon = query.value(0).toInt();
    
    query.prepare("INSERT INTO ways (wid, minlat, minlon, maxlat, maxlon) VALUES (:wid, :minlat, :minlon, :maxlat, :maxlon)");
    query.bindValue(":wid", newWays[i]);
    query.bindValue(":minlat", wayMinLat);
    query.bindValue(":minlon", wayMinLon);
    query.bindValue(":maxlat", wayMaxLat);
    query.bindValue(":maxlon", wayMaxLon);
    warn_query(&query);
  }
  db->commit();
  std::cout << "Last commit!" << std::endl;
  return true;
}
