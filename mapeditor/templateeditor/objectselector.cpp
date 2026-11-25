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
#include "../helper.h"

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
#include "../translator.h"

#include <QCheckBox>

ObjectSelector::ObjectSelector(ObjectConfig & obj) :
	ui(new Ui::ObjectSelector),
	obj(obj),
	advObjects(getAdventureMapItems())
{
	ui->setupUi(this);
	Helper::decorateDialog(this);
	setWindowTitle(tr("Object Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	fillBannedObjectCategories();
	fillBannedObjects();
	fillCustomObjects();
	fillRequiredObjects();

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
	auto& controller = qobject_cast<EditorMainWindow *>(getMainWindow())->controller;

	auto knownObjects = LIBRARY->objtypeh->knownObjects();
	for(auto & id : knownObjects)
	{
		auto knownSubObjects = LIBRARY->objtypeh->knownSubObjects(id);
		for(auto & subId : knownSubObjects)
		{
			auto objId = CompoundMapObjectID(id, subId);
			auto & factory = LIBRARY->objtypeh->getHandlerFor(id, subId);
			auto templates = factory.getTemplates();
			
			QString name{};
			try
			{
				auto templ = templates.at(0);
				auto temporaryObj(factory.create(controller.getCallback(), templ));
				QString translated = QString::fromStdString(temporaryObj->getObjectName().toString(&Translator::instance()));
				name = translated;
			}
			catch(...) {}

			if(name.isEmpty())
			{
				// no object could be created, so fall back to the name of the type itself
				auto subGroupTextID = factory.hasNameTextID() ? factory.getNameTextID() : LIBRARY->objtypeh->getObjectGroupNameTextID(id);
				name = QString::fromStdString(Translator::instance().translate(subGroupTextID));
			}

			if(!name.isEmpty())
				objects[objId] = name;
		}
	}

	return objects;
}

void ObjectSelector::setObjectComboCell(QTableWidget * table, int row, int column, const CompoundMapObjectID & selected) const
{
	auto combo = std::make_unique<QComboBox>();
	for(const auto & [objectId, name] : advObjects)
		combo->addItem(name, QVariant::fromValue(objectId));

	int index = combo->findData(QVariant::fromValue(selected));
	if (index != -1)
		combo->setCurrentIndex(index);

	combo->setEditable(true);
	// The combo box owns its completer
	auto completer = std::make_unique<QCompleter>(combo.get());
	completer->setModel(combo->model());
	completer->setCompletionMode(QCompleter::PopupCompletion);
	completer->setFilterMode(Qt::MatchContains);
	combo->setCompleter(completer.release());

	// The table takes ownership of its cell widgets
	table->setCellWidget(row, column, combo.release());
}

void ObjectSelector::setDeleteButtonCell(QTableWidget * table, int row, int column)
{
	auto deleteButton = std::make_unique<QPushButton>(tr("Delete"));
	connect(deleteButton.get(), &QPushButton::clicked, this, [table, column, button = deleteButton.get()]() {
		for (int r = 0; r < table->rowCount(); ++r) {
			if (table->cellWidget(r, column) == button) {
				table->removeRow(r);
				break;
			}
		}
	});
	table->setCellWidget(row, column, deleteButton.release());
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

		setDeleteButtonCell(ui->tableWidgetBannedObjectCategories, row, 1);
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
	const auto & bans = obj.getObjectBans();
	ui->tableWidgetBannedObjects->setColumnCount(3);
	ui->tableWidgetBannedObjects->setRowCount(static_cast<int>(bans.size()) + 1);
	ui->tableWidgetBannedObjects->setHorizontalHeaderLabels({tr("Object"), tr("Banned"), tr("Action")});

	auto addRow = [this](CompoundMapObjectID objId, bool banned, int row){
		setObjectComboCell(ui->tableWidgetBannedObjects, row, 0, objId);

		auto bannedBox = std::make_unique<QCheckBox>();
		bannedBox->setChecked(banned);
		ui->tableWidgetBannedObjects->setCellWidget(row, 1, bannedBox.release());

		setDeleteButtonCell(ui->tableWidgetBannedObjects, row, 2);
	};

	int row = 0;
	for (const auto & [id, banned] : bans)
		addRow(id, banned, row++);

	auto addButton = new QPushButton(tr("Add"));
	ui->tableWidgetBannedObjects->setCellWidget(ui->tableWidgetBannedObjects->rowCount() - 1, 2, addButton);
	connect(addButton, &QPushButton::clicked, this, [this, addRow]() {
		ui->tableWidgetBannedObjects->insertRow(ui->tableWidgetBannedObjects->rowCount() - 1);
		addRow((*advObjects.begin()).first, true, ui->tableWidgetBannedObjects->rowCount() - 2);
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
		auto banned = static_cast<QCheckBox *>(ui->tableWidgetBannedObjects->cellWidget(row, 1))->isChecked();
		obj.setObjectBanned(val, banned);
	}
}

void ObjectSelector::fillRequiredObjects()
{
	auto reqObjs = obj.getRequiredObjects();
	
	ui->tableWidgetRequiredObjects->setColumnCount(4);
	ui->tableWidgetRequiredObjects->setRowCount(reqObjs.size() + 1);
	ui->tableWidgetRequiredObjects->setHorizontalHeaderLabels({tr("Object"), tr("Count"), tr("Guard"), tr("Action")});

	auto addRow = [this](CompoundMapObjectID objId, ui16 count, std::optional<ui32> guard, int row){
		setObjectComboCell(ui->tableWidgetRequiredObjects, row, 0, objId);

		// Column 1: Count (QSpinBox, range 1-100)
		QSpinBox *spinCount = new QSpinBox();
		spinCount->setRange(1, 100);
		spinCount->setValue(count);
		ui->tableWidgetRequiredObjects->setCellWidget(row, 1, spinCount);

		// Column 2: Guard level (QSpinBox, range -1-999999, -1 = no guard)
		QSpinBox *spinGuard = new QSpinBox();
		spinGuard->setRange(-1, 999999);
		spinGuard->setValue(guard.value_or(-1));
		spinGuard->setSpecialValueText(tr("None")); // Shows "None" when value is -1
		ui->tableWidgetRequiredObjects->setCellWidget(row, 2, spinGuard);

		setDeleteButtonCell(ui->tableWidgetRequiredObjects, row, 3);
	};

	int row = 0;
	for (const auto & [objId, info] : reqObjs)
		addRow(objId, info.first, info.second, row++);

	// Add "Add" button in last row
	auto addButton = new QPushButton(tr("Add"));
	ui->tableWidgetRequiredObjects->setCellWidget(ui->tableWidgetRequiredObjects->rowCount() - 1, 3, addButton);
	connect(addButton, &QPushButton::clicked, this, [this, addRow]() {
		ui->tableWidgetRequiredObjects->insertRow(ui->tableWidgetRequiredObjects->rowCount() - 1);
		addRow((*advObjects.begin()).first, 1, std::nullopt, ui->tableWidgetRequiredObjects->rowCount() - 2);
	});

	ui->tableWidgetRequiredObjects->resizeColumnsToContents();
	ui->tableWidgetRequiredObjects->setColumnWidth(0, 400);
}

void ObjectSelector::getRequiredObjects()
{
	obj.clearRequiredObjects();
	for (int row = 0; row < ui->tableWidgetRequiredObjects->rowCount() - 1; ++row)
	{
		auto id = static_cast<QComboBox *>(ui->tableWidgetRequiredObjects->cellWidget(row, 0))->currentData().value<CompoundMapObjectID>();
		auto count = static_cast<QSpinBox *>(ui->tableWidgetRequiredObjects->cellWidget(row, 1))->value();
		auto guardValue = static_cast<QSpinBox *>(ui->tableWidgetRequiredObjects->cellWidget(row, 2))->value();

		std::optional<ui32> guard = std::nullopt;
		if (guardValue >= 0)
			guard = guardValue;

		obj.addRequiredObject(id, count, guard);
	}
}

void ObjectSelector::fillCustomObjects()
{
	ui->tableWidgetObjects->setColumnCount(5);
	ui->tableWidgetObjects->setRowCount(obj.customObjects.size() + 1);
	ui->tableWidgetObjects->setHorizontalHeaderLabels({tr("Object"), tr("Value"), tr("Probability"), tr("Max per zone"), tr("Action")});

	auto addRow = [this](CompoundMapObjectID objId, ui32 value, ui16 probability, ui32 maxPerZone, int row){
		setObjectComboCell(ui->tableWidgetObjects, row, 0, objId);

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

		setDeleteButtonCell(ui->tableWidgetObjects, row, 4);
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
	getRequiredObjects();

	close();
}

void ObjectSelector::on_buttonBoxResult_rejected()
{
	close();
}