/*
 * mapsettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "mapsettings.h"
#include "ui_mapsettings.h"
#include "mainwindow.h"

#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CHeroHandler.h"

MapSettings::MapSettings(MapController & ctrl, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MapSettings),
	controller(ctrl)
{
	ui->setupUi(this);

	assert(controller.map());

	ui->mapNameEdit->setText(tr(controller.map()->name.c_str()));
	ui->mapDescriptionEdit->setPlainText(tr(controller.map()->description.c_str()));
	
	show();
	
	
	for(int i = 0; i < controller.map()->allowedAbilities.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->skillh->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedAbilities[i] ? Qt::Checked : Qt::Unchecked);
		ui->listAbilities->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedSpell.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->spellh->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedSpell[i] ? Qt::Checked : Qt::Unchecked);
		ui->listSpells->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedArtifact.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->arth->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedArtifact[i] ? Qt::Checked : Qt::Unchecked);
		ui->listArts->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedHeroes.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->heroh->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedHeroes[i] ? Qt::Checked : Qt::Unchecked);
		ui->listHeroes->addItem(item);
	}
	
	//set difficulty
	switch(controller.map()->difficulty)
	{
		case 0:
			ui->diffRadio1->setChecked(true);
			break;
			
		case 1:
			ui->diffRadio2->setChecked(true);
			break;
			
		case 2:
			ui->diffRadio3->setChecked(true);
			break;
			
		case 3:
			ui->diffRadio4->setChecked(true);
			break;
			
		case 4:
			ui->diffRadio5->setChecked(true);
			break;
	};
	
	//victory & loss messages
	ui->victoryMessageEdit->setText(QString::fromStdString(controller.map()->victoryMessage));
	ui->defeatMessageEdit->setText(QString::fromStdString(controller.map()->defeatMessage));
	
	//victory & loss conditions
	for(auto & ev : controller.map()->triggeredEvents)
	{
		if(ev.effect.type == EventEffect::VICTORY)
		{
			if(ev.identifier == "standardVictory")
				ui->standardVictoryCheck->setChecked(true);

			if(ev.identifier == "specialVictory")
			{
				if(ev.trigger.get() == EventCondition::HAVE_ARTIFACT)
					
			}
		}
	}

	//ui8 difficulty;
	//ui8 levelLimit;

	//std::string victoryMessage;
	//std::string defeatMessage;
	//ui16 victoryIconIndex;
	//ui16 defeatIconIndex;

	//std::vector<PlayerInfo> players; /// The default size of the vector is PlayerColor::PLAYER_LIMIT.
}

MapSettings::~MapSettings()
{
	delete ui;
}

void MapSettings::on_pushButton_clicked()
{
	controller.map()->name = ui->mapNameEdit->text().toStdString();
	controller.map()->description = ui->mapDescriptionEdit->toPlainText().toStdString();
	controller.commitChangeWithoutRedraw();
	
	for(int i = 0; i < controller.map()->allowedAbilities.size(); ++i)
	{
		auto * item = ui->listAbilities->item(i);
		controller.map()->allowedAbilities[i] = item->checkState() == Qt::Checked;
	}
	for(int i = 0; i < controller.map()->allowedSpell.size(); ++i)
	{
		auto * item = ui->listSpells->item(i);
		controller.map()->allowedSpell[i] = item->checkState() == Qt::Checked;
	}
	for(int i = 0; i < controller.map()->allowedArtifact.size(); ++i)
	{
		auto * item = ui->listArts->item(i);
		controller.map()->allowedArtifact[i] = item->checkState() == Qt::Checked;
	}
	for(int i = 0; i < controller.map()->allowedHeroes.size(); ++i)
	{
		auto * item = ui->listHeroes->item(i);
		controller.map()->allowedHeroes[i] = item->checkState() == Qt::Checked;
	}
	
	close();
}
