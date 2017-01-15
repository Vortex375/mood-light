#include "reader.h"

#include <QDebug>

Reader::Reader::Reader()
{
    
}

Reader::Reader::~Reader()
{
    
}

void Reader::read()
{
    //qDebug() << "reading from stdin";
    while(std::cin) {
        std::string str;
        std::getline(std::cin, str);
        
        //qDebug() << "read:" << QString::fromLocal8Bit(str.c_str());
        
        emit data(QString::fromLocal8Bit(str.c_str()));
    }
    //qDebug() << "stdin EOF";
    emit finished();
}

#include "reader.moc"
