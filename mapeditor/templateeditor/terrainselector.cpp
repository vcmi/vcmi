/*
 * terrainselector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "terrainselector.h"
#include "ui_terrainselector.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/TerrainHandler.h"

TerrainSelector::TerrainSelector(std::set<TerrainId> & terrains) :
	ui(new Ui::TerrainSelector),
	terrainsSelected(terrains)
{
	ui->setupUi(this);

	setWindowTitle(tr("Terrain Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	for(auto const & objectPtr : LIBRARY->terrainTypeHandler->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(terrains.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
		ui->listWidgetTerrains->addItem(item);
	}

	show();
}

void TerrainSelector::showTerrainSelector(std::set<TerrainId> & terrains)
{
	auto * dialog = new TerrainSelector(terrains);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->exec();
}

void TerrainSelector::on_buttonBoxResult_accepted()
{
	terrainsSelected.clear();
	for(int i = 0; i < ui->listWidgetTerrains->count(); ++i)
	{
		auto * item = ui->listWidgetTerrains->item(i);
		if(item->checkState() == Qt::Checked)
			terrainsSelected.insert(item->data(Qt::UserRole).toInt());
	}

    close();
}

void TerrainSelector::on_buttonBoxResult_rejected()
{
    close();
}