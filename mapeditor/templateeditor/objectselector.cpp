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

#include "../mainwindow.h"
#include "../mapcontroller.h"

#include "../../lib/mapping/CMap.h"
#include "../../lib/rmg/ObjectConfig.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"

ObjectSelector::ObjectSelector(ObjectConfig & obj) :
	ui(new Ui::ObjectSelector),
	obj(obj),
	advObjects(getAdventureMapItems())
{
	ui->setupUi(this);

	setWindowTitle(tr("Object Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	fillBannedObjectCategories();
	fillBannedObjects();
	fillCustomObjects();

	show();
}

QMainWindow* getMainWindow()
{
	foreach (QWidget *w, qApp->topLevelWidgets())
		if (QMainWindow* mainWin = qobject_cast<QMainWindow*>(w))
			return mainWin;
	return nullptr;
}

std::map<CompoundMapObjectID, QString> ObjectSelector::getAdventureMapItems()
{
	std::map<CompoundMapObjectID, QString> objects;
	auto& controller = qobject_cast<MainWindow *>(getMainWindow())->controller;

	auto knownObjects = LIBRARY->objtypeh->knownObjects();
	for(auto & id : knownObjects)
	{
		auto knownSubObjects = LIBRARY->objtypeh->knownSubObjects(id);
		for(auto & subId : knownSubObjects)
		{
			auto objId = CompoundMapObjectID(id, subId);
			auto factory = LIBRARY->objtypeh->getHandlerFor(id, subId);
			auto templates = factory->getTemplates();
			
			QString name{};
			try
			{
				auto templ = templates.at(0);
				auto temporaryObj(factory->create(controller.getCallback(), templ));
				QString translated = QString::fromStdString(temporaryObj->getObjectName().c_str());
				name = translated;
			}
			catch(...) {}

			if(name.isEmpty())
			{
				auto subGroupName = QString::fromStdString(LIBRARY->objtypeh->getObjectName(id, subId));
				name = subGroupName;
			}

			if(!name.isEmpty())
				objects[objId] = name;
		}
	}

	return objects;
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

		auto deleteButton = new QPushButton(tr("Delete"));
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

	auto addButton = new QPushButton(tr("Add"));
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
		obj.bannedObjectCategories.push_back(val);
	}
}

void ObjectSelector::fillBannedObjects()
{
	ui->tableWidgetBannedObjects->setColumnCount(2);
	ui->tableWidgetBannedObjects->setRowCount(obj.bannedObjects.size() + 1);
	ui->tableWidgetBannedObjects->setHorizontalHeaderLabels({tr("Object"), tr("Action")});

	auto addRow = [this](CompoundMapObjectID obj, int row){
		QComboBox *combo = new QComboBox();
		for(auto & item : advObjects)
    		combo->addItem(item.second, QVariant::fromValue(item.first));

		int index = combo->findData(QVariant::fromValue(obj));
		if (index != -1)
			combo->setCurrentIndex(index);
		
		combo->setEditable(true);
		QCompleter* completer = new QCompleter(combo);
		completer->setModel(combo->model());
		completer->setCompletionMode(QCompleter::PopupCompletion);
		completer->setFilterMode(Qt::MatchContains);
		combo->setCompleter(completer);

		ui->tableWidgetBannedObjects->setCellWidget(row, 0, combo);

		auto deleteButton = new QPushButton(tr("Delete"));
		ui->tableWidgetBannedObjects->setCellWidget(row, 1, deleteButton);
		connect(deleteButton, &QPushButton::clicked, this, [this, deleteButton]() {
			for (int r = 0; r < ui->tableWidgetBannedObjects->rowCount(); ++r) {
				if (ui->tableWidgetBannedObjects->cellWidget(r, 1) == deleteButton) {
					ui->tableWidgetBannedObjects->removeRow(r);
					break;
				}
			}
		});
	};

	for (int row = 0; row < obj.bannedObjects.size(); ++row)
		addRow(obj.bannedObjects[row], row);

	auto addButton = new QPushButton(tr("Add"));
	ui->tableWidgetBannedObjects->setCellWidget(ui->tableWidgetBannedObjects->rowCount() - 1, 1, addButton);
	connect(addButton, &QPushButton::clicked, this, [this, addRow]() {
		ui->tableWidgetBannedObjects->insertRow(ui->tableWidgetBannedObjects->rowCount() - 1);
		addRow((*advObjects.begin()).first, ui->tableWidgetBannedObjects->rowCount() - 2);
	});

	ui->tableWidgetBannedObjects->resizeColumnsToContents();
	ui->tableWidgetBannedObjects->setColumnWidth(0, 300);
}

void ObjectSelector::getBannedObjects()
{
	obj.bannedObjects.clear();
	for (int row = 0; row < ui->tableWidgetBannedObjects->rowCount() - 1; ++row)
	{
		auto val = static_cast<QComboBox *>(ui->tableWidgetBannedObjects->cellWidget(row, 0))->currentData().value<CompoundMapObjectID>();
		obj.bannedObjects.push_back(val);
	}
}

void ObjectSelector::fillCustomObjects()
{
	ui->tableWidgetObjects->setColumnCount(5);
	ui->tableWidgetObjects->setRowCount(obj.customObjects.size() + 1);
	ui->tableWidgetObjects->setHorizontalHeaderLabels({tr("Object"), tr("Value"), tr("Probability"), tr("Max per zone"), tr("Action")});

	auto addRow = [this](CompoundMapObjectID obj, ui32 value, ui16 probability, ui32 maxPerZone, int row){
		QComboBox *combo = new QComboBox();
		for(auto & item : advObjects)
    		combo->addItem(item.second, QVariant::fromValue(item.first));

		int index = combo->findData(QVariant::fromValue(obj));
		if (index != -1)
			combo->setCurrentIndex(index);
		
		combo->setEditable(true);
		QCompleter* completer = new QCompleter(combo);
		completer->setModel(combo->model());
		completer->setCompletionMode(QCompleter::PopupCompletion);
		completer->setFilterMode(Qt::MatchContains);
		combo->setCompleter(completer);

		ui->tableWidgetObjects->setCellWidget(row, 0, combo);

		QSpinBox *spinValue = new QSpinBox();
        spinValue->setRange(0, 10000);
        spinValue->setValue(value);
		ui->tableWidgetObjects->setCellWidget(row, 1, spinValue);

		QSpinBox *spinProbability = new QSpinBox();
        spinProbability->setRange(0, 1000);
        spinProbability->setValue(probability);
		ui->tableWidgetObjects->setCellWidget(row, 2, spinProbability);

		QSpinBox *spinMaxPerZone = new QSpinBox();
        spinMaxPerZone->setRange(0, 100);
        spinMaxPerZone->setValue(maxPerZone);
		ui->tableWidgetObjects->setCellWidget(row, 3, spinMaxPerZone);

		auto deleteButton = new QPushButton(tr("Delete"));
		ui->tableWidgetObjects->setCellWidget(row, 4, deleteButton);
		connect(deleteButton, &QPushButton::clicked, this, [this, deleteButton]() {
			for (int r = 0; r < ui->tableWidgetObjects->rowCount(); ++r) {
				if (ui->tableWidgetObjects->cellWidget(r, 4) == deleteButton) {
					ui->tableWidgetObjects->removeRow(r);
					break;
				}
			}
		});
	};

	for (int row = 0; row < obj.customObjects.size(); ++row)
		addRow(obj.customObjects[row].getCompoundID(), obj.customObjects[row].value, obj.customObjects[row].probability, obj.customObjects[row].maxPerZone, row);

	auto addButton = new QPushButton(tr("Add"));
	ui->tableWidgetObjects->setCellWidget(ui->tableWidgetObjects->rowCount() - 1, 4, addButton);
	connect(addButton, &QPushButton::clicked, this, [this, addRow]() {
		ui->tableWidgetObjects->insertRow(ui->tableWidgetObjects->rowCount() - 1);
		addRow((*advObjects.begin()).first, 0, 0, 1, ui->tableWidgetObjects->rowCount() - 2);
	});

	ui->tableWidgetObjects->resizeColumnsToContents();
	ui->tableWidgetObjects->setColumnWidth(0, 300);
}

void ObjectSelector::getCustomObjects()
{
	obj.customObjects.clear();
	for (int row = 0; row < ui->tableWidgetObjects->rowCount() - 1; ++row)
	{
		auto id = static_cast<QComboBox *>(ui->tableWidgetObjects->cellWidget(row, 0))->currentData().value<CompoundMapObjectID>();
		auto value = static_cast<QSpinBox *>(ui->tableWidgetObjects->cellWidget(row, 1))->value();
		auto probability = static_cast<QSpinBox *>(ui->tableWidgetObjects->cellWidget(row, 2))->value();
		auto maxPerZone = static_cast<QSpinBox *>(ui->tableWidgetObjects->cellWidget(row, 3))->value();
		auto info = ObjectInfo(id);
		info.value = value;
		info.probability = probability;
		info.maxPerZone = maxPerZone;
		obj.customObjects.push_back(info);
	}
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