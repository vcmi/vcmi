/*
 * factionselector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "factionselector.h"
#include "ui_factionselector.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/entities/faction/CTownHandler.h"

FactionSelector::FactionSelector(std::set<FactionID> & factions) :
	ui(new Ui::FactionSelector),
	factionsSelected(factions)
{
	ui->setupUi(this);

	setWindowTitle(tr("Faction Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	for(auto const & objectPtr : LIBRARY->townh->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(factions.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
		ui->listWidgetFactions->addItem(item);
	}

	show();
}

void FactionSelector::showFactionSelector(std::set<FactionID> & factions)
{
	auto * dialog = new FactionSelector(factions);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->exec();
}

void FactionSelector::on_buttonBoxResult_accepted()
{
	factionsSelected.clear();
	for(int i = 0; i < ui->listWidgetFactions->count(); ++i)
	{
		auto * item = ui->listWidgetFactions->item(i);
		if(item->checkState() == Qt::Checked)
			factionsSelected.insert(FactionID(item->data(Qt::UserRole).toInt()));
	}

    close();
}

void FactionSelector::on_buttonBoxResult_rejected()
{
    close();
}