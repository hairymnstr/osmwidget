#ifndef OSM_PARSER_H
#define OSM_PARSER_H

#include <QObject>
#include <QXmlDefaultHandler>
#include <QSqlDatabase>
#include <QHash>
#include <QList>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#define STATUS_PENDING 1
#define STATUS_FETCHED 2
// class Node {
//   public:
//     unsigned long long id;
//     double lat;
//     double lon;
//     Node(unsigned long long, double, double);
//     Node();
// };

typedef struct {
  unsigned long long id;
  double lat;
  double lon;
} Node;

class Way {
  public:
    unsigned long long id;
    QVector<Node> nodes;
    Way(unsigned long long);
    Way();
};

class OsmDataSource : public QObject {
  Q_OBJECT
  
  public:
    OsmDataSource();
    ~OsmDataSource();
    void fetchData(double, double, double, double);
    QVector<Way> *getWays(QString, QString);
    bool cacheTile(int,int);
    
  public slots:
    void parseData(QNetworkReply *);
    
  private:
    QSqlDatabase db;
    QNetworkAccessManager *net;
    double latStep;
    double lonStep;
};

// class XapiFetcher : public QObject {
//   Q_OBJECT
//   
//   public:
//     XapiFetcher();
//     void fetch();
//     void busy();
//     
//   public slots:
//     void replyFinished(QNetworkReply *);
//     
//   private:
//     QNetworkAccessManager *net;
//     bool fetching;
// };

class OsmParser : public QXmlDefaultHandler {
  public:
    OsmParser(QSqlDatabase *);
    bool startDocument();
    bool endElement(const QString&, const QString&, const QString &name);
    bool startElement(const QString&, const QString&, const QString &name, const QXmlAttributes &attrs);
    
  private:
    bool inMarkup;
    bool inWay;
    unsigned long long currentWay;
    unsigned int wayNodeOrder;
//     QList<Node> nodes;
//     QList<Way> ways;
    unsigned long long wayTagCount;
    unsigned long long wayTagKeyCount;
    QHash<QString, unsigned long long> wayTagNames;
    QSqlDatabase *db;
};

#endif // ifndef OSM_PARSER_H