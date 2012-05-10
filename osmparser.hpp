#ifndef OSM_PARSER_H
#define OSM_PARSER_H

#include <QXmlDefaultHandler>
#include <QSqlDatabase>
#include <QHash>
#include <QList>
#include <QVector>

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

class OsmDataSource {
  public:
    OsmDataSource();
    ~OsmDataSource();
    void fetchData();
    int selectArea(double, double, double, double);
    int listWayTags();
    QVector<Way> *getWays(QString, QString);
    
  private:
    QSqlDatabase db;
};

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