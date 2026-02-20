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

private slots:
    void processServerResponse();
    void onErrorOccured(QAbstractSocket::SocketError);
    void loginBtnClicked();
    void regBtnClicked();
    void handleMessage(QString msg);
    void changeNameClicked();

private:
    Ui::wClientClass ui;
    QTcpSocket socket;
    QTimer* reconnectTimer;

    void setupUI();
    void setupClient();
    void setupTimer();
    
};

