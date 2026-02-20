#pragma once

#include <QDialog>
#include "ui_nameChangeDialog.h"
#include <QString>

class nameChangeDialog : public QDialog
{
	Q_OBJECT

public:
	nameChangeDialog(QWidget *parent = nullptr);
	~nameChangeDialog();

signals:
	void changeNameBtnClicked(QString newName);

private slots:
	void tryToChangeName();

private:
	Ui::nameChangeDialogClass ui;
};

