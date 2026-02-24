#include "wClient.h"
#include "nameChangeDialog.h"
#include "OnlineWidget.h"

wClient::wClient(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupClient();
    setupTimer();
}

void wClient::setupUI() {
    ui.setupUi(this);
    this->setLayout(ui.mainLayout);
    ui.stackedWidget->setCurrentIndex(0);
    
    connect(ui.registerBtn, &QPushButton::clicked, this, [this]() {
        ui.stackedWidget->setCurrentIndex(1);
        });

    connect(ui.backBtn, &QPushButton::clicked, this, [this]() {
        ui.stackedWidget->setCurrentIndex(0);
        });

    connect(ui.tabChat, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (index != 0) {
            QString username = ui.tabChat->tabText(index);
            int userId = onlineUsers[username];
            ui.tabChat->removeTab(index);
            idToTabIndex.remove(userId);
            idToField.remove(userId);
        }
        });

    connect(ui.regBtn, &QPushButton::clicked, this, &wClient::regBtnClicked);
    connect(ui.regLoginField, &QLineEdit::returnPressed, this, &wClient::regBtnClicked);
    connect(ui.regPassField, &QLineEdit::returnPressed, this, &wClient::regBtnClicked);
    connect(ui.regPassField2, &QLineEdit::returnPressed, this, &wClient::regBtnClicked);

    connect(ui.loginBtn, &QPushButton::clicked, this, &wClient::loginBtnClicked);
    connect(ui.loginField, &QLineEdit::returnPressed, this, &wClient::loginBtnClicked);
    connect(ui.passwordField, &QLineEdit::returnPressed, this, &wClient::loginBtnClicked);

    connect(ui.sendBtn, &QPushButton::clicked, this, &wClient::sendMessage);
    connect(ui.iField, &QLineEdit::returnPressed, this, &wClient::sendMessage);

    connect(ui.nameChangeBtn, &QPushButton::clicked, this, &wClient::changeNameClicked);

    connect(ui.logoutBtn, &QPushButton::clicked, this, &wClient::logoutBtnClicked);

}

void wClient::setupClient() {
    connect(&socket, &QTcpSocket::connected, this, [this]() {
        reconnectTimer->stop();
        emit connectionRestored();
        ui.infoLabel->clear();
        ui.infoLabel2->clear();
        ui.loginBtn->setEnabled(true);
        ui.registerBtn->setEnabled(true);
        ui.regBtn->setEnabled(true);
        ui.sendBtn->setEnabled(true);
        ui.statusField3->hide();
        });

    connect(&socket, &QTcpSocket::readyRead, this, [this]() {
        while (true) {
            if (waitingForDataSize) {
                if (socket.bytesAvailable() < 4) return;
                sizeOfData = (socket.read(4)).toInt();
                waitingForDataSize = false;
            }
            else {
                if (socket.bytesAvailable() < sizeOfData) return;
                QByteArray data = socket.read(sizeOfData);
                processServerResponse(data);
                waitingForDataSize = true;
            }
        }
        });
    connect(&socket, &QTcpSocket::errorOccurred, this, &wClient::onErrorOccured);
    connect(&socket, &QTcpSocket::disconnected, this, [this]() {
        emit connectionLost();
        });

    socket.connectToHost(QHostAddress::LocalHost, 1402);
}

void wClient::setupTimer() {
    reconnectTimer = new QTimer(this);
    reconnectTimer->setInterval(3000);
    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        int currPageIndex = ui.stackedWidget->currentIndex();
        if (socket.state() == QTcpSocket::UnconnectedState) {
            socket.connectToHost(QHostAddress::LocalHost, 1402);
            if (currPageIndex == 0) ui.infoLabel->setText("Попытка подключения...");
            else if (currPageIndex == 1) ui.infoLabel2->setText("Попытка подключения...");
        }
        });
}

void wClient::onErrorOccured(QAbstractSocket::SocketError error) {
    QString errInfo;
    int currPageIndex = ui.stackedWidget->currentIndex();

    emit connectionLost();
    reconnectTimer->start();

    ui.registerBtn->setEnabled(false);
    ui.regBtn->setEnabled(false);
    ui.loginBtn->setEnabled(false);
    ui.sendBtn->setEnabled(false);

    switch (error)
    {
    case QAbstractSocket::ConnectionRefusedError:
        errInfo = "Не удалось подключиться к серверу. Возможно, он отключён.";
        break;
    case QAbstractSocket::HostNotFoundError:
        errInfo = "Сервер не найден.";
        break;
    case QAbstractSocket::NetworkError:
        errInfo = "Ошибка сети. Проверьте подключение.";
        break;
    default:
        errInfo = "Не удалось подключиться к серверу.";
        break;
    }
    if (currPageIndex == 0) ui.infoLabel->setText(errInfo);
    else if (currPageIndex == 1) ui.infoLabel2->setText(errInfo);
    else {
        ui.statusField3->setText(errInfo);
        ui.statusField3->show();
    }
}

void wClient::processServerResponse(const QByteArray& utf8msg) {
    QString strmsg = QString::fromUtf8(utf8msg);
    QStringList parts = strmsg.split(' ');
    int code = parts[0].toInt();
    serverResponse response = static_cast<serverResponse>(code);

    switch (response)
    {
    case serverResponse::Successful:
        emit nameChangeAccepted(toStr(serverResponse::Successful));
        ui.nameField->setText(strmsg.section(' ', 1));
        break;
    case serverResponse::Registered:
        ui.statusLabel2->setStyleSheet("color: #aaff7f");
        ui.statusLabel2->setText(toStr(serverResponse::Registered));
        ui.uidField->setText(parts[1]);
        ui.nameField->setText(parts[2]);
        sendPacket(clientQuery::GetHistory, 0);
        QTimer::singleShot(2000, this, [this, parts]() {
            ui.stackedWidget->setCurrentIndex(2);
            });
        break;
    case serverResponse::LoginOK:
        ui.statusLabel->setStyleSheet("color: #aaff7f; font: 700 9pt 'Century Gothic'");
        ui.statusLabel->setText(toStr(serverResponse::LoginOK));
        ui.uidField->setText(parts[1]);
        ui.nameField->setText(parts[2]);
        sendPacket(clientQuery::GetHistory, 0);
        QTimer::singleShot(2000, this, [this, parts]() {     
            ui.stackedWidget->setCurrentIndex(2);
            });
        break;
    case serverResponse::WrongPassword:
        ui.statusLabel->setStyleSheet("color: #ffaa00; font: 700 9pt 'Century Gothic'");
        ui.statusLabel->setText(toStr(serverResponse::WrongPassword));
        break;
    case serverResponse::UserNotFound:
        ui.statusLabel->setStyleSheet("color: #ffaa00; font: 700 9pt 'Century Gothic'");
        ui.statusLabel->setText(toStr(serverResponse::UserNotFound));
        break;
    case serverResponse::UsernameExists:
        if (ui.stackedWidget->currentIndex() == 1) {
            ui.statusLabel2->setStyleSheet("color: #ffaa00; font: 700 9pt 'Century Gothic'");
            ui.statusLabel2->setText(toStr(serverResponse::UsernameExists));
        }
        else emit nameChangeRejected(toStr(serverResponse::UsernameExists));
        break;
    case serverResponse::NameTooLong:
        emit nameChangeRejected(toStr(serverResponse::NameTooLong));
        break;
    case serverResponse::Message:
        handleMessage(parts[1], parts[2]);
        break;
    case serverResponse::PrivateMessage:
        handlePrivateMessage(parts[1], parts[2], parts[3]);
        break;
    case serverResponse::UpdateOnline:
        updateOnline(strmsg.section(' ', 1));
        break;
    case serverResponse::SendHistory: {
        int userId = strmsg.section(' ', 1, 1).toInt();
        if (userId == 0) {
            loadHistory(strmsg.section(' ', 2), ui.chatField);
            break;
        }
        if (!idToField.contains(userId)) break;
        QTextEdit* field = idToField[userId];
        loadHistory(strmsg.section(' ', 2), field);
        break;
    }
    default:
        break;
    }
}

void wClient::loginBtnClicked() {
    QString username = ui.loginField->text();
    QString password = ui.passwordField->text();
    bool hasErr = false;

    if (username.isEmpty()) {
        highlightFieldErr(ui.loginField);
        hasErr = true;
    }
    if (password.isEmpty()) {
        highlightFieldErr(ui.passwordField);
        hasErr = true;
    }
    if (hasErr) return;

    QString query = QString("%1 %2").arg(username).arg(password);
    sendPacket(clientQuery::Login, query);
}

void wClient::regBtnClicked() {
    QString username = ui.regLoginField->text();
    QString password1 = ui.regPassField->text();
    QString password2 = ui.regPassField2->text();
    bool hasErr = false;

    if (password1 != password2 || password1.isEmpty()) {
        highlightFieldErr(ui.regPassField);
        highlightFieldErr(ui.regPassField2);
        hasErr = true;
    }
    if (username.isEmpty()) {
        highlightFieldErr(ui.regLoginField);
        hasErr = true;
    }
    if (hasErr) return;

    if (password1.length() < 8) {
        ui.statusLabel2->setText("<font color='#ffaa00'>Слишком короткий пароль</font>");
        return;
    }

    QString query = QString("%1 %2").arg(username).arg(password1); 
    sendPacket(clientQuery::Register, query);
}

void wClient::changeNameClicked() {
    nameChangeDialog dialog(this);
    connect(&dialog, &nameChangeDialog::changeNameBtnClicked, this, [this](QString newName) {
        sendPacket(clientQuery::NameChange, newName);
        });
    dialog.exec();
}

void wClient::logoutBtnClicked() {
    ui.loginField->clear();
    ui.passwordField->clear();
    ui.statusLabel->clear();
    ui.statusLabel2->clear();
    ui.stackedWidget->setCurrentIndex(0);
    ui.chatField->clear();

    sendPacket(clientQuery::Logout);
}

void wClient::privateMsgBtnClicked(const QString& username) {
    int recipientId = onlineUsers[username];

    if (idToTabIndex.find(recipientId) == idToTabIndex.end()) {
        QWidget* newTab = new QWidget(this);
        QTextEdit* oField = new QTextEdit(newTab);

        oField->setReadOnly(true);
        oField->setFixedSize(680, 460);

        idToTabIndex[recipientId] = ui.tabChat->addTab(newTab, username);
        idToField[recipientId] = oField;
        sendPacket(clientQuery::GetHistory, QString::number(recipientId));
    }
    ui.tabChat->setCurrentIndex(idToTabIndex[recipientId]);
}

void wClient::handleMessage(QString senderName, QString msg) {
    QString textForChat = QString("<font color='#4a875d'>%1</font>: %2").arg(senderName).arg(msg);
    ui.chatField->append(textForChat);
}

void wClient::handlePrivateMessage(QString senderId, QString senderName, QString msg) {
    QString textForChat = QString("<font color='#4a875d'>%1</font>: %2").arg(senderName).arg(msg);
    if (idToField.find(senderId.toInt()) != idToField.end()) {
        QTextEdit* targetField = idToField[senderId.toInt()];
        targetField->append(textForChat);
    }
    else {
        QWidget* newTab = new QWidget(this);
        QTextEdit* oField = new QTextEdit(newTab);

        oField->setReadOnly(true);
        oField->setFixedSize(680, 460);
        oField->append(textForChat);

        idToTabIndex[senderId.toInt()] = ui.tabChat->addTab(newTab, senderName);
        idToField[senderId.toInt()] = oField;
    }
}

void wClient::sendMessage() {
    QString textFromField = ui.iField->text();
    if (textFromField.isEmpty()) {
        wClient::highlightFieldErr(ui.iField);
        return;
    }

    QString selfName = ui.nameField->text();
    QString textForChat = QString("<font color='#ff0033'>%1 (Вы):</font> %2").arg(selfName).arg(textFromField);

    if (ui.tabChat->currentIndex() == 0) {
        sendPacket(clientQuery::Message, textFromField);
        ui.chatField->append(textForChat);
        ui.iField->clear();
    }
    else {
        QString recipient = ui.tabChat->tabText(ui.tabChat->currentIndex());
        int recipientId = onlineUsers[recipient];
        QString query = QString("%1 %2").arg(QString::number(recipientId)).arg(textFromField);

        sendPacket(clientQuery::PrivateMessage, query);
        idToField[recipientId]->append(textForChat);
        ui.iField->clear();
    }
}

void wClient::sendPacket(const clientQuery query, const QString& data) {
    int queryCode = static_cast<int>(query);
    QString formatedData = data.isEmpty() ? QString::number(queryCode) : QString("%1 %2").arg(queryCode).arg(data);
    QByteArray bArrData = formatedData.toUtf8();
    int dataSize = bArrData.size();
    QByteArray packet = QByteArray::number(dataSize).rightJustified(4, '0') + bArrData;
    socket.write(packet);
}

void wClient::updateOnline(const QString& onlineList) {
    if (onlineList.isEmpty()) return;

    QStringList parts = onlineList.split('\n');
    int selfId = (ui.uidField->text()).toInt();
    ui.listOnline->clear();
    onlineUsers.clear();

    for (int i = 0; i < parts.size(); ++i) {
        int userId = parts[i].section(' ', 0, 0).toInt();
        QString username = parts[i].section(' ', 1);
        onlineUsers[username] = userId;

        bool isSelf = selfId == userId;
        QListWidgetItem* item = new QListWidgetItem(ui.listOnline);
        OnlineWidget* onlineUserWidget = new OnlineWidget(username, this, isSelf);
        if (!isSelf) connect(onlineUserWidget, &OnlineWidget::writeBtnClicked, this, &wClient::privateMsgBtnClicked);
        item->setSizeHint(onlineUserWidget->sizeHint());
        ui.listOnline->setItemWidget(item, onlineUserWidget);
    }
}

void wClient::loadHistory(const QString& history, QTextEdit* field){
    if (history.isEmpty()) return;
    QStringList parts = history.split('\n');
    for (int i = 0; i < parts.size(); ++i) {
        int senderId = parts[i].section(' ', 0, 0).toInt();
        QString senderName = parts[i].section(' ', 1, 1);
        QString msg = parts[i].section(' ', 2);
        if (senderId == ui.uidField->text().toInt()) field->append(QString("<font color='#ff0033'>%1 (Вы):</font> %2").arg(senderName).arg(msg));
        else field->append(QString("<font color='#4a875d'>%1:</font> %2").arg(senderName).arg(msg));
    }
}

void wClient::highlightFieldErr(QLineEdit* field) {
    field->setStyleSheet("border: 1px solid red;");
    QTimer::singleShot(2000, field, [field]() {
        field->setStyleSheet("");
        });
}

wClient::~wClient() {}

