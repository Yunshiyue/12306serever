#include "server.h"

Server::Server(QObject *parent) : QTcpServer(parent)
{
    qDebug() << "服务器启动";
    db = QSqlDatabase::addDatabase("QPSQL");
    qDebug() << "加载驱动成功";
    db.setHostName("127.0.0.1");
    db.setPort(5433);
    db.setDatabaseName("12306");
    db.setUserName("postgres");
    db.setPassword("991019");
    if (!db.open())
        qWarning() << "Failed to connect to root postsql admin";


//    const char *database_info = "hostaddr=127.0.0.1 dbname=12306 user=postgres password=991019";
//    PGconn *conn = PQconnectdb(database_info);
//    if (PQstatus(conn) == CONNECTION_BAD)
//    {
//        qDebug() << "connect bad";
//    }
//    else if (PQstatus(conn) == CONNECTION_OK)
//    {
//        qDebug() << "connect ok";
//    }
}

/*
* 新连接到来，建立一个新线程
*/
void Server::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "新的连接";
    ServerThread* serverThread = new ServerThread(socketDescriptor, db, this);
    qDebug() << "建立线程";
    connect(serverThread, &ServerThread::finished, serverThread, &ServerThread::deleteLater);
    qDebug() << "准备开始";
    serverThread->start();
}
