/*
 * validator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>

VCMI_LIB_NAMESPACE_BEGIN
class CMap;
VCMI_LIB_NAMESPACE_END

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
