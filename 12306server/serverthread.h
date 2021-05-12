#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H

#include <iostream>
#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QByteArray>
#include <QDateTime>

enum operation { REGISTER, LOGIN, INFORMATION, UPDATE_INFORMATION, CHANGE_PASSWORD, LOAD_PASSENGER,
CHANGE_PASSENGER, DELETE_PASSENGER, ADD_PASSENGER, SEARCH_TRAIN, SEARCH_STATION, STATION_STATION, SUBMIT_ORDER,
PAY_ORDER, SEARCH_ORDER, ORDER_INFORMATION, CANCEL_ORDER, CANCEL_TICKET, PAY_TICKET, CHANGE_TICKET, TRANSFER
};

class ServerThread : public QThread
{
    Q_OBJECT

public:
    ServerThread(int socketDescriptor, QSqlDatabase db, QObject* parent);
    void run() override;

signals:
    void error(QTcpSocket::SocketError socketError);

private:
    QJsonObject TurnToFunction(QJsonObject);
    void sendData(QJsonObject, QTcpSocket*);
    QString GenerateByte(int first, int last, int total_num);
    QString GenerateOcupiedByte(int first, int last, int total_num);
    QJsonArray GetSeat(QJsonObject search_seat, int total_seat_num, QString seat_type, QString train_id, int departure_station, int arrival_station, int total_order, QString date);
    double GetPrice(QString seat_type, QString passenger_type);

    QJsonObject Register(QJsonObject);   //注册
    QJsonObject LogIn(QJsonObject);     //登录
    QJsonObject Information(QJsonObject);   //查看个人信息
    QJsonObject UpdateInformation(QJsonObject); //修改个人信息
    QJsonObject ChangePassword(QJsonObject);    //修改密码
    QJsonObject LoadPassenger(QJsonObject); //查看乘车人
    QJsonObject ChangePassenger(QJsonObject);   //修改乘车人
    QJsonObject DeletePassenger(QJsonObject);   //删除乘车人
    QJsonObject AddPassenger(QJsonObject);  //添加乘车人
    QJsonObject SearchTrain(QJsonObject);   //搜索车次
    QJsonObject SearchStation(QJsonObject); //搜索车站
    QJsonObject StationStation(QJsonObject);    //站站查询
    QJsonObject SubmitOrder(QJsonObject);   //生成订单
    QJsonObject PayOrder(QJsonObject);  //支付订单
    QJsonObject SearchOrder(QJsonObject);   //查询订单
    QJsonObject OrderInformation(QJsonObject);  //订单信息
    QJsonObject CancelOrder(QJsonObject);   //取消订单
    QJsonObject CancelTicket(QJsonObject);  //退票
    QJsonObject PayTicket(QJsonObject);     //支付票（改签后）
    QJsonObject ChangeTicket(QJsonObject);  //改签
    QJsonObject Transfer(QJsonObject);  //换乘

    int socketDescriptor;

    QSqlDatabase db;
};

#endif // SERVERTHREAD_H
