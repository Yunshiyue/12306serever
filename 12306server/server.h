#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSqlDatabase>
#include <QSqlDriver>
#include "serverthread.h"

class Server : public QTcpServer
{
    Q_OBJECT

public:
    Server(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;
private:
    QSqlDatabase db;
};

#endif // SERVER_H
