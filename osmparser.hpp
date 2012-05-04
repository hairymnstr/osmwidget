#ifndef OSM_PARSER_H
#define OSM_PARSER_H

#include <QXmlDefaultHandler>
#include <QSqlDatabase>
#include <QList>

class Node {
  public:
    long id;
    double lat;
    double lon;
    Node(long, double, double);
};

class Way {
  public:
    long id;
    QList<unsigned long> nodes;
    Way(long);
};

class OsmDataSource {
  public:
    OsmDataSource();
    ~OsmDataSource();
    
  private:
    QSqlDatabase db;
};

class OsmParser : public QXmlDefaultHandler {
  public:
    bool startDocument();
    bool endElement(const QString&, const QString&, const QString &name);
    bool startElement(const QString&, const QString&, const QString &name, const QXmlAttributes &attrs);
    int node_count();
    Node *node(int);
    
  private:
    bool inMarkup;
    bool inWay;
    QList<Node> nodes;
    QList<Way> ways;
};

#endif // ifndef OSM_PARSER_H