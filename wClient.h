#pragma once

#include <QtNetwork/qtcpsocket.h>
#include <QString>
#include <QTabWidget>
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
    void processServerResponse(const QByteArray& utf8msg);
    void onErrorOccured(QAbstractSocket::SocketError);

    void loginBtnClicked();
    void regBtnClicked();
    void changeNameClicked();
    void logoutBtnClicked();
    void privateMsgBtnClicked(const QString& username);

    void handleMessage(QString senderName, QString msg);
    void handlePrivateMessage(QString senderId, QString senderName, QString msg);
    void sendMessage();
    void sendPacket(const clientQuery query, const QString& data = QString());
    void updateOnline(const QString& onlineList); 
    void loadHistory(const QString& history, QTextEdit* field);

private:
    bool waitingForDataSize = true;
    int sizeOfData = 0;
    Ui::wClientClass ui;
    QTcpSocket socket;
    QTimer* reconnectTimer;
    QHash<QString, int> onlineUsers;
    QHash<int, QTextEdit*> idToField;
    QHash<int, int> idToTabIndex;
    QHash<int, int> tabIndexToId;

    void setupUI();
    void setupClient();
    void setupTimer(); 
    void cleanUpLayout(QLayout* layout);
};

