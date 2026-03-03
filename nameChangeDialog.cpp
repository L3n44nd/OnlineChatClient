#include "nameChangeDialog.h"
#include "wClient.h"

nameChangeDialog::nameChangeDialog(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	ui.changeNameField->setMaxLength(15);

	connect(ui.changeNameBtn, &QPushButton::clicked, this, &nameChangeDialog::tryToChangeName);
	connect(qobject_cast<wClient*>(parent), &wClient::nameChangeAccepted, this, [this](QString info) {
		ui.changeNameLabel->setStyleSheet("color: #aaff7f; font: 700 9pt 'Century Gothic'");
		ui.changeNameLabel->setText(std::move(info));
		QTimer::singleShot(2000, [this]() {
			this->close();
			});
		});
	connect(qobject_cast<wClient*>(parent), &wClient::nameChangeRejected, this, [this](QString info){
		ui.changeNameLabel->setStyleSheet("color: #ffaa00; font: 700 9pt 'Century Gothic'");
		ui.changeNameLabel->setText(std::move(info));
		});
	connect(qobject_cast<wClient*>(parent), &wClient::connectionLost, this, [this]() {
		ui.changeNameLabel->setStyleSheet("color: #ffaa00; font: 700 9pt 'Century Gothic'");
		ui.changeNameLabel->setText("Сервер не отвечает");
		ui.changeNameBtn->setEnabled(false);
		});
	connect(qobject_cast<wClient*>(parent), &wClient::connectionRestored, this, [this]() {
		ui.changeNameLabel->clear();
		ui.changeNameBtn->setEnabled(true);
		});
	connect(ui.cancelChangeBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void nameChangeDialog::tryToChangeName() {
	QString newName = ui.changeNameField->text();
	if (newName.isEmpty()) wClient::highlightFieldErr(ui.changeNameField);
	else emit changeNameBtnClicked(ui.changeNameField->text());
}

nameChangeDialog::~nameChangeDialog()
{}

