#include "nameChangeDialog.h"
#include "wClient.h"

nameChangeDialog::nameChangeDialog(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	
	QRegularExpression expr("[^\\\\]*");
	ui.changeNameField->setValidator(new QRegularExpressionValidator(expr, ui.changeNameBtn));

	connect(ui.changeNameBtn, &QPushButton::clicked, this, &nameChangeDialog::tryToChangeName);
	connect(ui.cancelChangeBtn, &QPushButton::clicked, this, &QDialog::reject);

	connect(qobject_cast<wClient*>(parent), &wClient::nameChangeAccepted, this, [this](QString info) {
		ui.changeNameLabel->setStyleSheet("color: #aaff7f; font: 700 9pt 'Century Gothic'");
		ui.changeNameLabel->setText(info);
		QTimer::singleShot(2000, this, [this]() {
			this->close();
			});
		});
	connect(qobject_cast<wClient*>(parent), &wClient::nameChangeRejected, this, [this](QString info) {
		ui.changeNameLabel->setStyleSheet("color: #ffaa00; font: 700 9pt 'Century Gothic'");
		ui.changeNameLabel->setText(info);
		});
	connect(qobject_cast<wClient*>(parent), &wClient::connectionLost, this, [this]() {
		QTimer::singleShot(100, this, [this]() {
			this->close();
			});
		});
}

void nameChangeDialog::tryToChangeName() {
	QString newName = ui.changeNameField->text();
	if (newName.isEmpty()) wClient::highlightFieldErr(ui.changeNameField);
	else emit changeNameBtnClicked(ui.changeNameField->text());
}

nameChangeDialog::~nameChangeDialog()
{}

