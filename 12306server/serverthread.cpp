#include "serverthread.h"


ServerThread::ServerThread(int socketDescriptor, QSqlDatabase db, QObject *parent) :
    QThread(parent), socketDescriptor(socketDescriptor), db(db)
{
    qDebug() << "线程构造函数";

}

//处理流程
void ServerThread::run()
{
    QTcpSocket* tcpSocket = new QTcpSocket();
    if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
        emit error(tcpSocket->error());
        return;
    }

    QByteArray get_data;
    if (tcpSocket->waitForReadyRead()) {

        //接受数据
        get_data = tcpSocket->readAll();
        QJsonObject get_json = QJsonDocument::fromJson(get_data).object();
        qDebug() << "服务器接收数据：";
        qDebug() << get_json;

        //解析数据
        sendData(TurnToFunction(get_json), tcpSocket);
    }
    //tcpSocket.disconnectFromHost();
    tcpSocket->waitForDisconnected();
    delete tcpSocket;
}

QJsonObject ServerThread::TurnToFunction(QJsonObject get_json)
{
    int operation_type = get_json.take("操作").toInt();
    switch (operation_type) {
    case REGISTER:
        return Register(get_json);

    case LOGIN:
        return LogIn(get_json);

    case INFORMATION:
        return Information(get_json);

    case UPDATE_INFORMATION:
        return UpdateInformation(get_json);
           
    case CHANGE_PASSWORD:
        return ChangePassword(get_json);

    case LOAD_PASSENGER:
        return LoadPassenger(get_json);

    case CHANGE_PASSENGER:
        return ChangePassenger(get_json);

    case DELETE_PASSENGER:
        return DeletePassenger(get_json);

    case ADD_PASSENGER:
        return AddPassenger(get_json);

    case SEARCH_TRAIN:
        return SearchTrain(get_json);

    case SEARCH_STATION:
        return SearchStation(get_json);

    case STATION_STATION:
        return StationStation(get_json);

    case SUBMIT_ORDER:
        return SubmitOrder(get_json);

    case PAY_ORDER:
        return PayOrder(get_json);

    case SEARCH_ORDER:
        return SearchOrder(get_json);

    case ORDER_INFORMATION:
        return OrderInformation(get_json);

    case CANCEL_ORDER:
        return CancelOrder(get_json);

    case CANCEL_TICKET:
        return CancelTicket(get_json);

    case PAY_TICKET:
        return PayTicket(get_json);

    case CHANGE_TICKET:
        return ChangeTicket(get_json);

    case TRANSFER:
        return Transfer(get_json);

    default:
        QJsonObject send_json;
        send_json.insert("消息", "失败");
        return send_json;
    }
}

void ServerThread::sendData(QJsonObject json, QTcpSocket* tcpSocket)
{
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    //发送长度
    QJsonObject length{ {"length", data.length()} };
    QJsonDocument length_doc(length);
    QByteArray length_array = length_doc.toJson();
    qDebug() << "length_array:" << length_array.length();
    qDebug() << "长度：" << data.length();
    tcpSocket->write(length_array);
    tcpSocket->waitForBytesWritten();
    tcpSocket->flush();

    if (tcpSocket->waitForReadyRead())
    {
        tcpSocket->write(data);
        tcpSocket->waitForBytesWritten();
        qDebug() << "服务器发送数据：";
        qDebug() << json;
    }
}

QString ServerThread::GenerateByte(int first, int last, int total_num)
{
    QString result_byte;
    for (int i = 0; i < total_num; i++) 
    {
        if (i < first-1 || i >= last-1)
        {
            result_byte.append("0");
        }
        else
        {
            result_byte.append("1");
        }
    }
    return result_byte;
}

QString ServerThread::GenerateOcupiedByte(int first, int last, int total_num)
{
    QString result_byte;
    for (int i = 0; i < total_num; i++)
    {
        if (i < first - 1 || i >= last - 1)
        {
            result_byte.append("1");
        }
        else
        {
            result_byte.append("0");
        }
    }
    return result_byte;
}

QJsonArray ServerThread::GetSeat(QJsonObject search_seats, int total_seat_num, QString seat_type, QString train_id, int departure_order, int arrival_order, int total_order, QString date)
{
    int search_seats_num = 0;
    if (search_seats.contains("第一排"))
    {
        search_seats_num += search_seats.value("第一排").toArray().size();
    }
    if (search_seats.contains("第二排"))
    {
        search_seats_num += search_seats.value("第二排").toArray().size();
    }

    //找车厢
    QVector<int> coach_ids;
    QSqlQuery search_coach_sql;
    search_coach_sql.exec("select coach_id from coach where train_id = '"
        + train_id + "' and seat_type = '" + seat_type + "'");
    while (search_coach_sql.next())
    {
        coach_ids.append(search_coach_sql.value("coach_id").toInt());
    }
    //车厢乱序
    for (int i = 0; i < coach_ids.size(); i++)
    {
        srand(i);
        int swap_num = rand() % coach_ids.size();
        std::swap(coach_ids[i], coach_ids[swap_num]);
    }
    QString free_byte = GenerateByte(departure_order, arrival_order, total_order);

    int line_num;
    if (seat_type == "二等")
    {
        line_num = 17;
    }
    else if (seat_type == "一等")
    {
        line_num = 15;
    }
    else
    {
        line_num = 8;
    }

    QJsonArray found_seats;
    for (int i = 0; i < coach_ids.size(); i++)
    {
        qDebug() << "当前车厢" << i;
        QVector<QVector<QString> > seat(line_num + 1);
        //插入座位
        int current_coach_id = coach_ids[i];
        QSqlQuery search_seat_sql;
        search_seat_sql.prepare("select seat_id from seat where train_id = ? and coach_id = ? and date = ? and free = free | ?");
        search_seat_sql.addBindValue(train_id);
        search_seat_sql.addBindValue(current_coach_id);
        search_seat_sql.addBindValue(date);
        search_seat_sql.addBindValue(free_byte);
        search_seat_sql.exec();
        while (search_seat_sql.next())
        {
            QString seat_id = search_seat_sql.value("seat_id").toString();
            int line;
            QString num;
            if (seat_id.size() == 3)
            {
                line = seat_id.left(2).toInt();
                num = seat_id.right(1);
            }
            else if (seat_id.size() == 2)
            {
                line = seat_id.left(1).toInt();
                num = seat_id.right(1);
            }
            seat[line].push_back(num);
        }
        qDebug() << seat;

        //寻找座位
        int current_line = 1;
        if (search_seats.contains("第二排"))
        {
            QJsonArray first_line = search_seats.value("第一排").toArray();
            QJsonArray second_line = search_seats.value("第二排").toArray();
            qDebug() << "first_line" << first_line;
            qDebug() << "second_line" << second_line;

            //搜索第j排和j+1排
            for (int j = 1; j < seat.size() - 1; j++)
            {
                current_line = j;
                QJsonArray found_seats2;
                if (seat[j].size() < first_line.size() || seat[j + 1].size() < second_line.size())
                {
                    continue;
                }
                bool finished_search = false;
                //搜索第一排
                for (int seat_no = 0; seat_no < seat[j].size(); seat_no++)
                {
                    for (int search_num = 0; search_num < first_line.size(); search_num++)
                    {
                        if (seat[j][seat_no] == first_line[search_num].toString())
                        {
                            QString current_seat_id = QString::number(j) + seat[j][seat_no];
                            QJsonObject current_seat{
                                { "车厢号", coach_ids[i] },
                                { "座位号", current_seat_id }
                            };
                            found_seats2.append(current_seat);
                        }
                    }
                }
                //搜索第二排
                if (found_seats2.size() == first_line.size())
                {
                    for (int seat_no = 0; seat_no < seat[j + 1].size(); seat_no++)
                    {
                        for (int search_num = 0; search_num < first_line.size(); search_num++)
                        {
                            if (seat[j + 1][seat_no] == second_line[search_num].toString())
                            {
                                QString current_seat_id = QString::number(j + 1) + seat[j + 1][seat_no];
                                QJsonObject current_seat{
                                    { "车厢号", coach_ids[i] },
                                    { "座位号", current_seat_id }
                                };
                                found_seats2.append(current_seat);
                            }
                        }
                    }
                }
                found_seats = found_seats2;
                qDebug() << "found_seats2" << found_seats2;
                if (first_line.size() + second_line.size() == found_seats2.size())
                {
                    break;
                }
            }
        }
        else
        {
            QJsonArray first_line = search_seats.value("第一排").toArray();
            qDebug() << "first_line" << first_line;
            for (int j = 1; j < seat.size(); j++)
            {
                current_line = j;
                QJsonArray found_seats2;
                for (int seat_no = 0; seat_no < seat[j].size(); seat_no++)
                {
                    for (int search_num = 0; search_num < first_line.size(); search_num++)
                    {
                        if (seat[j][seat_no] == first_line[search_num].toString())
                        {
                            QString current_seat_id = QString::number(j) + seat[j][seat_no];
                            QJsonObject current_seat{
                                { "车厢号", coach_ids[i] },
                                { "座位号", current_seat_id }
                            };
                            found_seats2.append(current_seat);
                        }
                    }
                }
                found_seats = found_seats2;
                if (found_seats2.size() == first_line.size())
                {
                    break;
                }
            }
        }
        qDebug() << "found_seats:" << found_seats;
        qDebug() << search_seats.size();
        qDebug() << found_seats.size() << "    " << total_seat_num << "   " << search_seats.size();
        //3种情况，选座都满足，选座不满足
        //选座满足，但总体不满足
        if (found_seats.size() < total_seat_num && found_seats.size() == search_seats_num)
        {
            qDebug() << "执行此处";
            int left_num = total_seat_num - found_seats.size();
            //往后找
            for (int line = current_line; line < seat.size(); line++)
            {
                for (int current_seat_id = 0; current_seat_id < seat[line].size(); current_seat_id++)
                {
                    QJsonObject current_seat{
                        { "车厢号", coach_ids[i] },
                        { "座位号", QString::number(line) + seat[line][current_seat_id] }
                    };
                    if (!found_seats.contains(current_seat))
                    {
                        found_seats.append(current_seat);
                        left_num--;
                    }
                    if (left_num == 0)
                    {
                        break;
                    }
                }
                if (left_num == 0)
                {
                    break;
                }
            }
            if (left_num == 0)
            {
                break;
            }
            //往前找
            for (int line = current_line - 1; line > 0; line--)
            {
                for (int current_seat_id = 0; current_seat_id < seat[line].size(); current_seat_id++)
                {
                    QJsonObject current_seat{
                        { "车厢号", coach_ids[i] },
                        { "座位号", QString::number(line) + seat[line][current_seat_id] }
                    };
                    if (!found_seats.contains(current_seat))
                    {
                        found_seats.append(current_seat);
                        left_num--;
                    }
                    if (left_num == 0)
                    {
                        break;
                    }
                }
                if (left_num == 0)
                {
                    break;
                }
            }
        }
        qDebug() << "found_seats" << found_seats;
        //选座满足
        if (found_seats.size() == total_seat_num)
        {
            break;
        }
    }
    //剩余座位从全车厢找
    int left_num = total_seat_num - found_seats.size();
    if (left_num > 0)
    {
        QSqlQuery search_left_seat_sql;
        search_left_seat_sql.prepare("select coach_id, seat_id from seat where train_id = ? and date = ? and free = free | ? ");
        search_left_seat_sql.addBindValue(train_id);
        search_left_seat_sql.addBindValue(date);
        search_left_seat_sql.addBindValue(free_byte);
        search_left_seat_sql.exec();
        while (search_left_seat_sql.next())
        {
            int current_coach_id = search_left_seat_sql.value("coach_id").toInt();
            QString current_seat_id = search_left_seat_sql.value("seat_id").toString();
            QJsonObject current_seat{
                { "车厢号", current_coach_id },
                { "座位号", current_seat_id }
            };
            if (!found_seats.contains(current_seat))
            {
                found_seats.append(current_seat);
                left_num--;
            }
            if (left_num == 0)
            {
                break;
            }
        }
    }
    return found_seats;
}

double ServerThread::GetPrice(QString seat_type, QString passenger_type)
{
    double rate = 1.0;
    double price = 0.0;
    if (passenger_type == "学生")
    {
        rate = 0.7;
    }
    if (seat_type == "二等")
    {
        price = 200.00 * rate;
    }
    else if (seat_type == "一等")
    {
        price = 400.00 * rate;
    }
    else if (seat_type == "商务")
    {
        price = 600 * rate;
    }
    return price;
}

//注册
QJsonObject ServerThread::Register(QJsonObject get_json)
{
    QJsonObject send_json;

    //用户名查重
    QString username = get_json.take("用户名").toString();
    QSqlQuery query_username;
    query_username.exec("select user_id from user_info where user_id = '" + username +"'");
    //若重复
    if (query_username.next()) {
        send_json.insert("消息", "失败");
    }
    //若不重复
    else {
        QString password = get_json.take("密码").toString();
        QString name = get_json.take("姓名").toString();
        QString sex = get_json.take("性别").toString();
        QString id_number = get_json.take("身份证号").toString();
        QString phone = get_json.take("手机").toString();
        QString email = get_json.take("邮箱").toString();

        QSqlQuery register_sql;
        register_sql.prepare("insert into user_info values(?,?,?,?,?,?,?)");
        register_sql.addBindValue(username);
        register_sql.addBindValue(password);
        register_sql.addBindValue(name);
        register_sql.addBindValue(sex);
        register_sql.addBindValue(id_number);
        register_sql.addBindValue(phone);
        register_sql.addBindValue(email);
        register_sql.exec();

        send_json.insert("消息", "成功");
    }

    return send_json;
}

//登录
QJsonObject ServerThread::LogIn(QJsonObject get_json)
{
    QJsonObject send_json;

    //查询正确与否
    QString username = get_json.take("用户名").toString();
    QString password = get_json.take("密码").toString();
    QSqlQuery login_select;
    login_select.exec("select user_id from user_info where user_id = '"
                      + username + "' and user_password = '" + password +"'");
    if (login_select.next()) {
        send_json.insert("消息", "成功");
    }
    else {
        send_json.insert("消息", "失败");
    }

    return send_json;
}

//查看个人信息
QJsonObject ServerThread::Information(QJsonObject get_json)
{
    QJsonObject send_json;

    QString user_id = get_json.take("用户名").toString();
    QSqlQuery select_info;
    select_info.exec("select user_name, sex, id_number,phone_number, " +
        QString("email from user_info where user_id = '") + user_id + "'");

    if (select_info.next()) {
        QString name = select_info.value(0).toString();
        QString sex = select_info.value(1).toString();
        QString id_number = select_info.value(2).toString();
        QString phone_number = select_info.value(3).toString();
        QString email = select_info.value(4).toString();
        send_json.insert("姓名", name);
        send_json.insert("性别", sex);
        send_json.insert("身份证号", id_number);
        send_json.insert("手机号", phone_number);
        send_json.insert("邮箱", email);
    }

    return send_json;
}

//更改个人信息
QJsonObject ServerThread::UpdateInformation(QJsonObject get_json)
{
    db.transaction();
    QString username = get_json.take("用户名").toString();
    QString phone_number = get_json.take("手机号").toString();
    QString email = get_json.take("邮箱").toString();

    QSqlQuery update_sql;
    bool ok = update_sql.exec("update user_info set phone_number = '" +
        phone_number + "' , email = '" + email + "' where user_id = '" + username + "'");

    QJsonObject send_json;
    if (ok)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }

    return send_json;
}

//修改密码
QJsonObject ServerThread::ChangePassword(QJsonObject get_json)
{
    QJsonObject send_json;

    QString username = get_json.take("用户名").toString();
    QString old_password = get_json.take("原密码").toString();
    QString new_password = get_json.take("新密码").toString();

    //查询原密码是否正确
    QSqlQuery check_old_password_sql;
    check_old_password_sql.exec("select user_id from user_info where user_id = '"
        + username + "' and user_password = '" + old_password + "'");
    if (check_old_password_sql.next())
    {
        QSqlQuery change_password_sql;
        change_password_sql.exec("update user_info set user_password = '"
            + new_password + "' where user_id = '" + username + "'");

        
        send_json.insert("消息", "成功");
    }
    else
    {
        send_json.insert("消息", "原密码不正确");
    }

    return send_json;
}

//查看乘车人
QJsonObject ServerThread::LoadPassenger(QJsonObject get_json)
{
    QString username = get_json.take("用户名").toString();
    
    QJsonObject send_json;
    QJsonArray passenger_array;

    QSqlQuery select_passenger_sql;
    select_passenger_sql.exec("select * from passenger where user_id = '" + username + "'");
    while (select_passenger_sql.next()) {
        QString id_number = select_passenger_sql.value("id_number").toString();
        QString name = select_passenger_sql.value("passenger_name").toString();
        QString sex = select_passenger_sql.value("sex").toString();
        QString phone_number = select_passenger_sql.value("phone_number").toString();
        QString passenger_type = select_passenger_sql.value("passenger_type").toString();

        QJsonObject passenger{
            {"身份证号", id_number},
            {"姓名", name},
            {"性别", sex},
            {"手机号", phone_number},
            {"类型", passenger_type}
        };

        passenger_array.append(passenger);
    }

    send_json.insert("乘车人", passenger_array);
    send_json.insert("消息", "成功");
    return send_json;
}

QJsonObject ServerThread::ChangePassenger(QJsonObject get_json)
{
    db.transaction();
    QJsonObject send_json;

    QString username = get_json.take("用户名").toString();
    QString id_number = get_json.take("身份证号").toString();
    QString phone = get_json.take("手机号").toString();
    QString type = get_json.take("类型").toString();

    QSqlQuery change_passenger_sql;
    bool ok = change_passenger_sql.exec("update passenger set phone_number = '"
        + phone + "', passenger_type = '" + type + "' where id_number = '"
        + id_number + "' and user_id = '" + username + "'");

    if (ok)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }

    return send_json;
}

QJsonObject ServerThread::DeletePassenger(QJsonObject get_json)
{
    db.transaction();
    QJsonObject send_json;

    QString username = get_json.take("用户名").toString();
    QString id_number = get_json.take("身份证号").toString();

    QSqlQuery delete_passenger_sql;
    bool ok = delete_passenger_sql.exec("delete from passenger where user_id = '"
        + username + "' and id_number = '" + id_number + "'");

    if (ok)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }
    return send_json;
}

QJsonObject ServerThread::AddPassenger(QJsonObject get_json)
{
    db.transaction();
    QJsonObject send_json;

    QString username = get_json.take("用户名").toString();
    QString passenger_name = get_json.take("姓名").toString();
    QString sex = get_json.take("性别").toString();
    QString id_number = get_json.take("身份证号").toString();
    QString phone = get_json.take("手机号").toString();
    QString type = get_json.take("类型").toString();

    QSqlQuery add_passenger_sql;
    add_passenger_sql.prepare("insert into passenger values(?,?,?,?,?,?)");
    add_passenger_sql.addBindValue(id_number);
    add_passenger_sql.addBindValue(username);
    add_passenger_sql.addBindValue(passenger_name);
    add_passenger_sql.addBindValue(sex);
    add_passenger_sql.addBindValue(phone);
    add_passenger_sql.addBindValue(type);
    bool ok = add_passenger_sql.exec();

    if (ok)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }

    return send_json;
}

QJsonObject ServerThread::SearchTrain(QJsonObject get_json)
{
    QJsonObject send_json;

    QString select_train_id = get_json.take("车次号").toString();

    QSqlQuery search_train_sql;
    search_train_sql.exec("select * from train where train_id = '" + select_train_id + "'");
    if (search_train_sql.next())
    {
        QString train_id = search_train_sql.value("train_id").toString();
        QString first_station = search_train_sql.value("first_station").toString();
        QString last_station = search_train_sql.value("last_station").toString();
        QString departure_time = search_train_sql.value("departure_time").toString();
        QString arrival_time = search_train_sql.value("destination_time").toString();
        QString train_type = search_train_sql.value("train_type").toString();
        
        send_json.insert("车次号", train_id);
        send_json.insert("起始站", first_station);
        send_json.insert("终点站", last_station);
        send_json.insert("始发时间", departure_time);
        send_json.insert("终到时间", arrival_time);
        send_json.insert("车次类型", train_type);

        QSqlQuery search_pass_by_sql;
        search_pass_by_sql.exec("select station_id, arrival_time, start_time, day from pass_by where train_id = '" + train_id + "' order by arrival_order");
        QJsonArray pass_by_station;
        while (search_pass_by_sql.next())
        {
            QString station_id = search_pass_by_sql.value("station_id").toString();
            QString arrival_time = search_pass_by_sql.value("arrival_time").toString();
            QString start_time = search_pass_by_sql.value("start_time").toString();
            int day = search_pass_by_sql.value("day").toInt();

            QJsonObject pass_by_info{
                {"车站", station_id},
                {"到达时间", arrival_time},
                {"开车时间", start_time},
                {"跨天", day}
            };
            pass_by_station.append(pass_by_info);
        }
        send_json.insert("途径站", pass_by_station);
        send_json.insert("消息", "成功");
    }
    else
    {
        search_train_sql.exec("select * from train where train_id like '%" + select_train_id + "%'");
        if (search_train_sql.next())
        {
            QString train_id = search_train_sql.value("train_id").toString();
            QString first_station = search_train_sql.value("first_station").toString();
            QString last_station = search_train_sql.value("last_station").toString();
            QString departure_time = search_train_sql.value("departure_time").toString();
            QString arrival_time = search_train_sql.value("destination_time").toString();
            QString train_type = search_train_sql.value("train_type").toString();

            send_json.insert("车次号", train_id);
            send_json.insert("起始站", first_station);
            send_json.insert("终点站", last_station);
            send_json.insert("始发时间", departure_time);
            send_json.insert("终到时间", arrival_time);
            send_json.insert("车次类型", train_type);

            QSqlQuery search_pass_by_sql;
            search_pass_by_sql.exec("select station_id, arrival_time, start_time, day from pass_by where train_id = '" + train_id + "' order by arrival_order");
            QJsonArray pass_by_station;
            while (search_pass_by_sql.next())
            {
                QString station_id = search_pass_by_sql.value("station_id").toString();
                QString arrival_time = search_pass_by_sql.value("arrival_time").toString();
                QString start_time = search_pass_by_sql.value("start_time").toString();
                int day = search_pass_by_sql.value("day").toInt();

                QJsonObject pass_by_info{
                    {"车站", station_id},
                    {"到达时间", arrival_time},
                    {"开车时间", start_time},
                    {"跨天", day}
                };
                pass_by_station.append(pass_by_info);
            }
            send_json.insert("途径站", pass_by_station);
            send_json.insert("消息", "成功");
        }
        else
        {
            send_json.insert("消息", "抱歉，没有此车的信息");
        }
    }

    return send_json;
}

QJsonObject ServerThread::SearchStation(QJsonObject get_json)
{
    QJsonObject send_json;
    QString station = get_json.take("车站名").toString();

    QSqlQuery search_station_sql;
    search_station_sql.exec("select * from pass_by join train on pass_by.train_id = train.train_id where pass_by.station_id = '" 
        + station +"' order by arrival_time");
    QJsonArray pass_by_trains;
    while (search_station_sql.next())
    {
        QString train_id = search_station_sql.value("train_id").toString();
        QString first_station = search_station_sql.value("first_station").toString();
        QString last_station = search_station_sql.value("last_station").toString();
        QString departure_time = search_station_sql.value("departure_time").toString();
        QString destination_time = search_station_sql.value("destination_time").toString();
        QString arrival_station = search_station_sql.value("station_id").toString();
        QString arrival_time = search_station_sql.value("arrival_time").toString();
        QString start_time = search_station_sql.value("start_time").toString();

        QJsonObject pass_by_info{
            {"车次号", train_id},
            {"起始站", first_station},
            {"终点站", last_station},
            {"首发时间", departure_time},
            {"终到时间", destination_time},
            {"途经站", arrival_station},
            {"到站时间", arrival_time},
            {"开车时间", start_time}
        };
        pass_by_trains.append(pass_by_info);
    }
    if (pass_by_trains.empty())
    {
        send_json.insert("消息", "没有该车站的信息");
    }
    else
    {
        send_json.insert("消息", "成功");
        send_json.insert("途经列车", pass_by_trains);
    }

    return send_json;
}

QJsonObject ServerThread::StationStation(QJsonObject get_json)
{
    QJsonObject send_json;

    QString departure_station = get_json.take("出发站").toString();
    QString arrival_station = get_json.take("到达站").toString();
    QString date = get_json.take("日期").toString();
    QSqlQuery search_station_station_sql;
    //找是否为同一个城市，若是，准确站名，若不是模糊
    //查找是否为一个城市
    QString departure_city;
    QString arrival_city;
    search_station_station_sql.exec("select city from station where station_id = '" + departure_station + "' or city = '" + departure_station +"'");
    if (search_station_station_sql.next()) 
    {
        departure_city = search_station_station_sql.value("city").toString();
    }
    search_station_station_sql.exec("select city from station where station_id = '" + arrival_station + "' or city = '" + arrival_station +"'");
    if (search_station_station_sql.next()) 
    {
        arrival_city = search_station_station_sql.value("city").toString();
    }
    if (departure_city.isEmpty())
    {
        send_json.insert("消息", "没有车站" + departure_city + "的信息");
        return send_json;
    }
    if (arrival_city.isEmpty())
    {
        send_json.insert("消息", "没有车站" + arrival_city + "的信息");
        return send_json;
    }

    QJsonArray trains;
    QSqlQuery search_train_sql;
    //如果是一个城市
    if (departure_city == arrival_city)
    {
        
        search_train_sql.prepare("select * from (pass_by natural join station) as pa join (pass_by natural join station) as pb on pa.train_id = pb.train_id " +
            QString("where pa.station_id = ? and pb.station_id = ? and pa.arrival_order < pb.arrival_order order by pa.start_time"));
        search_train_sql.addBindValue(departure_station);
        search_train_sql.addBindValue(arrival_station);
        search_train_sql.exec();
    }
    //如果不是一个城市
    else
    {
        search_train_sql.prepare("select * from (pass_by natural join station) as pa join (pass_by natural join station) as pb on pa.train_id = pb.train_id " +
            QString("where pa.city = ? and pb.city = ? and pa.arrival_order < pb.arrival_order order by pa.start_time"));
        search_train_sql.addBindValue(departure_city);
        search_train_sql.addBindValue(arrival_city);
        search_train_sql.exec();
    }
    while (search_train_sql.next())
    {
        QString train_id = search_train_sql.value("train_id").toString();

        QString departure_station = search_train_sql.value(0).toString();
        QString departure_time = search_train_sql.value(4).toString();
        QString departure_day = search_train_sql.value(5).toString();

        QString arrival_station = search_train_sql.value(7).toString();
        QString arrival_time = search_train_sql.value(10).toString();
        QString arrival_day = search_train_sql.value(12).toString();

        int start_order = search_train_sql.value(2).toInt();
        int arrival_order = search_train_sql.value(9).toInt();
        QJsonObject train{
            {"车次号", train_id},
            {"出发站", departure_station},
            {"出发时间", departure_time},
            {"出发日", departure_day},
            {"到达站", arrival_station},
            {"到达时间", arrival_time},
            {"到达日", arrival_day},
        };

        QSqlQuery total_station_sql;
        total_station_sql.exec("select count(*) from pass_by where train_id = '" + train_id + "'");
        int total_station_num = 0;
        if (total_station_sql.next())
        {
            total_station_num = total_station_sql.value(0).toInt();
        }
        QString free_byte = GenerateByte(start_order, arrival_order, total_station_num);
        //查询余票
        QSqlQuery free_count_sql;
        free_count_sql.prepare("select seat_type, count(*) from seat natural join coach " 
            + QString("where train_id = ? and free = free | ? and date = ? group by seat_type"));
        free_count_sql.addBindValue(train_id);
        free_count_sql.addBindValue(free_byte);
        free_count_sql.addBindValue(date);
        free_count_sql.exec();
        QJsonObject free_seat_count;
        while (free_count_sql.next())
        {
            QString seat_type = free_count_sql.value("seat_type").toString();
            int seat_count = free_count_sql.value("count").toInt();
            free_seat_count.insert(seat_type, seat_count);
        }
        train.insert("余票", free_seat_count);

        //加入列表
        trains.append(train);
    }
    if (!trains.isEmpty())
    {
        send_json.insert("车次", trains);
        send_json.insert("消息", "成功");
    }
    else
    {
        send_json.insert("消息", "换乘");
    }
    return send_json;
}

QJsonObject ServerThread::SubmitOrder(QJsonObject get_json)
{
    db.transaction();
    QString username = get_json.take("用户名").toString();
    QJsonArray tickets = get_json.take("车票").toArray();
    QJsonArray passengers = get_json.take("乘车人").toArray();

    //插订单
    QSqlQuery insert_order_sql;
    QString datetime_now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString order_id = username + QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    insert_order_sql.prepare("insert into order_info values(?, ?, ?, ?,?)");
    insert_order_sql.addBindValue(order_id);
    insert_order_sql.addBindValue(username);
    insert_order_sql.addBindValue(datetime_now);
    insert_order_sql.addBindValue("未付款");
    insert_order_sql.addBindValue(0);
    bool ok1 = insert_order_sql.exec();

    bool ok2 = false;
    bool ok3 = false;
    for (int i = 0; i < tickets.size(); i++)
    {
        QJsonObject current_ticket = tickets.at(i).toObject();
        //需要提供的信息：车次号、日期、出发站、到达站、用户名、乘车人、座位类型、座位
        QString train_id = current_ticket.take("车次号").toString();
        QString date = current_ticket.take("日期").toString();
        QString departure_station = current_ticket.take("出发站").toString();
        QString arrival_station = current_ticket.take("到达站").toString();

        QString seat_type = current_ticket.take("座位类型").toString();
        QJsonObject search_seats = current_ticket.take("选座").toObject();


        //确定到达次序
        int departure_order = 0;
        int arrival_order = 0;
        int total_order = 0;
        QSqlQuery search_order_sql;
        search_order_sql.exec("select arrival_order from pass_by where train_id = '"
            + train_id + "' and station_id = '" + departure_station + "'");
        if (search_order_sql.next())
        {
            departure_order = search_order_sql.value("arrival_order").toInt();
        }
        search_order_sql.exec("select arrival_order from pass_by where train_id = '"
            + train_id + "' and station_id = '" + arrival_station + "'");
        if (search_order_sql.next())
        {
            arrival_order = search_order_sql.value("arrival_order").toInt();
        }
        search_order_sql.exec("select count(*) from pass_by where train_id = '" + train_id + "'");
        if (search_order_sql.next())
        {
            total_order = search_order_sql.value(0).toInt();
        }

        //寻找座位
        QJsonArray seats = GetSeat(search_seats, passengers.size(), seat_type, train_id, departure_order, arrival_order, total_order, date);
        qDebug() << "seat:" << seats;



        //改座位
        QString ocupied_byte = GenerateOcupiedByte(departure_order, arrival_order, total_order);
        for (int i = 0; i < passengers.size(); i++)
        {
            QJsonObject passenger = passengers.at(i).toObject();

            QJsonObject seat = seats.at(i).toObject();
            int coach_id = seat.value("车厢号").toInt();
            QString seat_id = seat.value("座位号").toString();
            passenger.insert("座位", seat);
            passengers.replace(i, passenger);

            QString id_number = passenger.value("身份证号").toString();
            QString ticket_type = passenger.value("类型").toString();
            double price = GetPrice(seat_type, ticket_type);
            QSqlQuery insert_order_passenger_sql;
            insert_order_passenger_sql.prepare("insert into order_passenger values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            insert_order_passenger_sql.addBindValue(order_id);
            insert_order_passenger_sql.addBindValue(id_number);
            insert_order_passenger_sql.addBindValue(train_id);
            insert_order_passenger_sql.addBindValue(departure_station);
            insert_order_passenger_sql.addBindValue(arrival_station);
            insert_order_passenger_sql.addBindValue(coach_id);
            insert_order_passenger_sql.addBindValue(seat_id);
            insert_order_passenger_sql.addBindValue(date);
            insert_order_passenger_sql.addBindValue(price);
            insert_order_passenger_sql.addBindValue("未付款");
            insert_order_passenger_sql.addBindValue(ticket_type);
            ok2 = insert_order_passenger_sql.exec();
            QSqlQuery update_seat_sql;
            update_seat_sql.prepare("update seat set free = free & ? where train_id = ? and coach_id = ? and seat_id = ? and date = ?");
            qDebug() << ocupied_byte;
            update_seat_sql.addBindValue(ocupied_byte);
            update_seat_sql.addBindValue(train_id);
            update_seat_sql.addBindValue(coach_id);
            update_seat_sql.addBindValue(seat_id);
            update_seat_sql.addBindValue(date);
            ok3 = update_seat_sql.exec();
        }
    }

    //更新订单总价
    QSqlQuery update_price_sql;
    bool ok4 = update_price_sql.exec("update order_info as po set total_price = (select sum(price) from order_passenger where po.order_id = order_id) where order_id = '" + order_id + "'");
    
    //搜索订单
    QJsonArray orders;
    QSqlQuery search_orders_sql;
    search_orders_sql.exec("select * from order_passenger natural join coach natural join passenger where order_id = '" + order_id + "'");
    double total_price = 0.00;
    while (search_orders_sql.next())
    {
        QString name = search_orders_sql.value("passenger_name").toString();
        QString id_number = search_orders_sql.value("id_number").toString();
        QString ticket_type = search_orders_sql.value("ticket_type").toString();
        QString train_id = search_orders_sql.value("train_id").toString();
        QString departure_station = search_orders_sql.value("departure_station").toString();
        QString arrival_station = search_orders_sql.value("arrival_station").toString();
        QString departure_date = search_orders_sql.value("train_date").toString();
        int coach_id = search_orders_sql.value("coach_id").toInt();
        QString seat_id = search_orders_sql.value("seat_id").toString();
        QString seat_type = search_orders_sql.value("seat_type").toString();
        double price = search_orders_sql.value("price").toDouble();
        total_price += price;
        QString ticket_status = search_orders_sql.value("ticket_status").toString();
        QJsonObject order{
            {"姓名", name},
            {"身份证号", id_number},
            {"类型", ticket_type},
            {"车次号", train_id},
            {"出发站", departure_station},
            {"到达站", arrival_station},
            {"出发日期", departure_date},
            {"车厢号", coach_id},
            {"座位号", seat_id},
            {"座位类型", seat_type},
            {"价格", price},
            {"状态", ticket_status},
        };
        orders.append(order);
    }
    

    QJsonObject send_json{
        {"订单号", order_id},
        {"订单时间", datetime_now},
        {"车票", orders},
        {"订单状态", "未付款"},
        {"总价", total_price},
    };

    if (ok1 && ok2 && ok3 && ok4)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }
    return send_json;
}

QJsonObject ServerThread::PayOrder(QJsonObject get_json)
{
    db.transaction();
    QString order_id = get_json.value("订单号").toString();
    QSqlQuery pay_order_sql;
    bool ok1 = pay_order_sql.exec("update order_info set order_status = '已完成' where order_id = '" + order_id + "'");
    bool ok2 = pay_order_sql.exec("update order_passenger set ticket_status = '已完成' where order_id = '" + order_id + "' and ticket_status = '未付款'");
    
    QJsonObject send_json;
    if (ok1 && ok2)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }
    return send_json;
}

QJsonObject ServerThread::SearchOrder(QJsonObject get_json)
{
    QString username = get_json.take("用户名").toString();
    QString order_type = get_json.take("订单类型").toString();

    QJsonArray orders;
    QSqlQuery search_order_sql;
    if (order_type == "全部")
    {
        search_order_sql.exec("select * from order_info where user_id = '" + username + "'");
        while (search_order_sql.next())
        {
            QString order_id = search_order_sql.value("order_id").toString();
            QString order_time = search_order_sql.value("order_date").toString();
            QString order_status = search_order_sql.value("order_status").toString();
            double total_price = search_order_sql.value("total_price").toDouble();
            QJsonObject order {
                {"订单号", order_id},
                {"订单时间", order_time},
                {"订单状态", order_status},
                {"总价", total_price}
            };
            orders.append(order);
        }
    }
    else
    {
        search_order_sql.exec("select * from order_info where user_id = '" 
            + username + "' and order_status = '" + order_type + "'");
        while (search_order_sql.next())
        {
            QString order_id = search_order_sql.value("order_id").toString();
            QString order_time = search_order_sql.value("order_date").toString();
            QString order_status = search_order_sql.value("order_status").toString();
            double total_price = search_order_sql.value("total_price").toDouble();
            QJsonObject order{
                {"订单号", order_id},
                {"订单时间", order_time},
                {"订单状态", order_status},
                {"总价", total_price}
            };
            orders.append(order);
        }
    }

    QJsonObject send_json{
        {"订单", orders},
        {"消息", "成功"}
    };

    return send_json;
}

QJsonObject ServerThread::OrderInformation(QJsonObject get_json)
{
    QString order_id = get_json.take("订单号").toString();
    QJsonArray tickets;
    QSqlQuery search_ticket_sql;
    search_ticket_sql.exec("select * from order_passenger natural join coach natural join passenger where order_id = '" + order_id + "'");
    while (search_ticket_sql.next())
    {
        QString name = search_ticket_sql.value("passenger_name").toString();
        QString id_number = search_ticket_sql.value("id_number").toString();
        QString ticket_type = search_ticket_sql.value("ticket_type").toString();
        QString train_id = search_ticket_sql.value("train_id").toString();
        QString departure_station = search_ticket_sql.value("departure_station").toString();
        QString arrival_station = search_ticket_sql.value("arrival_station").toString();
        QString departure_date = search_ticket_sql.value("train_date").toString();
        int coach_id = search_ticket_sql.value("coach_id").toInt();
        QString seat_id = search_ticket_sql.value("seat_id").toString();
        QString seat_type = search_ticket_sql.value("seat_type").toString();
        double price = search_ticket_sql.value("price").toDouble();
        QString ticket_status = search_ticket_sql.value("ticket_status").toString();
        QJsonObject ticket{
            {"姓名", name},
            {"身份证号", id_number},
            {"类型", ticket_type},
            {"车次号", train_id},
            {"出发站", departure_station},
            {"到达站", arrival_station},
            {"出发日期", departure_date},
            {"车厢号", coach_id},
            {"座位号", seat_id},
            {"座位类型", seat_type},
            {"价格", price},
            {"状态", ticket_status},
        };
        tickets.append(ticket);
    }

    QJsonObject send_json{
        {"车票", tickets},
        {"消息", "成功"}
    };
    return send_json;
}

//取消订单
QJsonObject ServerThread::CancelOrder(QJsonObject get_json)
{
    db.database();
    QString order_id = get_json.take("订单号").toString();

    //改订单
    QSqlQuery cancel_order_sql;
    bool ok1 = cancel_order_sql.exec("update order_info set order_status = '已取消' where order_id = '" + order_id + "'");

    //改票
    QSqlQuery cancel_ticket_sql;
    bool ok2 = cancel_ticket_sql.exec("update order_passenger set ticket_status = '已取消' where order_id = '" + order_id + "'");
    
    //改座位
    QSqlQuery select_seat_sql;
    select_seat_sql.exec("select * from order_passenger where order_id = '" + order_id + "'");
    bool ok3 = false;
    while (select_seat_sql.next())
    {
        QString train_id = select_seat_sql.value("train_id").toString();
        QString departure_station = select_seat_sql.value("departure_station").toString();
        QString arrival_station = select_seat_sql.value("arrival_station").toString();
        int coach_id = select_seat_sql.value("coach_id").toInt();
        QString seat_id = select_seat_sql.value("seat_id").toString();
        QString train_date = select_seat_sql.value("train_date").toString();

        //确定到达次序
        int departure_order = 0;
        int arrival_order = 0;
        int total_order = 0;
        QSqlQuery search_order_sql;
        search_order_sql.exec("select arrival_order from pass_by where train_id = '"
            + train_id + "' and station_id = '" + departure_station + "'");
        if (search_order_sql.next())
        {
            departure_order = search_order_sql.value("arrival_order").toInt();
        }
        search_order_sql.exec("select arrival_order from pass_by where train_id = '"
            + train_id + "' and station_id = '" + arrival_station + "'");
        if (search_order_sql.next())
        {
            arrival_order = search_order_sql.value("arrival_order").toInt();
        }
        search_order_sql.exec("select count(*) from pass_by where train_id = '" + train_id + "'");
        if (search_order_sql.next())
        {
            total_order = search_order_sql.value(0).toInt();
        }

        //修改座位
        QString free_byte = GenerateByte(departure_order, arrival_order, total_order);
        QSqlQuery change_seat_sql;
        change_seat_sql.prepare("update seat set free = free | ? where train_id = ? and coach_id = ? and seat_id = ? and date = ?");
        change_seat_sql.addBindValue(free_byte);
        change_seat_sql.addBindValue(train_id);
        change_seat_sql.addBindValue(coach_id);
        change_seat_sql.addBindValue(seat_id);
        change_seat_sql.addBindValue(train_date);
        ok3 = change_seat_sql.exec();
    }

    QJsonObject send_json;

    if (ok1 && ok2 && ok3)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }

    return send_json;
}

//退票
QJsonObject ServerThread::CancelTicket(QJsonObject get_json)
{
    QString order_id = get_json.take("订单号").toString();
    QString id_number = get_json.take("身份证号").toString();
    QString train_id = get_json.take("车次号").toString();

    db.transaction();
    //改票
    QSqlQuery cancel_ticket_sql;
    cancel_ticket_sql.prepare("update order_passenger set ticket_status = '已退票' where order_id = ? and id_number = ? and train_id = ?");
    cancel_ticket_sql.addBindValue(order_id);
    cancel_ticket_sql.addBindValue(id_number);
    cancel_ticket_sql.addBindValue(train_id);
    bool ok1 = cancel_ticket_sql.exec();
    bool ok2 = false;

    //改座位
    QSqlQuery select_seat_sql;
    select_seat_sql.prepare("select * from order_passenger where order_id = ? and id_number = ? and train_id = ?");
    select_seat_sql.addBindValue(order_id);
    select_seat_sql.addBindValue(id_number);
    select_seat_sql.addBindValue(train_id);
    select_seat_sql.exec();
    while (select_seat_sql.next())
    {
        QString departure_station = select_seat_sql.value("departure_station").toString();
        QString arrival_station = select_seat_sql.value("arrival_station").toString();
        int coach_id = select_seat_sql.value("coach_id").toInt();
        QString seat_id = select_seat_sql.value("seat_id").toString();
        QString train_date = select_seat_sql.value("train_date").toString();

        //确定到达次序
        int departure_order = 0;
        int arrival_order = 0;
        int total_order = 0;
        QSqlQuery search_order_sql;
        search_order_sql.exec("select arrival_order from pass_by where train_id = '"
            + train_id + "' and station_id = '" + departure_station + "'");
        if (search_order_sql.next())
        {
            departure_order = search_order_sql.value("arrival_order").toInt();
        }
        search_order_sql.exec("select arrival_order from pass_by where train_id = '"
            + train_id + "' and station_id = '" + arrival_station + "'");
        if (search_order_sql.next())
        {
            arrival_order = search_order_sql.value("arrival_order").toInt();
        }
        search_order_sql.exec("select count(*) from pass_by where train_id = '" + train_id + "'");
        if (search_order_sql.next())
        {
            total_order = search_order_sql.value(0).toInt();
        }

        //修改座位
        QString free_byte = GenerateByte(departure_order, arrival_order, total_order);
        QSqlQuery change_seat_sql;
        change_seat_sql.prepare("update seat set free = free | ? where train_id = ? and coach_id = ? and seat_id = ? and date = ?");
        change_seat_sql.addBindValue(free_byte);
        change_seat_sql.addBindValue(train_id);
        change_seat_sql.addBindValue(coach_id);
        change_seat_sql.addBindValue(seat_id);
        change_seat_sql.addBindValue(train_date);
        ok2 = change_seat_sql.exec();
        qDebug() << free_byte << " " << train_id << " " << coach_id << " " << seat_id;
    }

    QJsonObject send_json;
    if (ok1 && ok2)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }

    return send_json;
}

QJsonObject ServerThread::PayTicket(QJsonObject get_json)
{
    db.transaction();
    QString order_id = get_json.take("订单号").toString();
    QString id_number = get_json.take("身份证号").toString();
    QString train_id = get_json.take("车次号").toString();

    //改票
    QSqlQuery change_ticket_sql;
    change_ticket_sql.prepare("update order_passenger set ticket_status = '已改签' where order_id = ? and id_number = ? and train_id = ?");
    change_ticket_sql.addBindValue(order_id);
    change_ticket_sql.addBindValue(id_number);
    change_ticket_sql.addBindValue(train_id);
    bool ok = change_ticket_sql.exec();

    QJsonObject send_json;
    if (ok)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }

    return send_json;
}

//改签
QJsonObject ServerThread::ChangeTicket(QJsonObject get_json)
{
    db.transaction();
    //需要提供的信息：车次号、日期、出发站、到达站、用户名、乘车人、座位类型、座位
    QString username = get_json.take("用户名").toString();
    QJsonArray passengers = get_json.take("乘车人").toArray();

    QJsonObject new_ticket = get_json.take("车票").toArray().at(0).toObject();

    QString train_id = new_ticket.take("车次号").toString();
    QString date = new_ticket.take("日期").toString();
    QString departure_station = new_ticket.take("出发站").toString();
    QString arrival_station = new_ticket.take("到达站").toString();
    
    QString seat_type = new_ticket.take("座位类型").toString();
    QJsonObject search_seats = new_ticket.take("选座").toObject();
    
    QJsonObject ticket = get_json.take("原车票").toObject();
    QString order_id = ticket.value("订单号").toString();

    //改之前的票
    QString old_train_id = ticket.value("原车次号").toString();
    QString old_departure_station = ticket.value("原出发站").toString();
    QString old_arrival_station = ticket.value("原到达站").toString();
    QString old_date = ticket.value("原日期").toString();
    int old_coach_id = ticket.value("原车厢号").toInt();
    QString old_seat_id = ticket.value("原座位号").toString();

    int old_departure_order = 0;
    int old_arrival_order = 0;
    int old_total_order = 0;
    QSqlQuery search_old_order_sql;
    search_old_order_sql.exec("select arrival_order from pass_by where train_id = '"
        + old_train_id + "' and station_id = '" + old_departure_station + "'");
    if (search_old_order_sql.next())
    {
        old_departure_order = search_old_order_sql.value("arrival_order").toInt();
    }
    search_old_order_sql.exec("select arrival_order from pass_by where train_id = '"
        + old_train_id + "' and station_id = '" + old_arrival_station + "'");
    if (search_old_order_sql.next())
    {
        old_arrival_order = search_old_order_sql.value("arrival_order").toInt();
    }
    search_old_order_sql.exec("select count(*) from pass_by where train_id = '" + old_train_id + "'");
    if (search_old_order_sql.next())
    {
        old_total_order = search_old_order_sql.value(0).toInt();
    }
    QString free_byte = GenerateByte(old_departure_order, old_arrival_order, old_total_order);
    QString old_id_number = ticket.value("身份证号").toString();
    QSqlQuery delete_old_sql;
    delete_old_sql.exec("delete from order_passenger where order_id = '"
        + order_id + "' and id_number = '" + old_id_number + "' and train_id = '" + old_train_id + "'");
    delete_old_sql.prepare("update seat set free = free | ? where train_id = ? and coach_id = ? and seat_id = ? and date = ?");
    delete_old_sql.addBindValue(free_byte);
    delete_old_sql.addBindValue(old_train_id);
    delete_old_sql.addBindValue(old_coach_id);
    delete_old_sql.addBindValue(old_seat_id);
    delete_old_sql.addBindValue(old_date);
    bool ok1 = delete_old_sql.exec();
    

    //确定到达次序
    int departure_order = 0;
    int arrival_order = 0;
    int total_order = 0;
    QSqlQuery search_order_sql;
    search_order_sql.exec("select arrival_order from pass_by where train_id = '"
        + train_id + "' and station_id = '" + departure_station + "'");
    if (search_order_sql.next())
    {
        departure_order = search_order_sql.value("arrival_order").toInt();
    }
    search_order_sql.exec("select arrival_order from pass_by where train_id = '"
        + train_id + "' and station_id = '" + arrival_station + "'");
    if (search_order_sql.next())
    {
        arrival_order = search_order_sql.value("arrival_order").toInt();
    }
    search_order_sql.exec("select count(*) from pass_by where train_id = '" + train_id + "'");
    if (search_order_sql.next())
    {
        total_order = search_order_sql.value(0).toInt();
    }

    //寻找座位
    QJsonArray seats = GetSeat(search_seats, passengers.size(), seat_type, train_id, departure_order, arrival_order, total_order, date);
    qDebug() << "seat:" << seats;

    //改座位
    QString ocupied_byte = GenerateOcupiedByte(departure_order, arrival_order, total_order);
    double current_price = 0.00;
    bool ok2 = false;
    for (int i = 0; i < passengers.size(); i++)
    {
        QJsonObject passenger = passengers.at(i).toObject();

        QJsonObject seat = seats.at(i).toObject();
        int coach_id = seat.value("车厢号").toInt();
        QString seat_id = seat.value("座位号").toString();
        passenger.insert("座位", seat);
        passengers.replace(i, passenger);

        QString id_number = passenger.value("身份证号").toString();
        QString ticket_type = passenger.value("类型").toString();
        double price = GetPrice(seat_type, ticket_type);
        current_price = price;
        QSqlQuery insert_order_passenger_sql;
        insert_order_passenger_sql.prepare("insert into order_passenger values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        insert_order_passenger_sql.addBindValue(order_id);
        insert_order_passenger_sql.addBindValue(id_number);
        insert_order_passenger_sql.addBindValue(train_id);
        insert_order_passenger_sql.addBindValue(departure_station);
        insert_order_passenger_sql.addBindValue(arrival_station);
        insert_order_passenger_sql.addBindValue(coach_id);
        insert_order_passenger_sql.addBindValue(seat_id);
        insert_order_passenger_sql.addBindValue(date);
        insert_order_passenger_sql.addBindValue(price);
        insert_order_passenger_sql.addBindValue("未付款");
        insert_order_passenger_sql.addBindValue(ticket_type);
        insert_order_passenger_sql.exec();
        QSqlQuery update_seat_sql;
        update_seat_sql.prepare("update seat set free = free & ? where train_id = ? and coach_id = ? and seat_id = ? and date = ?");
        qDebug() << ocupied_byte;
        update_seat_sql.addBindValue(ocupied_byte);
        update_seat_sql.addBindValue(train_id);
        update_seat_sql.addBindValue(coach_id);
        update_seat_sql.addBindValue(seat_id);
        update_seat_sql.addBindValue(date);
        ok2 = update_seat_sql.exec();
    }

    //更新订单总价
    QSqlQuery update_price_sql;
    bool ok3 = update_price_sql.exec("update order_info as po set total_price = (select sum(price) from order_passenger where po.order_id = order_id) where order_id = '" + order_id + "'");


    

    QString datetime_now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    //QJsonObject send_json{
    //    {"订单号", order_id},
    //    {"订单时间", datetime_now},
    //    {"车次号", train_id},
    //    {"出发站", departure_station},
    //    {"到达站", arrival_station},
    //    {"座位类型", seat_type},
    //    {"出发日期", date},
    //    {"订单状态", "未付款"},
    //    {"总价", 0},
    //    {"乘车人", passengers},
    //    {"消息", "成功"},
    //};
    
    QJsonArray orders;
    QString name = passengers.at(0).toObject().value("姓名").toString();
    QString id_number = passengers.at(0).toObject().value("身份证号").toString();
    QString ticket_type = passengers.at(0).toObject().value("类型").toString();
    int coach_id = passengers.at(0).toObject().value("座位").toObject().value("车厢号").toInt();
    QString seat_id = passengers.at(0).toObject().value("座位").toObject().value("座位号").toString();
    QJsonObject order{
        {"姓名", name},
        {"身份证号", id_number},
        {"类型", ticket_type},
        {"车次号", train_id},
        {"出发站", departure_station},
        {"到达站", arrival_station},
        {"出发日期", date},
        {"车厢号", coach_id},
        {"座位号", seat_id},
        {"座位类型", seat_type},
        {"价格", current_price},
        {"状态", "未付款"},
    };
    orders.append(order);


    QJsonObject send_json{
        {"订单号", order_id},
        {"订单时间", datetime_now},
        {"车票", orders},
        {"订单状态", "已完成"},
        {"总价", current_price},
    };
    if (ok1 && ok2 && ok3)
    {
        db.commit();
        send_json.insert("消息", "成功");
    }
    else
    {
        db.rollback();
        send_json.insert("消息", "失败");
    }
    return send_json;
}

QJsonObject ServerThread::Transfer(QJsonObject get_json)
{
    QString departure_station = get_json.take("出发站").toString();
    QString arrival_station = get_json.take("到达站").toString();
    QString date = get_json.take("日期").toString();
    QSqlQuery search_station_station_sql;

    //查找城市
    QString departure_city;
    QString arrival_city;
    search_station_station_sql.exec("select city from station where station_id = '" + departure_station + "' or city = '" + departure_station + "'");
    if (search_station_station_sql.next())
    {
        departure_city = search_station_station_sql.value("city").toString();
    }
    search_station_station_sql.exec("select city from station where station_id = '" + arrival_station + "' or city = '" + arrival_station + "'");
    if (search_station_station_sql.next())
    {
        arrival_city = search_station_station_sql.value("city").toString();
    }

    //找方案
    QJsonArray transfer_ways;
    QSqlQuery tranfer_sql;
    tranfer_sql.prepare("select * from " + 
        QString("(select pa.train_id as first_train_id, pa.station_id as first_departure_station, pa.start_time as first_start_time, pa.arrival_order as first_departure_order, pb.station_id as first_arrival_station, pb.arrival_time as first_arrival_time, pb.arrival_order as first_arrival_order, pb.city as first_arrival_city from (pass_by natural join station) as pa join (pass_by natural join station) as pb on pa.train_id = pb.train_id where pa.arrival_order < pb.arrival_order and pa.city = ?) ")+
        QString("as ta ") + 
        QString("join ") +
        QString("(select pc.train_id as second_train_id, pc.station_id as second_departure_station, pc.start_time as second_start_time, pc.arrival_order as second_departure_order, pd.station_id as second_arrival_station, pd.arrival_time as second_arrival_time, pc.city as second_start_city, pd.arrival_order as second_arrival_order, pd.day as arrival_day from (pass_by natural join station) as pc join (pass_by natural join station) as pd on pc.train_id = pd.train_id where pc.arrival_order < pd.arrival_order and pd.station_id = ?) ") +
        QString("as tb ") + 
        QString("on ta.first_arrival_city = tb.second_start_city ") +
        QString("where ta.first_arrival_time < tb.second_start_time or (ta.first_arrival_time > tb.second_start_time and tb.arrival_day > 0) ") + 
        QString("order by first_start_time"));
    tranfer_sql.addBindValue(departure_city);
    tranfer_sql.addBindValue(arrival_city);
    tranfer_sql.exec();
    while (tranfer_sql.next())
    {
        //第一程
        QString first_train_id = tranfer_sql.value("first_train_id").toString();
        QString first_departure_station = tranfer_sql.value("first_departure_station").toString();
        QString first_start_time = tranfer_sql.value("first_start_time").toString();
        int first_departure_order = tranfer_sql.value("first_deparutre_order").toInt();
        QString first_arrival_station = tranfer_sql.value("first_arrival_station").toString();
        QString first_arrival_time = tranfer_sql.value("first_arrival_time").toString();
        int first_arrival_order = tranfer_sql.value("first_arrival_order").toInt();

        //第二程
        QString second_train_id = tranfer_sql.value("second_train_id").toString();
        QString second_departure_station = tranfer_sql.value("second_departure_station").toString();
        QString second_start_time = tranfer_sql.value("second_start_time").toString();
        int second_departure_order = tranfer_sql.value("second_departure_order").toInt();
        QString second_arrival_station = tranfer_sql.value("second_arrival_station").toString();
        QString second_arrival_time = tranfer_sql.value("second_arrival_time").toString();
        int second_arrival_order = tranfer_sql.value("second_arrival_order").toInt();

        int arrival_day = tranfer_sql.value("arrival_day").toInt();

        //总次序
        QSqlQuery total_station_sql;
        total_station_sql.exec("select count(*) from pass_by where train_id = '" + first_train_id + "'");
        int first_total_station_num = 0;
        if (total_station_sql.next())
        {
            first_total_station_num = total_station_sql.value(0).toInt();
        }
        total_station_sql.exec("select count(*) from pass_by where train_id = '" + second_train_id + "'");
        int second_total_station_num = 0;
        if (total_station_sql.next())
        {
            second_total_station_num = total_station_sql.value(0).toInt();
        }

        //查询余票
        //查第一程
        QString first_free_byte = GenerateByte(first_departure_order, first_arrival_order, first_total_station_num);
        QSqlQuery free_count_sql;
        free_count_sql.prepare("select seat_type, count(*) from seat natural join coach "
            + QString("where train_id = ? and free = free | ? and date = ? group by seat_type"));
        free_count_sql.addBindValue(first_train_id);
        free_count_sql.addBindValue(first_free_byte);
        free_count_sql.addBindValue(date);
        free_count_sql.exec();
        QJsonObject first_free_seat_count;
        while (free_count_sql.next())
        {
            QString seat_type = free_count_sql.value("seat_type").toString();
            int seat_count = free_count_sql.value("count").toInt();
            first_free_seat_count.insert(seat_type, seat_count);
        }
        //查第二程
        QString second_free_byte = GenerateByte(second_departure_order, second_arrival_order, second_total_station_num);
        free_count_sql.prepare("select seat_type, count(*) from seat natural join coach "
            + QString("where train_id = ? and free = free | ? and date = ? group by seat_type"));
        free_count_sql.addBindValue(second_train_id);
        free_count_sql.addBindValue(second_free_byte);
        free_count_sql.addBindValue(date);
        free_count_sql.exec();
        QJsonObject second_free_seat_count;
        while (free_count_sql.next())
        {
            QString seat_type = free_count_sql.value("seat_type").toString();
            int seat_count = free_count_sql.value("count").toInt();
            second_free_seat_count.insert(seat_type, seat_count);
        }
        
        QJsonObject first_train{
            {"车次号", first_train_id},
            {"出发站", first_departure_station},
            {"出发时间", first_start_time},
            {"到达站", first_arrival_station},
            {"到达时间", first_arrival_time},
            {"余票", first_free_seat_count}
        };
        
        QJsonObject second_train{
            {"车次号", second_train_id},
            {"出发站", second_departure_station},
            {"出发时间", second_start_time},
            {"到达站", second_arrival_station},
            {"到达时间", second_arrival_time},
            {"余票", second_free_seat_count},
            {"到达日", arrival_day},
        };

        QJsonObject current_way{
            {"第一程", first_train},
            {"第二程", second_train}
        };
        qDebug() << current_way;
        transfer_ways.append(current_way);
    }

    QJsonObject send_json;
    if (transfer_ways.empty())
    {
        send_json.insert("消息", "失败");
    }
    else
    {
        send_json.insert("消息", "成功");
        send_json.insert("方案", transfer_ways);
    }

    return send_json;
}
