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
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/spells/CSpellHandler.h"


MapSettings::MapSettings(MapController & ctrl, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MapSettings),
	controller(ctrl)
{
	ui->setupUi(this);

	setWindowModality(Qt::WindowModal);
	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	
	assert(controller.map());
	controller.settingsDialog = this;

	for(auto const & objectPtr : LIBRARY->skillh->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedAbilities.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
		ui->listAbilities->addItem(item);
	}
	for(auto const & objectPtr : LIBRARY->spellh->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedSpells.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
		ui->listSpells->addItem(item);
	}
	for(auto const & objectPtr : LIBRARY->arth->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedArtifact.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
		ui->listArts->addItem(item);
	}
	for(auto const & objectPtr : LIBRARY->heroh->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedHeroes.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
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
	controller.settingsDialog = nullptr;
	delete ui;
}

ModSettings * MapSettings::getModSettings()
{
	return ui->mods;
}

void MapSettings::on_pushButton_clicked()
{	
	auto updateMapArray = [](const QListWidget * widget, auto & arr)
	{
		arr.clear();
		for(int i = 0; i < widget->count(); ++i)
		{
			auto * item = widget->item(i);
			if (item->checkState() == Qt::Checked)
				arr.emplace(i);
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
