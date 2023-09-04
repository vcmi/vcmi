#include "StdInc.h"
#include "modsettings.h"
#include "ui_modsettings.h"
#include "../mapcontroller.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/modding/CModInfo.h"

void traverseNode(QTreeWidgetItem * item, std::function<void(QTreeWidgetItem*)> action)
{
	// Do something with item
	action(item);
	for (int i = 0; i < item->childCount(); ++i)
		traverseNode(item->child(i), action);
}

ModSettings::ModSettings(QWidget *parent) :
	AbstractSettings(parent),
	ui(new Ui::ModSettings)
{
	ui->setupUi(this);
}

ModSettings::~ModSettings()
{
	delete ui;
}

void ModSettings::initialize(const CMap & map)
{
	mapPointer = &map;

	//mods management
	//collect all active mods
	QMap<QString, QTreeWidgetItem*> addedMods;
	QSet<QString> modsToProcess;
	ui->treeMods->blockSignals(true);

	auto createModTreeWidgetItem = [&](QTreeWidgetItem * parent, const CModInfo & modInfo)
	{
		auto item = new QTreeWidgetItem(parent, {QString::fromStdString(modInfo.name), QString::fromStdString(modInfo.version.toString())});
		item->setData(0, Qt::UserRole, QVariant(QString::fromStdString(modInfo.identifier)));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(0, map.mods.count(modInfo.identifier) ? Qt::Checked : Qt::Unchecked);
		//set parent check
		if(parent && item->checkState(0) == Qt::Checked)
			parent->setCheckState(0, Qt::Checked);
		return item;
	};

	for(const auto & modName : VLC->modh->getActiveMods())
	{
		QString qmodName = QString::fromStdString(modName);
		if(qmodName.split(".").size() == 1)
		{
			const auto & modInfo = VLC->modh->getModInfo(modName);
			addedMods[qmodName] = createModTreeWidgetItem(nullptr, modInfo);
			ui->treeMods->addTopLevelItem(addedMods[qmodName]);
		}
		else
		{
			modsToProcess.insert(qmodName);
		}
	}

	for(auto qmodIter = modsToProcess.begin(); qmodIter != modsToProcess.end();)
	{
		auto qmodName = *qmodIter;
		auto pieces = qmodName.split(".");
		assert(pieces.size() > 1);

		QString qs;
		for(int i = 0; i < pieces.size() - 1; ++i)
			qs += pieces[i];

		if(addedMods.count(qs))
		{
			const auto & modInfo = VLC->modh->getModInfo(qmodName.toStdString());
			addedMods[qmodName] = createModTreeWidgetItem(addedMods[qs], modInfo);
			modsToProcess.erase(qmodIter);
			qmodIter = modsToProcess.begin();
		}
		else
			++qmodIter;
	}

	ui->treeMods->blockSignals(false);
}

void ModSettings::update(CMap & map)
{
	//Mod management
	auto widgetAction = [&](QTreeWidgetItem * item)
	{
		if(item->checkState(0) == Qt::Checked)
		{
			auto modName = item->data(0, Qt::UserRole).toString().toStdString();
			map.mods[modName] = VLC->modh->getModInfo(modName).version;
		}
	};

	map.mods.clear();
	for (int i = 0; i < ui->treeMods->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = ui->treeMods->topLevelItem(i);
		traverseNode(item, widgetAction);
	}
}

void ModSettings::updateModWidgetBasedOnMods(const ModCompatibilityInfo & mods)
{
	//Mod management
	auto widgetAction = [&](QTreeWidgetItem * item)
	{
		auto modName = item->data(0, Qt::UserRole).toString().toStdString();
		item->setCheckState(0, mods.count(modName) ? Qt::Checked : Qt::Unchecked);
	};

	for (int i = 0; i < ui->treeMods->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = ui->treeMods->topLevelItem(i);
		traverseNode(item, widgetAction);
	}
}

void ModSettings::on_modResolution_map_clicked()
{
	updateModWidgetBasedOnMods(MapController::modAssessmentMap(*mapPointer));
}


void ModSettings::on_modResolution_full_clicked()
{
	updateModWidgetBasedOnMods(MapController::modAssessmentAll());
}

void ModSettings::on_treeMods_itemChanged(QTreeWidgetItem *item, int column)
{
	//set state for children
	for (int i = 0; i < item->childCount(); ++i)
		item->child(i)->setCheckState(0, item->checkState(0));

	//set state for parent
	ui->treeMods->blockSignals(true);
	if(item->checkState(0) == Qt::Checked)
	{
		while(item->parent())
		{
			item->parent()->setCheckState(0, Qt::Checked);
			item = item->parent();
		}
	}
	ui->treeMods->blockSignals(false);
}
