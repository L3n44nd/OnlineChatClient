#include "wClient.h"

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
    ui.loginField->setTextMargins(5, 0, 0, 0); 
    ui.passwordField->setTextMargins(5, 0, 0, 0);
    ui.regPassField->setTextMargins(5, 0, 0, 0);
    ui.regLoginField->setTextMargins(5, 0, 0, 0);
    ui.regPassField2->setTextMargins(5, 0, 0, 0);
    
    connect(ui.registerBtn, &QPushButton::clicked, this, [this]() {
        ui.stackedWidget->setCurrentIndex(1);
        });
    connect(ui.backBtn, &QPushButton::clicked, this, [this]() {
        ui.stackedWidget->setCurrentIndex(0);
        });

    connect(ui.loginBtn, &QPushButton::clicked, this, &wClient::loginBtnClicked);
    connect(ui.regBtn, &QPushButton::clicked, this, &wClient::regBtnClicked);

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

void wClient::processServerResponse() {}

void wClient::loginBtnClicked() {
    QString username = ui.loginField->text();
    QString password = ui.passwordField->text();
    bool hasErr = false;

    if (username.isEmpty()) {
        ui.loginField->setStyleSheet("border: 2px solid red;");
        QTimer::singleShot(2000, ui.loginField, [this]() {
            ui.loginField->setStyleSheet("");
            });
        hasErr = true;
    }
    if (password.isEmpty()) {
        ui.passwordField->setStyleSheet("border: 2px solid red;");
        QTimer::singleShot(2000, ui.passwordField, [this]() {
            ui.passwordField->setStyleSheet("");
            });
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

    if (password1 != password2) {
        ui.regPassField->setStyleSheet("border: 2px solid red;");
        ui.regPassField2->setStyleSheet("border: 2px solid red;");
        QTimer::singleShot(2000, this, [this]() {
            ui.regPassField->setStyleSheet("");
            ui.regPassField2->setStyleSheet("");
            });
        return;
    }
}

wClient::~wClient()
{}

