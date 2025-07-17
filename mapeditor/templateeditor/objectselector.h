/*
 * objectselector.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QWidget>

#include "../../lib/rmg/CRmgTemplate.h"

Q_DECLARE_METATYPE(CompoundMapObjectID);

namespace Ui {
class ObjectSelector;
}

class ObjectSelector : public QDialog
{
	Q_OBJECT

public:
	explicit ObjectSelector(ObjectConfig & obj);

	static void showObjectSelector(ObjectConfig & obj);

private slots:
	void on_buttonBoxResult_accepted();
    void on_buttonBoxResult_rejected();

private:
	Ui::ObjectSelector *ui;

	ObjectConfig & obj;
	
	std::map<CompoundMapObjectID, QString> advObjects;
	std::map<CompoundMapObjectID, QString> getAdventureMapItems();

	void fillBannedObjectCategories();
	void getBannedObjectCategories();
	void fillBannedObjects();
	void getBannedObjects();
	void fillCustomObjects();
	void getCustomObjects();
};
