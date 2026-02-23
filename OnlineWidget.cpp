#include "OnlineWidget.h"
#include <QLayout>

OnlineWidget::OnlineWidget(const QString& name, QWidget* parent, bool isSelf)
	: QWidget(parent)
{
	QHBoxLayout* layout = new QHBoxLayout(this);
	nameLabel = new QLabel(name, this);
	layout->addWidget(nameLabel);

	if (!isSelf) {
		writeBtn = new QPushButton("Написать", this);
		writeBtn->setFixedSize(75, 20);
		layout->addWidget(writeBtn);
		connect(writeBtn, &QPushButton::clicked, this, [this, name](){
			emit writeBtnClicked(name);
	});
	}
}

OnlineWidget::~OnlineWidget()
{}

