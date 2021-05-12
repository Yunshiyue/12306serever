#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[])
{
    //https://www.dushibaiyu.com/2013/12/qtcpserver%E5%A4%9A%E7%BA%BF%E7%A8%8B%E5%AE%9E%E7%8E%B0.html
    QCoreApplication a(argc, argv);
    QString datetime_now = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    qDebug() << datetime_now;
    Server server;
    server.listen(QHostAddress::Any, 3333);

    return a.exec();
}
