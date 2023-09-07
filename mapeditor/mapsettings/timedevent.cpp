/*
 * timedevent.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "timedevent.h"
#include "ui_timedevent.h"

TimedEvent::TimedEvent(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::TimedEvent)
{
	ui->setupUi(this);
}

TimedEvent::~TimedEvent()
{
	delete ui;
}

void TimedEvent::on_eventResources_clicked()
{

}


void TimedEvent::on_TimedEvent_finished(int result)
{

}

