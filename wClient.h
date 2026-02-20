#pragma once

#include <QtNetwork/qtcpsocket.h>
#include <QString>
#include <qtimer.h>
#include "ui_wClient.h"
#include "\repos\wServer\Common\protocol.h"

class wClient : public QWidget
{
    Q_OBJECT

public:
    wClient(QWidget *parent = nullptr);
    ~wClient();
    static void highlightFieldErr(QLineEdit* field);

signals:
    void nameChangeAccepted(QString info);
    void nameChangeRejected(QString reason);
    void connectionLost();
    void connectionRestored();

private slots:
    void processServerResponse();
    void onErrorOccured(QAbstractSocket::SocketError);
    void loginBtnClicked();
    void regBtnClicked();
    void handleMessage(QString senderId, QString senderName, QString msg);
    void handlePrivateMessage(QString senderId, QString senderName, QString msg);
    void changeNameClicked();
    void logoutBtnClicked();

private:
    Ui::wClientClass ui;
    QTcpSocket socket;
    QTimer* reconnectTimer;

    void setupUI();
    void setupClient();
    void setupTimer(); 
};

