/*
 * townbuildingswidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "townbuildingswidget.h"
#include "ui_townbuildingswidget.h"
#include "mapeditorroles.h"
#include "../lib/entities/building/CBuilding.h"
#include "../lib/entities/faction/CTownHandler.h"
#include "../lib/texts/CGeneralTextHandler.h"

std::string defaultBuildingIdConversion(BuildingID bId)
{
	switch(bId)
	{
		case BuildingID::DEFAULT: return "DEFAULT";
		case BuildingID::MAGES_GUILD_1: return "MAGES_GUILD_1";
		case BuildingID::MAGES_GUILD_2: return "MAGES_GUILD_2";
		case BuildingID::MAGES_GUILD_3: return "MAGES_GUILD_3";
		case BuildingID::MAGES_GUILD_4: return "MAGES_GUILD_4";
		case BuildingID::MAGES_GUILD_5: return "MAGES_GUILD_5";
		case BuildingID::TAVERN: return "TAVERN";
		case BuildingID::SHIPYARD: return "SHIPYARD";
		case BuildingID::FORT: return "FORT";
		case BuildingID::CITADEL: return "CITADEL";
		case BuildingID::CASTLE: return "CASTLE";
		case BuildingID::VILLAGE_HALL: return "VILLAGE_HALL";
		case BuildingID::TOWN_HALL: return "TOWN_HALL";
		case BuildingID::CITY_HALL: return "CITY_HALL";
		case BuildingID::CAPITOL: return "CAPITOL";
		case BuildingID::MARKETPLACE: return "MARKETPLACE";
		case BuildingID::RESOURCE_SILO: return "RESOURCE_SILO";
		case BuildingID::BLACKSMITH: return "BLACKSMITH";
		case BuildingID::SPECIAL_1: return "SPECIAL_1";
		case BuildingID::SPECIAL_2: return "SPECIAL_2";
		case BuildingID::SPECIAL_3: return "SPECIAL_3";
		case BuildingID::SPECIAL_4: return "SPECIAL_4";
		case BuildingID::HORDE_1: return "HORDE_1";
		case BuildingID::HORDE_1_UPGR: return "HORDE_1_UPGR";
		case BuildingID::HORDE_2: return "HORDE_2";
		case BuildingID::HORDE_2_UPGR: return "HORDE_2_UPGR";
		case BuildingID::SHIP: return "SHIP";
		case BuildingID::GRAIL: return "GRAIL";
		case BuildingID::EXTRA_TOWN_HALL: return "EXTRA_TOWN_HALL";
		case BuildingID::EXTRA_CITY_HALL: return "EXTRA_CITY_HALL";
		case BuildingID::EXTRA_CAPITOL: return "EXTRA_CAPITOL";
		case BuildingID::DWELL_LVL_1: return "DWELL_LVL_1";
		case BuildingID::DWELL_LVL_2: return "DWELL_LVL_2";
		case BuildingID::DWELL_LVL_3: return "DWELL_LVL_3";
		case BuildingID::DWELL_LVL_4: return "DWELL_LVL_4";
		case BuildingID::DWELL_LVL_5: return "DWELL_LVL_5";
		case BuildingID::DWELL_LVL_6: return "DWELL_LVL_6";
		case BuildingID::DWELL_LVL_7: return "DWELL_LVL_7";
		case BuildingID::DWELL_LVL_8: return "DWELL_LVL_8";
		case BuildingID::DWELL_LVL_1_UP: return "DWELL_LVL_1_UP";
		case BuildingID::DWELL_LVL_2_UP: return "DWELL_LVL_2_UP";
		case BuildingID::DWELL_LVL_3_UP: return "DWELL_LVL_3_UP";
		case BuildingID::DWELL_LVL_4_UP: return "DWELL_LVL_4_UP";
		case BuildingID::DWELL_LVL_5_UP: return "DWELL_LVL_5_UP";
		case BuildingID::DWELL_LVL_6_UP: return "DWELL_LVL_6_UP";
		case BuildingID::DWELL_LVL_7_UP: return "DWELL_LVL_7_UP";
		case BuildingID::DWELL_LVL_8_UP: return "DWELL_LVL_8_UP";
		default:
			return "UNKNOWN";
	}
}

QStandardItem * getBuildingParentFromTreeModel(const CBuilding * building, const QStandardItemModel & model)
{
	QStandardItem * parent = nullptr;
	std::vector<QModelIndex> stack(1);
	do
	{
		auto pindex = stack.back();
		stack.pop_back();
		auto rowCount = model.rowCount(pindex);
		for (int i = 0; i < rowCount; ++i)
		{
			QModelIndex index = model.index(i, 0, pindex);
			if (building->upgrade.getNum() == model.itemFromIndex(index)->data(MapEditorRoles::BuildingIDRole).toInt())
			{
				parent = model.itemFromIndex(index);
				break;
			}
			if (model.hasChildren(index))
				stack.push_back(index);
		}
	} while(!parent && !stack.empty());
	return parent;
}

QVariantList getBuildingVariantsFromModel(const QStandardItemModel & model, int modelColumn, Qt::CheckState checkState)
{
	QVariantList result;
	std::vector<QModelIndex> stack(1);
	do
	{
		auto pindex = stack.back();
		stack.pop_back();
		auto rowCount = model.rowCount(pindex);
		for (int i = 0; i < rowCount; ++i)
		{
			QModelIndex index = model.index(i, modelColumn, pindex);
			auto * item = model.itemFromIndex(index);
			if(item && item->checkState() == checkState)
				result.push_back(item->data(MapEditorRoles::BuildingIDRole));
			index = model.index(i, 0, pindex);
			if (model.hasChildren(index))
				stack.push_back(index);
		}
	} while(!stack.empty());

	return result;
}



TownBuildingsWidget::TownBuildingsWidget(CGTownInstance & t, QWidget *parent) :
	town(t),
	QDialog(parent),
	ui(new Ui::TownBuildingsWidget)
{
	ui->setupUi(this);
	ui->treeView->setModel(&model);
	//ui->treeView->setColumnCount(3);
	model.setHorizontalHeaderLabels(QStringList() << tr("Type") << tr("Enabled") << tr("Built"));
	connect(&model, &QStandardItemModel::itemChanged, this, &TownBuildingsWidget::onItemChanged);
	//setAttribute(Qt::WA_DeleteOnClose);
}

TownBuildingsWidget::~TownBuildingsWidget()
{
	delete ui;
}

QStandardItem * TownBuildingsWidget::addBuilding(const CTown & ctown, int bId, std::set<si32> & remaining)
{
	BuildingID buildingId(bId);
	const auto & building = ctown.buildings.at(buildingId);
	if(!building)
	{
		remaining.erase(bId);
		return nullptr;
	}
	
	QString name = QString::fromStdString(building->getNameTranslated());
	
	if(name.isEmpty())
		name = QString::fromStdString(defaultBuildingIdConversion(buildingId));
	
	QList<QStandardItem *> checks;
	
	checks << new QStandardItem(name);
	checks.back()->setData(bId, MapEditorRoles::BuildingIDRole);
	
	checks << new QStandardItem;
	checks.back()->setCheckable(true);
	checks.back()->setCheckState(town.forbiddenBuildings.count(buildingId) ? Qt::Unchecked : Qt::Checked);
	checks.back()->setData(bId, MapEditorRoles::BuildingIDRole);
	
	checks << new QStandardItem;
	checks.back()->setCheckable(true);
	checks.back()->setCheckState(town.hasBuilt(buildingId) ? Qt::Checked : Qt::Unchecked);
	checks.back()->setData(bId, MapEditorRoles::BuildingIDRole);
	
	if(building->getBase() == buildingId)
	{
		model.appendRow(checks);
	}
	else
	{
		QStandardItem * parent = getBuildingParentFromTreeModel(building.get(), model);
		
		if(!parent)
			parent = addBuilding(ctown, building->upgrade.getNum(), remaining);
		
		if(!parent)
		{
			remaining.erase(bId);
			return nullptr;
		}
		
		parent->appendRow(checks);
	}
	
	remaining.erase(bId);
	return checks.front();
}

void TownBuildingsWidget::addBuildings(const CTown & ctown)
{
	auto buildings = ctown.getAllBuildings();
	while(!buildings.empty())
	{
		addBuilding(ctown, *buildings.begin(), buildings);
	}
	ui->treeView->resizeColumnToContents(0);
	ui->treeView->resizeColumnToContents(1);
	ui->treeView->resizeColumnToContents(2);
}

std::set<BuildingID> TownBuildingsWidget::getBuildingsFromModel(int modelColumn, Qt::CheckState checkState)
{
	auto buildingVariants = getBuildingVariantsFromModel(model, modelColumn, checkState);
	std::set<BuildingID> result;
	for (const auto & buildingId : buildingVariants)
	{
		result.insert(buildingId.toInt());
	}
	return result;
}

std::set<BuildingID> TownBuildingsWidget::getForbiddenBuildings()
{
	return getBuildingsFromModel(Column::ENABLED, Qt::Unchecked);
}

std::set<BuildingID> TownBuildingsWidget::getBuiltBuildings()
{
	return getBuildingsFromModel(Column::BUILT, Qt::Checked);
}

void TownBuildingsWidget::on_treeView_expanded(const QModelIndex &index)
{
	ui->treeView->resizeColumnToContents(0);
}

void TownBuildingsWidget::on_treeView_collapsed(const QModelIndex &index)
{
	ui->treeView->resizeColumnToContents(0);
}

void TownBuildingsWidget::on_buildAll_clicked()
{
	setAllRowsColumnCheckState(Column::BUILT, Qt::Checked);
}

void TownBuildingsWidget::on_demolishAll_clicked()
{
	setAllRowsColumnCheckState(Column::BUILT, Qt::Unchecked);
}

void TownBuildingsWidget::on_enableAll_clicked()
{
	setAllRowsColumnCheckState(Column::ENABLED, Qt::Checked);
}

void TownBuildingsWidget::on_disableAll_clicked()
{
	setAllRowsColumnCheckState(Column::ENABLED, Qt::Unchecked);
}


void TownBuildingsWidget::setRowColumnCheckState(const QStandardItem * item, Column column, Qt::CheckState checkState) {
	auto sibling = item->model()->sibling(item->row(), column, item->index());
	model.itemFromIndex(sibling)->setCheckState(checkState);
}

void TownBuildingsWidget::setAllRowsColumnCheckState(Column column, Qt::CheckState checkState)
{
	std::vector<QModelIndex> stack(1);
	do
	{
		auto parentIndex = stack.back();
		stack.pop_back();
		auto rowCount = model.rowCount(parentIndex);
		for (int i = 0; i < rowCount; ++i)
		{
			QModelIndex index = model.index(i, column, parentIndex);
			if (auto* item = model.itemFromIndex(index))
				item->setCheckState(checkState);
			index = model.index(i, 0, parentIndex);
			if (model.hasChildren(index))
				stack.push_back(index);
		}
	} while(!stack.empty());
}

void TownBuildingsWidget::onItemChanged(const QStandardItem * item) {
	disconnect(&model, &QStandardItemModel::itemChanged, this, &TownBuildingsWidget::onItemChanged);
	auto rowFirstColumnIndex = item->model()->sibling(item->row(), Column::TYPE, item->index());
	QStandardItem * nextRow = model.itemFromIndex(rowFirstColumnIndex);
	if (item->checkState() == Qt::Checked) {
		while (nextRow) {
			setRowColumnCheckState(nextRow, Column(item->column()), Qt::Checked);
			if (item->column() == Column::BUILT) {
				setRowColumnCheckState(nextRow, Column::ENABLED, Qt::Checked);
			}
			nextRow = nextRow->parent();

		}
	}
	else if (item->checkState() == Qt::Unchecked) {
		std::vector<QStandardItem*> stack;
		stack.push_back(nextRow);
		do
		{
			nextRow = stack.back();
			stack.pop_back();
			setRowColumnCheckState(nextRow, Column(item->column()), Qt::Unchecked);
			if (item->column() == Column::ENABLED) {
				setRowColumnCheckState(nextRow, Column::BUILT, Qt::Unchecked);
			}
			if (nextRow->hasChildren()) {
				for (int i = 0; i < nextRow->rowCount(); ++i) {
					stack.push_back(nextRow->child(i, Column::TYPE));
				}
			}
			
		} while(!stack.empty());
	}
	connect(&model, &QStandardItemModel::itemChanged, this, &TownBuildingsWidget::onItemChanged);
}

TownBuildingsDelegate::TownBuildingsDelegate(CGTownInstance & t)
	: BaseInspectorItemDelegate()
	, town(t)
{
}

QWidget * TownBuildingsDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new TownBuildingsWidget(town, parent);
}

void TownBuildingsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<TownBuildingsWidget *>(editor))
	{
		auto * ctown = town.getTown();
		ed->addBuildings(*ctown);
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void TownBuildingsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<TownBuildingsWidget *>(editor))
	{
		town.forbiddenBuildings = ed->getForbiddenBuildings();
		town.removeAllBuildings();
		for(const auto & building : ed->getBuiltBuildings())
			town.addBuilding(building);
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void TownBuildingsDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList(QObject::tr("Built buildings:"));
	auto * ctown = town.getTown();
	for(const auto & buildingID : town.getBuildings())
	{
		if(buildingID == BuildingID::DEFAULT)
			continue;
		auto name = QString::fromStdString(ctown->buildings.at(buildingID)->getNameTranslated());
		if(name.isEmpty())
			name = QString::fromStdString(defaultBuildingIdConversion(buildingID));
		textList += name;
	}
	textList += QObject::tr("Forbidden buildings:");
	for(const auto & buildingID : town.forbiddenBuildings)
	{
		auto name = QString::fromStdString(ctown->buildings.at(buildingID)->getNameTranslated());
		if(name.isEmpty())
			name = QString::fromStdString(defaultBuildingIdConversion(buildingID));
		textList += name;
	}
	setModelTextData(model, index, textList);
}
