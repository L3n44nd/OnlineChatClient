#include "nameChangeDialog.h"
#include "wClient.h"

nameChangeDialog::nameChangeDialog(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	ui.changeNameField->setTextMargins(5, 0, 0, 0);

	connect(ui.changeNameBtn, &QPushButton::clicked, this, &nameChangeDialog::tryToChangeName);
	connect(qobject_cast<wClient*>(parent), &wClient::nameChangeAccepted, this, [this](QString info) {
		ui.changeNameLabel->setStyleSheet("color: #aaff7f");
		ui.changeNameLabel->setText(std::move(info));
		QTimer::singleShot(2000, [this]() {
			this->close();
			});
		});
	connect(qobject_cast<wClient*>(parent), &wClient::nameChangeRejected, this, [this](QString info){
		ui.changeNameLabel->setStyleSheet("color: #ffaa00");
		ui.changeNameLabel->setText(std::move(info));
		});
	connect(ui.cancelChangeBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void nameChangeDialog::tryToChangeName() {
	QString newName = ui.changeNameField->text();
	if (newName.isEmpty()) wClient::highlightFieldErr(ui.changeNameField);
	else if (newName.length() > 14) {
		ui.changeNameLabel->setStyleSheet("color: #ffaa00");
		ui.changeNameLabel->setText("Слишком длинное имя.");	
	}
	else emit changeNameBtnClicked(ui.changeNameField->text());
}

nameChangeDialog::~nameChangeDialog()
{}

