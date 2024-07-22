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
#include "../../lib/constants/NumericConstants.h"
#include "../../lib/constants/StringConstants.h"

TownEventDialog::TownEventDialog(CGTownInstance & t, QListWidgetItem * item, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::TownEventDialog),
	town(t),
	item(item)
{
	ui->setupUi(this);

	ui->buildingsTree->setModel(&buildingsModel);

	params = item->data(Qt::UserRole).toMap();
	ui->eventFirstOccurrence->setMinimum(1);
	ui->eventFirstOccurrence->setMaximum(999);
	ui->eventRepeatAfter->setMaximum(999);
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

	show();
}

TownEventDialog::~TownEventDialog()
{
	delete ui;
}

void TownEventDialog::initPlayers()
{
	for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		bool isAffected = (1 << i) & params.value("players").toInt();
		auto * item = new QListWidgetItem(QString::fromStdString(GameConstants::PLAYER_COLOR_NAMES[i]));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setCheckState(isAffected ? Qt::Checked : Qt::Unchecked);
		ui->playersAffected->addItem(item);
	}
}

void TownEventDialog::initResources()
{
	ui->resourcesTable->setRowCount(GameConstants::RESOURCE_QUANTITY);
	auto resourcesMap = params.value("resources").toMap();
	for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
	{
		auto name = QString::fromStdString(GameConstants::RESOURCE_NAMES[i]);
		int val = resourcesMap.value(name).toInt();
		ui->resourcesTable->setItem(i, 0, new QTableWidgetItem(name));

		QSpinBox * edit = new QSpinBox(ui->resourcesTable);
		edit->setMaximum(i == GameResID::GOLD ? 999999 : 999);
		edit->setMinimum(i == GameResID::GOLD ? -999999 : -999);
		edit->setSingleStep(i == GameResID::GOLD ? 100 : 1);
		edit->setValue(val);

		ui->resourcesTable->setCellWidget(i, 1, edit);
	}
}

void TownEventDialog::initBuildings()
{
	auto * ctown = town.town;
	if (!ctown)
		ctown = VLC->townh->randomTown;
	if (!ctown)
		throw std::runtime_error("No Town defined for type selected");
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
	const CBuilding * building = ctown.buildings.at(buildingId);
	if (!building)
	{
		remaining.erase(bId);
		return nullptr;
	}

	QString name = tr(building->getNameTranslated().c_str());

	if (name.isEmpty())
		name = QString::fromStdString(defaultBuildingIdConversion(buildingId));

	QList<QStandardItem *> checks;

	checks << new QStandardItem(name);
	checks.back()->setData(bId, Qt::UserRole);

	checks << new QStandardItem;
	checks.back()->setCheckable(true);
	checks.back()->setCheckState(params["buildings"].toList().contains(bId) ? Qt::Checked : Qt::Unchecked);
	checks.back()->setData(bId, Qt::UserRole);

	if (building->getBase() == buildingId)
	{
		buildingsModel.appendRow(checks);
	}
	else
	{
		QStandardItem * parent = nullptr;
		std::vector<QModelIndex> stack;
		stack.push_back(QModelIndex());
		while (!parent && !stack.empty())
		{
			auto pindex = stack.back();
			stack.pop_back();
			for (int i = 0; i < buildingsModel.rowCount(pindex); ++i)
			{
				QModelIndex index = buildingsModel.index(i, 0, pindex);
				if (building->upgrade.getNum() == buildingsModel.itemFromIndex(index)->data(Qt::UserRole).toInt())
				{
					parent = buildingsModel.itemFromIndex(index);
					break;
				}
				if (buildingsModel.hasChildren(index))
					stack.push_back(index);
			}
		}

		if (!parent)
			parent = addBuilding(ctown, building->upgrade.getNum(), remaining);

		if (!parent)
		{
			remaining.erase(bId);
			return nullptr;
		}

		parent->appendRow(checks);
	}

	remaining.erase(bId);
	return checks.front();
}

void TownEventDialog::initCreatures()
{
	auto creatures = params.value("creatures").toList();
	auto * ctown = town.town;
	for (int i = 0; i < 7; ++i)
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
				auto cre = VLC->creatures()->getById(creature);
				auto creatureName = QString::fromStdString(cre->getNameSingularTranslated());
				creatureNames.append(creatureNames.isEmpty() ? creatureName : " / " + creatureName);
			}
		}
		auto * item = new QTableWidgetItem();
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		item->setText(creatureNames);
		ui->creaturesTable->setItem(i, 0, item);

		auto creatureNumber = creatures.size() > i ? creatures.at(i).toInt() : 0;
		QSpinBox* edit = new QSpinBox(ui->creaturesTable);
		edit->setValue(creatureNumber);
		edit->setMaximum(999999);
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

	item->setData(Qt::UserRole, descriptor);
	auto itemText = tr("Day %1 - %2").arg(ui->eventFirstOccurrence->value(), 3).arg(ui->eventNameText->text());
	item->setText(itemText);
}

QVariant TownEventDialog::playersToVariant()
{
	int players = 0;
	for (int i = 0; i < ui->playersAffected->count(); ++i)
	{
		auto * item = ui->playersAffected->item(i);
		if (item->checkState() == Qt::Checked)
			players |= 1 << i;
	}
	return QVariant::fromValue(players);
}

QVariantMap TownEventDialog::resourcesToVariant()
{
	auto res = item->data(Qt::UserRole).toMap().value("resources").toMap();
	for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
	{
		auto * itemType = ui->resourcesTable->item(i, 0);
		auto * itemQty = static_cast<QSpinBox *> (ui->resourcesTable->cellWidget(i, 1));

		res[itemType->text()] = QVariant::fromValue(itemQty->value());
	}
	return res;
}

QVariantList TownEventDialog::buildingsToVariant()
{
	QVariantList buildingsList;
	std::vector<QModelIndex> stack;
	stack.push_back(QModelIndex());
	while (!stack.empty())
	{
		auto pindex = stack.back();
		stack.pop_back();
		for (int i = 0; i < buildingsModel.rowCount(pindex); ++i)
		{
			QModelIndex index = buildingsModel.index(i, 1, pindex);
			if (auto * item = buildingsModel.itemFromIndex(index))
				if (item->checkState() == Qt::Checked)
					buildingsList.push_back(item->data(Qt::UserRole));
			index = buildingsModel.index(i, 0, pindex);
			if (buildingsModel.hasChildren(index))
				stack.push_back(index);
		}
	}
	return buildingsList;
}

QVariantList TownEventDialog::creaturesToVariant()
{
	QVariantList creaturesList;
	for (int i = 0; i < 7; ++i)
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

void TownEventDialog::setRowColumnCheckState(QStandardItem * item, int column, Qt::CheckState checkState) {
	auto sibling = item->model()->sibling(item->row(), column, item->index());
	buildingsModel.itemFromIndex(sibling)->setCheckState(checkState);
}

void TownEventDialog::onItemChanged(QStandardItem * item)
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
		while (!stack.empty()) {
			nextRow = stack.back();
			stack.pop_back();
			setRowColumnCheckState(nextRow, item->column(), Qt::Unchecked);
			if (nextRow->hasChildren()) {
				for (int i = 0; i < nextRow->rowCount(); ++i) {
					stack.push_back(nextRow->child(i, 0));
				}
			}

		}
	}
	connect(&buildingsModel, &QStandardItemModel::itemChanged, this, &TownEventDialog::onItemChanged);
}
