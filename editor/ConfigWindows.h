#pragma once
#include <QtGui>

class Editor;


class MapSpecifications: public QDialog
{
	Q_OBJECT
public:
	explicit MapSpecifications(QWidget *p = 0);

private:
	Editor * editor;
	QLineEdit *lineEdit;
};







