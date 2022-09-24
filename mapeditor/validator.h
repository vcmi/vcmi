#pragma once

#include <QDialog>
#include "../lib/mapping/CMap.h"

namespace Ui {
class Validator;
}

class Validator : public QDialog
{
	Q_OBJECT
public:
	struct Issue
	{
		QString message;
		bool critical;
		
		Issue(const QString & m, bool c): message(m), critical(c) {}
	};
	
public:
	explicit Validator(const CMap * map, QWidget *parent = nullptr);
	~Validator();
	
	static std::list<Issue> validate(const CMap * map);

private:
	Ui::Validator *ui;
};
