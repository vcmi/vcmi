/*
 * objectselector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "objectselector.h"
#include "ui_objectselector.h"

#include "../../lib/rmg/ObjectConfig.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"

ObjectSelector::ObjectSelector(ObjectConfig & obj) :
	ui(new Ui::ObjectSelector),
	obj(obj)
{
	ui->setupUi(this);

	setWindowTitle(tr("Object Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	fillBannedObjectCategories();
	fillBannedObjects();
	fillCustomObjects();

	show();
}

void ObjectSelector::fillBannedObjectCategories()
{
	ui->tableWidgetBannedObjectCategories->setColumnCount(2);
	ui->tableWidgetBannedObjectCategories->setRowCount(obj.bannedObjectCategories.size() + 1);
	ui->tableWidgetBannedObjectCategories->setHorizontalHeaderLabels({tr("Category"), tr("Action")});

	auto addRow = [this](ObjectConfig::EObjectCategory category, int row){
		std::map<ObjectConfig::EObjectCategory, QString> values = {
			{ ObjectConfig::EObjectCategory::OTHER, tr("Other") },
			{ ObjectConfig::EObjectCategory::ALL, tr("All") },
			{ ObjectConfig::EObjectCategory::NONE, tr("None") },
			{ ObjectConfig::EObjectCategory::CREATURE_BANK, tr("Creature bank") },
			{ ObjectConfig::EObjectCategory::BONUS, tr("Bonus") },
			{ ObjectConfig::EObjectCategory::DWELLING, tr("Dwelling") },
			{ ObjectConfig::EObjectCategory::RESOURCE, tr("Resource") },
			{ ObjectConfig::EObjectCategory::RESOURCE_GENERATOR, tr("Resource generator") },
			{ ObjectConfig::EObjectCategory::SPELL_SCROLL, tr("Spell scroll") },
			{ ObjectConfig::EObjectCategory::RANDOM_ARTIFACT, tr("Random artifact") },
			{ ObjectConfig::EObjectCategory::PANDORAS_BOX, tr("Pandoras box") },
			{ ObjectConfig::EObjectCategory::QUEST_ARTIFACT, tr("Quest artifact") },
			{ ObjectConfig::EObjectCategory::SEER_HUT, tr("Seer hut") }
		};
		QComboBox *combo = new QComboBox();
		for(auto & item : values)
    		combo->addItem(item.second, QVariant(static_cast<int>(item.first)));

		int index = combo->findData(static_cast<int>(category));
		if (index != -1)
			combo->setCurrentIndex(index);

		ui->tableWidgetBannedObjectCategories->setCellWidget(row, 0, combo);

		auto deleteButton = new QPushButton("Delete");
		ui->tableWidgetBannedObjectCategories->setCellWidget(row, 1, deleteButton);
		connect(deleteButton, &QPushButton::clicked, this, [this, deleteButton]() {
			for (int r = 0; r < ui->tableWidgetBannedObjectCategories->rowCount(); ++r) {
				if (ui->tableWidgetBannedObjectCategories->cellWidget(r, 1) == deleteButton) {
					ui->tableWidgetBannedObjectCategories->removeRow(r);
					break;
				}
			}
		});
	};

	for (int row = 0; row < obj.bannedObjectCategories.size(); ++row)
		addRow(obj.bannedObjectCategories[row], row);

	auto addButton = new QPushButton("Add");
	ui->tableWidgetBannedObjectCategories->setCellWidget(ui->tableWidgetBannedObjectCategories->rowCount() - 1, 1, addButton);
	connect(addButton, &QPushButton::clicked, this, [this, addRow]() {
		ui->tableWidgetBannedObjectCategories->insertRow(ui->tableWidgetBannedObjectCategories->rowCount() - 1);
		addRow(ObjectConfig::EObjectCategory::ALL, ui->tableWidgetBannedObjectCategories->rowCount() - 2);
	});

	ui->tableWidgetBannedObjectCategories->resizeColumnsToContents();
	ui->tableWidgetBannedObjectCategories->setColumnWidth(0, 300);
}

void ObjectSelector::getBannedObjectCategories()
{
	obj.bannedObjectCategories.clear();
	for (int row = 0; row < ui->tableWidgetBannedObjectCategories->rowCount() - 1; ++row)
	{
		auto val = static_cast<ObjectConfig::EObjectCategory>(static_cast<QComboBox *>(ui->tableWidgetBannedObjectCategories->cellWidget(row, 0))->currentData().toInt());
		if(std::find(obj.bannedObjectCategories.begin(), obj.bannedObjectCategories.end(), val) == obj.bannedObjectCategories.end())
			obj.bannedObjectCategories.push_back(val);
	}
}

void ObjectSelector::fillBannedObjects()
{
}

void ObjectSelector::getBannedObjects()
{
}

void ObjectSelector::fillCustomObjects()
{
}

void ObjectSelector::getCustomObjects()
{
}

void ObjectSelector::showObjectSelector(ObjectConfig & obj)
{
	auto * dialog = new ObjectSelector(obj);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->exec();
}

void ObjectSelector::on_buttonBoxResult_accepted()
{
	getBannedObjectCategories();
	getBannedObjects();
	getCustomObjects();

    close();
}

void ObjectSelector::on_buttonBoxResult_rejected()
{
    close();
}