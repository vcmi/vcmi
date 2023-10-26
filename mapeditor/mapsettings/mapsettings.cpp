/*
 * mapsettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mapsettings.h"
#include "ui_mapsettings.h"
#include "mainwindow.h"

#include "../../lib/CSkillHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CArtHandler.h"
#include "../../lib/CHeroHandler.h"


MapSettings::MapSettings(MapController & ctrl, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MapSettings),
	controller(ctrl)
{
	ui->setupUi(this);

	assert(controller.map());
	
	show();

	for(int i = 0; i < controller.map()->allowedAbilities.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->skillh->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedAbilities[i] ? Qt::Checked : Qt::Unchecked);
		ui->listAbilities->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedSpells.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->spellh->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedSpells[i] ? Qt::Checked : Qt::Unchecked);
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

	ui->general->initialize(controller);
	ui->mods->initialize(controller);
	ui->victory->initialize(controller);
	ui->lose->initialize(controller);
	ui->events->initialize(controller);
	ui->rumors->initialize(controller);
}

MapSettings::~MapSettings()
{
	delete ui;
}

void MapSettings::on_pushButton_clicked()
{	
	auto updateMapArray = [](const QListWidget * widget, std::vector<bool> & arr)
	{
		for(int i = 0; i < arr.size(); ++i)
		{
			auto * item = widget->item(i);
			arr[i] = item->checkState() == Qt::Checked;
		}
	};
	
	updateMapArray(ui->listAbilities, controller.map()->allowedAbilities);
	updateMapArray(ui->listSpells, controller.map()->allowedSpells);
	updateMapArray(ui->listArts, controller.map()->allowedArtifact);
	updateMapArray(ui->listHeroes, controller.map()->allowedHeroes);

	controller.map()->triggeredEvents.clear();

	ui->general->update();
	ui->mods->update();
	ui->victory->update();
	ui->lose->update();
	ui->events->update();
	ui->rumors->update();

	controller.commitChangeWithoutRedraw();

	close();
}
