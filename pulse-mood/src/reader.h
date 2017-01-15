#ifndef READER_H
#define READER_H

#include <QObject>
#include <QString>
#include <iostream>

class Reader : public QObject {
    Q_OBJECT
 
public:
    Reader();
    ~Reader();
 
public slots:
    void read();
 
signals:
    void data(QString data);
    void finished();
    void error(QString err);
 
private:
    // add your variables here
};

#endif // READER_H
