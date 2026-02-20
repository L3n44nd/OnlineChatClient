#include "wClient.h"
#include "nameChangeDialog.h"

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

    connect(ui.nameChangeBtn, &QPushButton::clicked, this, &wClient::changeNameClicked);

    connect(ui.loginBtn, &QPushButton::clicked, this, &wClient::loginBtnClicked);
    connect(ui.loginField, &QLineEdit::returnPressed, this, &wClient::loginBtnClicked);
    connect(ui.passwordField, &QLineEdit::returnPressed, this, &wClient::loginBtnClicked);

    connect(ui.regBtn, &QPushButton::clicked, this, &wClient::regBtnClicked);
    connect(ui.regLoginField, &QLineEdit::returnPressed, this, &wClient::regBtnClicked);
    connect(ui.regPassField, &QLineEdit::returnPressed, this, &wClient::regBtnClicked);
    connect(ui.regPassField2, &QLineEdit::returnPressed, this, &wClient::regBtnClicked);

    connect(ui.tabChat, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (index != 0) {
            ui.tabChat->removeTab(index);
        }
        });

}

void wClient::setupClient() {
    connect(&socket, &QTcpSocket::connected, this, [this]() {
        reconnectTimer->stop();
        ui.infoLabel->clear();
        ui.infoLabel2->clear();
        ui.loginBtn->setEnabled(true);
        ui.registerBtn->setEnabled(true);
        ui.regBtn->setEnabled(true);
        });
    connect(&socket, &QTcpSocket::readyRead, this, &wClient::processServerResponse);
    connect(&socket, &QTcpSocket::errorOccurred, this, &wClient::onErrorOccured);

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

    reconnectTimer->start();

    ui.registerBtn->setEnabled(false);
    ui.regBtn->setEnabled(false);
    ui.loginBtn->setEnabled(false);

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
}

void wClient::processServerResponse() {
    QByteArray utf8msg = socket.readAll();
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
        QTimer::singleShot(2000, this, [this, parts]() {
            ui.uidField->setText(parts[1]);
            ui.nameField->setText(parts[2]);
            ui.stackedWidget->setCurrentIndex(2);
            });
        break;
    case serverResponse::LoginOK:
        ui.statusLabel->setStyleSheet("color: #aaff7f");
        ui.statusLabel->setText(toStr(serverResponse::LoginOK));
        QTimer::singleShot(2000, this, [this, parts]() {
            ui.uidField->setText(parts[1]);
            ui.nameField->setText(parts[2]);
            ui.stackedWidget->setCurrentIndex(2);
            });
        break;
    case serverResponse::WrongPassword:
        ui.statusLabel->setStyleSheet("color: #ffaa00");
        ui.statusLabel->setText(toStr(serverResponse::WrongPassword));
        break;
    case serverResponse::UserNotFound:
        ui.statusLabel->setStyleSheet("color: #ffaa00");
        ui.statusLabel->setText(toStr(serverResponse::UserNotFound));
        break;
    case serverResponse::UsernameExists:
        if (ui.stackedWidget->currentIndex() == 1) {
            ui.statusLabel2->setStyleSheet("color: #ffaa00");
            ui.statusLabel2->setText(toStr(serverResponse::UsernameExists));
        }
        else emit nameChangeRejected(toStr(serverResponse::UsernameExists));
        break;
    case serverResponse::NameTooLong:
        emit nameChangeRejected(toStr(serverResponse::NameTooLong));
        break;
    case serverResponse::Message:
        handleMessage("textMsg");
        break;
    case serverResponse::PrivateMessage:
        break;
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

    int qCode = static_cast<int>(clientQuery::Login);
    QString strMsg = QString("%1 %2 %3").arg(qCode).arg(username).arg(password);
    QByteArray bArrMsg = strMsg.toUtf8();
    socket.write(bArrMsg);
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

    int qCode = static_cast<int>(clientQuery::Register);
    QString strMsg = QString("%1 %2 %3").arg(qCode).arg(username).arg(password1);
    QByteArray bArrMsg = strMsg.toUtf8();
    socket.write(bArrMsg);
}

void wClient::changeNameClicked() {
    nameChangeDialog dialog(this);
    connect(&dialog, &nameChangeDialog::changeNameBtnClicked, this, [this](QString newName) {
        int qCode = static_cast<int>(clientQuery::NameChange);
        QString strQuery = QString("%1 %2").arg(qCode).arg(std::move(newName));
        QByteArray bArrQuery = strQuery.toUtf8();
        socket.write(bArrQuery);
        });
    dialog.exec();
}

void wClient::handleMessage(QString msg) {}

void wClient::highlightFieldErr(QLineEdit* field) {
    field->setStyleSheet("border: 2px solid red;");
    QTimer::singleShot(2000, field, [field]() {
        field->setStyleSheet("");
        });
}

wClient::~wClient()
{}


