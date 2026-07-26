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

    QRegularExpression expr("[^\\\\]*");
    ui.iField->setValidator(new QRegularExpressionValidator(expr, ui.iField));
    ui.loginField->setValidator(new QRegularExpressionValidator(expr, ui.loginField));
    ui.regLoginField->setValidator(new QRegularExpressionValidator(expr, ui.regLoginField));

    connect(ui.registerBtn, &QPushButton::clicked, this, [this]() {
        ui.stackedWidget->setCurrentIndex(1);
        });

    connect(ui.backBtn, &QPushButton::clicked, this, [this]() {
        ui.stackedWidget->setCurrentIndex(0);
        });

    connect(ui.tabChat, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (index != 0) { 
            int userId = tabIndexToId[index];
            tabIndexToId.remove(index);
            idToTabIndex.remove(userId);
            idToField.remove(userId);
            ui.tabChat->removeTab(index);
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
        ui.infoLabel->clear();
        ui.loginBtn->setEnabled(true);
        ui.registerBtn->setEnabled(true);
        ui.regBtn->setEnabled(true);
        });

    connect(&socket, &QTcpSocket::readyRead, this, [this]() {
        while (true) {
            if (waitingForDataSize) {
                if (socket.bytesAvailable() < 4) return;
                QByteArray sizeBytes = socket.read(4);
                QDataStream stream(sizeBytes);
                stream >> sizeOfData;
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

    socket.connectToHost(QHostAddress::LocalHost, 1403);
}

void wClient::setupTimer() {
    reconnectTimer = new QTimer(this);
    reconnectTimer->setInterval(3000);
    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        if (socket.state() == QTcpSocket::UnconnectedState) {
            socket.connectToHost(QHostAddress::LocalHost, 1403);
            ui.infoLabel->setText("Попытка подключения...");
        }
        });
}

void wClient::onErrorOccured(QAbstractSocket::SocketError error) {
    emit connectionLost();
    reconnectTimer->start();

    ui.statusLabel->clear();
    ui.statusLabel2->clear();
    ui.chatField->clear();
    ui.regLoginField->clear();
    ui.regPassField->clear();
    ui.regPassField2->clear();
    ui.iField->clear();

    QString errInfo;
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
    ui.infoLabel->setText(errInfo);

    ui.stackedWidget->setCurrentIndex(0);
    cleanUpTabs();
}

void wClient::processServerResponse(const QByteArray& utf8msg) {
    QString strmsg = QString::fromUtf8(utf8msg);
    int code = strmsg.section(' ', 0, 0).toInt();
    QString data = strmsg.section(' ', 1);
    serverResponse response = static_cast<serverResponse>(code);

    switch (response)
    {
    case serverResponse::Successful:
        emit nameChangeAccepted(toStr(serverResponse::Successful));
        ui.nameField->setText(data);
        break;
    case serverResponse::Registered: {
        ui.statusLabel2->setStyleSheet("color: #aaff7f; font: 700 9pt 'Century Gothic'");
        ui.statusLabel2->setText(toStr(serverResponse::Registered));

        QString uid = data.section('\n', 0, 0);
        QString username = data.section('\n', 1);
        ui.uidField->setText(uid);
        ui.nameField->setText(username);

        sendPacket(clientQuery::GetHistory, 0);
        QTimer::singleShot(2000, this, [this]() {
            ui.stackedWidget->setCurrentIndex(2);
            });
        break;
    }
    case serverResponse::LoginOK: {
        ui.statusLabel->setStyleSheet("color: #aaff7f; font: 700 9pt 'Century Gothic'");
        ui.statusLabel->setText(toStr(serverResponse::LoginOK));

        QString uid = data.section('\n', 0, 0);
        QString username = data.section('\n', 1);
        ui.uidField->setText(uid);
        ui.nameField->setText(username);

        sendPacket(clientQuery::GetHistory, 0);
        QTimer::singleShot(2000, this, [this]() {
            ui.stackedWidget->setCurrentIndex(2);
            });
        break;
    }
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
    case serverResponse::AlreadyAuthorized:
        ui.statusLabel->setStyleSheet("color: #ffaa00; font: 700 9pt 'Century Gothic'");
        ui.statusLabel->setText(toStr(serverResponse::AlreadyAuthorized));
        break;
    case serverResponse::Message: {
        QString sender = data.section('\n', 0, 0);
        QString msg = data.section('\n', 1);
        handleMessage(sender, msg);
        break;
    }
    case serverResponse::PrivateMessage: {
        QString senderId = data.section('\n', 0, 0);
        QString senderName = data.section('\n', 1, 1);
        QString msg = data.section('\n', 2);
        handlePrivateMessage(senderId, senderName, msg);
        break;
    }
    case serverResponse::UpdateOnline:
        updateOnline(data);
        break;
    case serverResponse::SendHistory: {
        int userId = data.section('\n', 0, 0).toInt();
        if (userId == 0) {
            loadHistory(data.section('\n', 1), ui.chatField);
            break;
        }
        if (!idToField.contains(userId)) break;
        QTextEdit* field = idToField[userId];
        loadHistory(data.section('\n', 1), field);
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

    QString query = QString("%1\n%2").arg(username).arg(password);
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

    QString query = QString("%1\n%2").arg(username).arg(password1); 
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
    socket.disconnectFromHost();

    cleanUpTabs();
    ui.loginField->clear();
    ui.passwordField->clear();
    ui.statusLabel->clear();
    ui.statusLabel2->clear();
    ui.chatField->clear();
    ui.regLoginField->clear();
    ui.regPassField->clear();
    ui.regPassField2->clear();
    ui.iField->clear();

    ui.stackedWidget->setCurrentIndex(0);
    reconnectTimer->start();
}

void wClient::cleanUpTabs() {
    if (tabIndexToId.isEmpty()) return;

    QList<int> tabIndexes = tabIndexToId.keys();
    std::sort(tabIndexes.begin(), tabIndexes.end());

    for (int i = tabIndexes.size() - 1; i >= 0; --i) {
        ui.tabChat->removeTab(tabIndexes[i]);
    }
    tabIndexToId.clear();
    onlineUsers.clear();
    idToField.clear();
    idToTabIndex.clear();
}

void wClient::privateMsgBtnClicked(const QString& username) {
    int recipientId = onlineUsers[username];

    if (!idToTabIndex.contains(recipientId)) {
        QWidget* newTab = new QWidget(this);
        QTextEdit* oField = new QTextEdit(newTab);

        oField->setReadOnly(true);
        oField->setFixedSize(680, 460);
        oField->setStyleSheet(
            "background-color: rgb(140, 127, 108);"
            "color: rgb(220, 211, 192);"
        );         
        int index = ui.tabChat->addTab(newTab, username);
        idToTabIndex[recipientId] = index;
        tabIndexToId[index] = recipientId;
        idToField[recipientId] = oField;

        sendPacket(clientQuery::GetHistory, QString::number(recipientId));
    }
    ui.tabChat->setCurrentIndex(idToTabIndex[recipientId]);
}

void wClient::handleMessage(QString senderName, QString msg) {
    QString textForChat = QString("<font color='#3b2e24'>%1:</font> %2").arg(senderName).arg(msg);
    ui.chatField->append(textForChat);
}

void wClient::handlePrivateMessage(QString senderId, QString senderName, QString msg) {
    QString textForChat = QString("<font color='#3b2e24'>%1</font>: %2").arg(senderName).arg(msg);
    if (idToTabIndex.contains(senderId.toInt())) {
        QTextEdit* targetField = idToField[senderId.toInt()];
        targetField->append(textForChat);
    }
    else {
        QWidget* newTab = new QWidget(this);
        QTextEdit* oField = new QTextEdit(newTab);

        oField->setReadOnly(true);
        oField->setFixedSize(680, 460);
        oField->setStyleSheet(
            "background-color: rgb(140, 127, 108);"
            "color: rgb(220, 211, 192);"
        );

        int index = ui.tabChat->addTab(newTab, senderName);
        idToTabIndex[senderId.toInt()] = index;
        tabIndexToId[index] = senderId.toInt();
        idToField[senderId.toInt()] = oField;

        sendPacket(clientQuery::GetHistory, senderId);
    }
}

void wClient::sendMessage() {
    QString textFromField = ui.iField->text();
    if (textFromField.isEmpty()) {
        wClient::highlightFieldErr(ui.iField);
        return;
    }

    QString selfName = ui.nameField->text();
    QString textForChat = QString("<font color='#aa0000'>%1 (Вы):</font> %2").arg(selfName).arg(textFromField);

    if (ui.tabChat->currentIndex() == 0) {
        sendPacket(clientQuery::Message, textFromField);
        ui.chatField->append(textForChat);
    }
    else {
        int recipientId = tabIndexToId[ui.tabChat->currentIndex()];
        QString query = QString("%1\n%2").arg(QString::number(recipientId)).arg(textFromField);

        sendPacket(clientQuery::PrivateMessage, query);
        idToField[recipientId]->append(textForChat);
    }
    ui.iField->clear();
}

void wClient::sendPacket(const clientQuery query, const QString& data) {
    int queryCode = static_cast<int>(query);
    QString formatedData = data.isEmpty() ? QString::number(queryCode) : QString("%1 %2").arg(queryCode).arg(data);
    QByteArray bArrData = formatedData.toUtf8();

    qint32 dataSize = bArrData.size();
    QByteArray packet;
    QDataStream stream(&packet, QIODeviceBase::WriteOnly);
    stream << dataSize;
    packet += bArrData;
    socket.write(packet);
}

void wClient::updateOnline(const QString& onlineList) {

    QStringList parts = onlineList.split("\n\n");
    int selfId = (ui.uidField->text()).toInt();

    onlineUsers.clear();
    cleanUpLayout(ui.onlineLayout);

    for (int i = 0; i < parts.size(); ++i) {
        int userId = parts[i].section('\n', 0, 0).toInt();
        if (selfId == userId) continue;

        QString username = parts[i].section('\n', 1);
        onlineUsers[username] = userId;
        if (idToTabIndex.contains(userId)) ui.tabChat->setTabText(idToTabIndex[userId], username);

        QPushButton* writeBtn = new QPushButton(username, this);
        connect(writeBtn, &QPushButton::clicked, this, [username, this]() {
            privateMsgBtnClicked(username);
            });
        writeBtn->setStyleSheet(
            "font: 700 9pt 'Century Gothic';"
            "color: rgb(255, 231, 197);"
            "background-color: rgb(126, 114, 97);"
            "border: none;"
        );
        writeBtn->setFixedSize(180, 20);
        ui.onlineLayout->addWidget(writeBtn);
        ui.onlineLayout->addStretch();
    }
}

void wClient::loadHistory(const QString& history, QTextEdit* field) {
    if (history.isEmpty()) return;
    QStringList parts = history.split("\n\n");
    for (int i = 0; i < parts.size(); ++i) {
        int senderId = parts[i].section('\n', 0, 0).toInt();
        QString senderName = parts[i].section('\n', 1, 1);
        QString msg = parts[i].section('\n', 2);

        if (senderId == ui.uidField->text().toInt()) field->append(QString("<font color='#aa0000'>%1 (Вы):</font> %2").arg(senderName).arg(msg));
        else field->append(QString("<font color='#3b2e24'>%1:</font> %2").arg(senderName).arg(msg));
    }
}

void wClient::highlightFieldErr(QLineEdit* field) {
    QString fieldStyle = field->styleSheet();
    QString err = fieldStyle + "border: 2px solid #b90031; border-radius: 3px";
    field->setStyleSheet(err);
    QTimer::singleShot(2000, field, [field, fieldStyle]() {
        field->setStyleSheet(fieldStyle);
        });
}

void wClient::cleanUpLayout(QLayout* layout) {
    for (int i = layout->count() - 1; i >= 0; --i) {
        QLayoutItem* item = layout->itemAt(i);
        delete item->widget();
        delete layout->takeAt(i);
    }
}

wClient::~wClient() {}

