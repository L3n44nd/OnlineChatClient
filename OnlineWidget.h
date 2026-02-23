#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
//#include "ui_OnlineWidget.h"

class OnlineWidget : public QWidget
{
	Q_OBJECT

public:
	OnlineWidget(const QString& name, QWidget* parent = nullptr, bool isSelf = false);
	~OnlineWidget();

signals:
	void writeBtnClicked(const QString& username);

private:
	//Ui::OnlineWidgetClass ui;
	QString username;
	QLabel* nameLabel;
	QPushButton* writeBtn = nullptr;
};

