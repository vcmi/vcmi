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
#include "timedevent.h"
#include "ui_eventsettings.h"
#include "../mapcontroller.h"
#include "../../lib/mapping/CMapDefines.h"
#include "../../lib/constants/NumericConstants.h"
#include "../../lib/constants/StringConstants.h"

QVariant toVariant(const TResources & resources)
{
	QVariantMap result;
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
		result[QString::fromStdString(GameConstants::RESOURCE_NAMES[i])] = QVariant::fromValue(resources[i]);
	return result;
}

TResources resourcesFromVariant(const QVariant & v)
{
	JsonNode vJson;
	for(auto r : v.toMap().keys())
		vJson[r.toStdString()].Integer() = v.toMap().value(r).toInt();
	return TResources(vJson);

}

QVariant toVariant(const CMapEvent & event)
{
	QVariantMap result;
	result["name"] = QString::fromStdString(event.name);
	result["message"] = QString::fromStdString(event.message.toString());
	result["players"] = QVariant::fromValue(event.players);
	result["humanAffected"] = QVariant::fromValue(event.humanAffected);
	result["computerAffected"] = QVariant::fromValue(event.computerAffected);
	result["firstOccurence"] = QVariant::fromValue(event.firstOccurence);
	result["nextOccurence"] = QVariant::fromValue(event.nextOccurence);
	result["resources"] = toVariant(event.resources);
	return QVariant(result);
}

CMapEvent eventFromVariant(CMapHeader & mapHeader, const QVariant & variant)
{
	CMapEvent result;
	auto v = variant.toMap();
	result.name = v.value("name").toString().toStdString();
	result.message.appendTextID(mapRegisterLocalizedString("map", mapHeader, TextIdentifier("header", "event", result.name, "message"), v.value("message").toString().toStdString()));
	result.players = v.value("players").toInt();
	result.humanAffected = v.value("humanAffected").toInt();
	result.computerAffected = v.value("computerAffected").toInt();
	result.firstOccurence = v.value("firstOccurence").toInt();
	result.nextOccurence = v.value("nextOccurence").toInt();
	result.resources = resourcesFromVariant(v.value("resources"));
	return result;
}

EventSettings::EventSettings(QWidget *parent) :
	AbstractSettings(parent),
	ui(new Ui::EventSettings)
{
	ui->setupUi(this);
}

EventSettings::~EventSettings()
{
	delete ui;
}

void EventSettings::initialize(MapController & c)
{
	AbstractSettings::initialize(c);
	for(const auto & event : controller->map()->events)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(event.name));
		item->setData(Qt::UserRole, toVariant(event));
		ui->eventsList->addItem(item);
	}
}

void EventSettings::update()
{
	controller->map()->events.clear();
	for(int i = 0; i < ui->eventsList->count(); ++i)
	{
		const auto * item = ui->eventsList->item(i);
		controller->map()->events.push_back(eventFromVariant(*controller->map(), item->data(Qt::UserRole)));
	}
}

void EventSettings::on_timedEventAdd_clicked()
{
	CMapEvent event;
	event.name = tr("New event").toStdString();
	auto * item = new QListWidgetItem(QString::fromStdString(event.name));
	item->setData(Qt::UserRole, toVariant(event));
	ui->eventsList->addItem(item);
	on_eventsList_itemActivated(item);
}


void EventSettings::on_timedEventRemove_clicked()
{
	if(auto * item = ui->eventsList->currentItem())
		ui->eventsList->takeItem(ui->eventsList->row(item));
}


void EventSettings::on_eventsList_itemActivated(QListWidgetItem *item)
{
	new TimedEvent(item, parentWidget());
}

