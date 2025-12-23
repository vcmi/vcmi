/*
 * towneventdialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "../StdInc.h"
#include "townbuildingswidget.h"
#include "towneventdialog.h"
#include "ui_towneventdialog.h"
#include "mapeditorroles.h"
#include "../mapsettings/eventsettings.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/constants/NumericConstants.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/entities/ResourceTypeHandler.h"

static const int FIRST_DAY_FOR_EVENT = 1;
static const int LAST_DAY_FOR_EVENT = 999;
static const int MAXIMUM_EVENT_REPEAT_AFTER = 999;

static const int MAXIMUM_GOLD_CHANGE = 999999;
static const int MAXIMUM_RESOURCE_CHANGE = 999;
static const int GOLD_STEP = 100;
static const int RESOURCE_STEP = 1;

static const int MAXIMUM_CREATURES_CHANGE = 999999;

TownEventDialog::TownEventDialog(CGTownInstance & t, QListWidgetItem * item, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::TownEventDialog),
	town(t),
	townEventListItem(item)
{
	ui->setupUi(this);

	ui->buildingsTree->setModel(&buildingsModel);

	params = townEventListItem->data(MapEditorRoles::TownEventRole).toMap();
	ui->eventFirstOccurrence->setMinimum(FIRST_DAY_FOR_EVENT);
	ui->eventFirstOccurrence->setMaximum(LAST_DAY_FOR_EVENT);
	ui->eventRepeatAfter->setMaximum(MAXIMUM_EVENT_REPEAT_AFTER);
	ui->eventNameText->setText(params.value("name").toString());
	ui->eventMessageText->setPlainText(params.value("message").toString());
	ui->eventAffectsCpu->setChecked(params.value("computerAffected").toBool());
	ui->eventAffectsHuman->setChecked(params.value("humanAffected").toBool());
	ui->eventFirstOccurrence->setValue(params.value("firstOccurrence").toInt()+1);
	ui->eventRepeatAfter->setValue(params.value("nextOccurrence").toInt());

	initPlayers();
	initResources();
	initBuildings();
	initCreatures();
}

TownEventDialog::~TownEventDialog()
{
	delete ui;
}

void TownEventDialog::initPlayers()
{
	auto playerList = params.value("players").toList();
	for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		MetaString str;
		str.appendName(PlayerColor(i));
		bool isAffected = playerList.contains(toQString(PlayerColor(i)));
		auto * item = new QListWidgetItem(QString::fromStdString(str.toString()));
		item->setData(MapEditorRoles::PlayerIDRole, QVariant::fromValue(i));
		item->setCheckState(isAffected ? Qt::Checked : Qt::Unchecked);
		ui->playersAffected->addItem(item);
	}
}

void TownEventDialog::initResources()
{
	ui->resourcesTable->setRowCount(LIBRARY->resourceTypeHandler->getAllObjects().size());
	auto resourcesMap = params.value("resources").toMap();
	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		MetaString str;
		str.appendName(GameResID(i));
		auto name = QString::fromStdString(str.toString());
		auto * item = new QTableWidgetItem();
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		item->setText(name);
		ui->resourcesTable->setItem(i, 0, item);

		int val = resourcesMap.value(QString::fromStdString(i.toResource()->getJsonKey())).toInt();
		auto * edit = new QSpinBox(ui->resourcesTable);
		edit->setMaximum(i == GameResID::GOLD ? MAXIMUM_GOLD_CHANGE : MAXIMUM_RESOURCE_CHANGE);
		edit->setMinimum(i == GameResID::GOLD ? -MAXIMUM_GOLD_CHANGE : -MAXIMUM_RESOURCE_CHANGE);
		edit->setSingleStep(i == GameResID::GOLD ? GOLD_STEP : RESOURCE_STEP);
		edit->setValue(val);

		ui->resourcesTable->setCellWidget(i, 1, edit);
	}
}

void TownEventDialog::initBuildings()
{
	auto * ctown = town.getTown();
	auto allBuildings = ctown->getAllBuildings();
	while (!allBuildings.empty())
	{
		addBuilding(*ctown, *allBuildings.begin(), allBuildings);
	}
	ui->buildingsTree->resizeColumnToContents(0);

	connect(&buildingsModel, &QStandardItemModel::itemChanged, this, &TownEventDialog::onItemChanged);
}

QStandardItem * TownEventDialog::addBuilding(const CTown& ctown, BuildingID buildingId, std::set<si32>& remaining)
{
	auto bId = buildingId.num;
	const auto & building = ctown.buildings.at(buildingId);

	QString name = QString::fromStdString(building->getNameTranslated());

	if (name.isEmpty())
		name = QString::fromStdString(defaultBuildingIdConversion(buildingId));

	QList<QStandardItem *> checks;

	checks << new QStandardItem(name);
	checks.back()->setData(bId, MapEditorRoles::BuildingIDRole);

	checks << new QStandardItem;
	checks.back()->setCheckable(true);
	checks.back()->setCheckState(params["buildings"].toList().contains(bId) ? Qt::Checked : Qt::Unchecked);
	checks.back()->setData(bId, MapEditorRoles::BuildingIDRole);

	if (building->getBase() == buildingId)
	{
		buildingsModel.appendRow(checks);
	}
	else
	{
		QStandardItem * parent = getBuildingParentFromTreeModel(building.get(), buildingsModel);

		if (!parent)
			parent = addBuilding(ctown, building->upgrade.getNum(), remaining);

		parent->appendRow(checks);
	}

	remaining.erase(bId);
	return checks.front();
}

void TownEventDialog::initCreatures()
{
	auto creatures = params.value("creatures").toList();
	auto * ctown = town.getTown();
	if (!ctown)
		ui->creaturesTable->setRowCount(GameConstants::CREATURES_PER_TOWN);
	else 
		ui->creaturesTable->setRowCount(ctown->creatures.size());

	for (int i = 0; i < ui->creaturesTable->rowCount(); ++i)
	{
		QString creatureNames;
		if (!ctown)
		{
			creatureNames.append(tr("Creature level %1 / Creature level %1 Upgrade").arg(i + 1));
		}
		else
		{
			auto creaturesOnLevel = ctown->creatures.at(i);
			for (auto& creature : creaturesOnLevel)
			{
				auto cre = LIBRARY->creatures()->getById(creature);
				auto creatureName = QString::fromStdString(cre->getNameSingularTranslated());
				creatureNames.append(creatureNames.isEmpty() ? creatureName : " / " + creatureName);
			}
		}
		auto * item = new QTableWidgetItem();
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		item->setText(creatureNames);
		ui->creaturesTable->setItem(i, 0, item);

		auto creatureNumber = creatures.size() > i ? creatures.at(i).toInt() : 0;
		auto * edit = new QSpinBox(ui->creaturesTable);
		edit->setValue(creatureNumber);
		edit->setMaximum(MAXIMUM_CREATURES_CHANGE);
		ui->creaturesTable->setCellWidget(i, 1, edit);

	}
	ui->creaturesTable->resizeColumnToContents(0);
}

void TownEventDialog::on_TownEventDialog_finished(int result)
{
	QVariantMap descriptor;
	descriptor["name"] = ui->eventNameText->text();
	descriptor["message"] = ui->eventMessageText->toPlainText();
	descriptor["humanAffected"] = QVariant::fromValue(ui->eventAffectsHuman->isChecked());
	descriptor["computerAffected"] = QVariant::fromValue(ui->eventAffectsCpu->isChecked());
	descriptor["firstOccurrence"] = QVariant::fromValue(ui->eventFirstOccurrence->value()-1);
	descriptor["nextOccurrence"] = QVariant::fromValue(ui->eventRepeatAfter->value());
	descriptor["players"] = playersToVariant();
	descriptor["resources"] = resourcesToVariant();
	descriptor["buildings"] = buildingsToVariant();
	descriptor["creatures"] = creaturesToVariant();

	townEventListItem->setData(MapEditorRoles::TownEventRole, descriptor);
	auto itemText = tr("Day %1 - %2").arg(ui->eventFirstOccurrence->value(), 3).arg(ui->eventNameText->text());
	townEventListItem->setText(itemText);
}

QVariant TownEventDialog::playersToVariant()
{
	QVariantList players;
	for (int i = 0; i < ui->playersAffected->count(); ++i)
	{
		auto * item = ui->playersAffected->item(i);
		if (item->checkState() == Qt::Checked)
			players.push_back(toQString(PlayerColor(i)));
	}
	return QVariant::fromValue(players);
}

QVariantMap TownEventDialog::resourcesToVariant()
{
	auto res = params.value("resources").toMap();
	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		auto itemType = QString::fromStdString(i.toResource()->getJsonKey());
		auto * itemQty = static_cast<QSpinBox *> (ui->resourcesTable->cellWidget(i, 1));

		res[itemType] = QVariant::fromValue(itemQty->value());
	}
	return res;
}

QVariantList TownEventDialog::buildingsToVariant()
{
	return getBuildingVariantsFromModel(buildingsModel, 1, Qt::Checked);
}

QVariantList TownEventDialog::creaturesToVariant()
{
	QVariantList creaturesList;
	for (int i = 0; i < ui->creaturesTable->rowCount(); ++i)
	{
		auto * item = static_cast<QSpinBox *>(ui->creaturesTable->cellWidget(i, 1));
		creaturesList.push_back(item->value());
	}
	return creaturesList;
}

void TownEventDialog::on_okButton_clicked()
{
	close();
}

void TownEventDialog::setRowColumnCheckState(const QStandardItem * item, int column, Qt::CheckState checkState) {
	auto sibling = item->model()->sibling(item->row(), column, item->index());
	buildingsModel.itemFromIndex(sibling)->setCheckState(checkState);
}

void TownEventDialog::onItemChanged(const QStandardItem * item)
{
	disconnect(&buildingsModel, &QStandardItemModel::itemChanged, this, &TownEventDialog::onItemChanged);
	auto rowFirstColumnIndex = item->model()->sibling(item->row(), 0, item->index());
	QStandardItem * nextRow = buildingsModel.itemFromIndex(rowFirstColumnIndex);
	if (item->checkState() == Qt::Checked) {
		while (nextRow) {
			setRowColumnCheckState(nextRow,item->column(), Qt::Checked);
			nextRow = nextRow->parent();

		}
	}
	else if (item->checkState() == Qt::Unchecked) {
		std::vector<QStandardItem *> stack;
		stack.push_back(nextRow);
		do
		{
			nextRow = stack.back();
			stack.pop_back();
			setRowColumnCheckState(nextRow, item->column(), Qt::Unchecked);
			if (nextRow->hasChildren()) {
				for (int i = 0; i < nextRow->rowCount(); ++i) {
					stack.push_back(nextRow->child(i, 0));
				}
			}

		} while(!stack.empty());
	}
	connect(&buildingsModel, &QStandardItemModel::itemChanged, this, &TownEventDialog::onItemChanged);
}
