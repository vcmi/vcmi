/*
 * modsettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "modsettings.h"
#include "ui_modsettings.h"
#include "../mapcontroller.h"
#include "../../lib/modding/ModDescription.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/mapping/CMapService.h"

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

void ModSettings::initialize(MapController & c)
{
	AbstractSettings::initialize(c);

	//mods management
	//collect all active mods
	QMap<QString, QTreeWidgetItem*> addedMods;
	QSet<QString> modsToProcess;
	ui->treeMods->blockSignals(true);
	ui->treeMods->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->treeMods->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

	auto createModTreeWidgetItem = [&](QTreeWidgetItem * parent, const ModDescription & modInfo)
	{
		auto item = new QTreeWidgetItem(parent, {QString::fromStdString(modInfo.getName()), QString::fromStdString(modInfo.getVersion().toString())});
		item->setData(0, Qt::UserRole, QVariant(QString::fromStdString(modInfo.getID())));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(0, controller->map()->mods.count(modInfo.getID()) ? Qt::Checked : Qt::Unchecked);
		//set parent check
		if(parent && item->checkState(0) == Qt::Checked)
			parent->setCheckState(0, Qt::Checked);
		return item;
	};

	for(const auto & modName : LIBRARY->modh->getActiveMods())
	{
		if(modName == "core")
			continue;
		QString qmodName = QString::fromStdString(modName);
		if(qmodName.split(".").size() == 1)
		{
			const auto & modInfo = LIBRARY->modh->getModInfo(modName);
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

		pieces.pop_back();
		auto qs = pieces.join(".");

		if(addedMods.count(qs))
		{
			const auto & modInfo = LIBRARY->modh->getModInfo(qmodName.toStdString());
			addedMods[qmodName] = createModTreeWidgetItem(addedMods[qs], modInfo);
			modsToProcess.erase(qmodIter);
			qmodIter = modsToProcess.begin();
		}
		else
			++qmodIter;
	}

	ui->treeMods->blockSignals(false);
}

void ModSettings::update()
{
	//Mod management
	auto widgetAction = [&](QTreeWidgetItem * item)
	{
		if(item->checkState(0) == Qt::Checked)
		{
			auto modName = item->data(0, Qt::UserRole).toString().toStdString();
			controller->map()->mods[modName] = LIBRARY->modh->getModInfo(modName).getVerificationInfo();
		}
	};

	controller->map()->mods.clear();
	for (int i = 0; i < ui->treeMods->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = ui->treeMods->topLevelItem(i);
		traverseNode(item, widgetAction);
	}
}

void ModSettings::updateModWidgetBasedOnMods(const ModCompatibilityInfo & mods, bool leaveCheckedUnchanged)
{
	//Mod management
	auto widgetAction = [&](QTreeWidgetItem * item)
	{
		auto modName = item->data(0, Qt::UserRole).toString().toStdString();
		if(leaveCheckedUnchanged)
		{
			if(mods.count(modName))
				item->setCheckState(0, Qt::Checked);
		}
		else
		{
			item->setCheckState(0, mods.count(modName) ? Qt::Checked : Qt::Unchecked);
		}
	};

	for (int i = 0; i < ui->treeMods->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = ui->treeMods->topLevelItem(i);
		traverseNode(item, widgetAction);
	}
}

void ModSettings::on_modResolution_map_clicked()
{
	updateModWidgetBasedOnMods(MapController::modAssessmentMap(*controller->map()));
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
