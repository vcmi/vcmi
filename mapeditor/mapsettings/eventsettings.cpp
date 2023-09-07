/*
 * eventsettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "eventsettings.h"
#include "ui_eventsettings.h"

EventSettings::EventSettings(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::EventSettings)
{
	ui->setupUi(this);
}

EventSettings::~EventSettings()
{
	delete ui;
}

void EventSettings::on_timedEventAdd_clicked()
{

}


void EventSettings::on_timedEventRemove_clicked()
{

}


void EventSettings::on_eventsList_itemActivated(QListWidgetItem *item)
{

}

