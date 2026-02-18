#pragma once

#include <QtNetwork/qtcpsocket.h>
#include <qstring.h>
#include <qtimer.h>
#include "ui_wClient.h"
#include "\repos\wServer\Common\protocol.h"

class wClient : public QWidget
{
    Q_OBJECT

public:
    wClient(QWidget *parent = nullptr);
    ~wClient();

private slots:
    void processServerResponse();
    void onErrorOccured(QAbstractSocket::SocketError);
    void loginBtnClicked();
    void regBtnClicked();

private:
    Ui::wClientClass ui;
    QTcpSocket socket;
    QTimer* reconnectTimer;

    void setupUI();
    void setupClient();
    void setupTimer();
    void highlightFieldErr(QLineEdit* field);
};

