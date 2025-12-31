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
#include "eventsettings.h"
#include "../mapeditorroles.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/entities/ResourceTypeHandler.h"

TimedEvent::TimedEvent(MapController & c, QListWidgetItem * t, QWidget *parent) : 
	controller(c),
	QDialog(parent),
	ui(new Ui::TimedEvent),
	target(t)
{
	ui->setupUi(this);



	const auto params = t->data(Qt::UserRole).toMap();
	ui->eventNameText->setText(params.value("name").toString());
	ui->eventMessageText->setPlainText(params.value("message").toString());
	ui->eventAffectsCpu->setChecked(params.value("computerAffected").toBool());
	ui->eventAffectsHuman->setChecked(params.value("humanAffected").toBool());
	ui->eventFirstOccurrence->setValue(params.value("firstOccurrence").toInt() + 1);
	ui->eventRepeatAfter->setValue(params.value("nextOccurrence").toInt());

	auto playerList = params.value("players").toList();
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		bool isAffected = playerList.contains(toQString(PlayerColor(i)));
		MetaString str;
		str.appendName(PlayerColor(i));
		auto * item = new QListWidgetItem(QString::fromStdString(str.toString()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setCheckState(isAffected ? Qt::Checked : Qt::Unchecked);
		ui->playersAffected->addItem(item);
	}

	ui->resources->setRowCount(LIBRARY->resourceTypeHandler->getAllObjects().size());
	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		MetaString str;
		str.appendName(GameResID(i));
		auto name = QString::fromStdString(str.toString());
		int val = params.value("resources").toMap().value(QString::fromStdString(i.toResource()->getJsonKey())).toInt();
		ui->resources->setItem(i, 0, new QTableWidgetItem(name));
		auto nval = new QTableWidgetItem(QString::number(val));
		nval->setFlags(nval->flags() | Qt::ItemIsEditable);
		ui->resources->setItem(i, 1, nval);
	}
	auto deletedObjectInstances = params.value("deletedObjectsInstances").toList();
	for(auto const & idAsVariant : deletedObjectInstances)
	{
		auto id = ObjectInstanceID(idAsVariant.toInt());
		auto obj = controller.map()->objects[id];
		if(obj)
			insertObjectToDelete(obj.get());
	}
	show();
}

TimedEvent::~TimedEvent()
{
	delete ui;
}


void TimedEvent::on_TimedEvent_finished(int result)
{
	QVariantMap descriptor;
	descriptor["name"] = ui->eventNameText->text();
	descriptor["message"] = ui->eventMessageText->toPlainText();
	descriptor["humanAffected"] = QVariant::fromValue(ui->eventAffectsHuman->isChecked());
	descriptor["computerAffected"] = QVariant::fromValue(ui->eventAffectsCpu->isChecked());
	descriptor["firstOccurrence"] = QVariant::fromValue(ui->eventFirstOccurrence->value() - 1);
	descriptor["nextOccurrence"] = QVariant::fromValue(ui->eventRepeatAfter->value());

	QVariantList players;
	for(int i = 0; i < ui->playersAffected->count(); ++i)
	{
		auto * item = ui->playersAffected->item(i);
		if(item->checkState() == Qt::Checked)
			players.push_back(toQString(PlayerColor(i)));
	}
	descriptor["players"] = QVariant::fromValue(players);

	auto res = target->data(Qt::UserRole).toMap().value("resources").toMap();
	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		auto itemType = QString::fromStdString(i.toResource()->getJsonKey());
		auto * itemQty = ui->resources->item(i, 1);
		res[itemType] = QVariant::fromValue(itemQty->text().toInt());
	}
	descriptor["resources"] = res;

	QVariantList deletedObjects;
	for(int i = 0; i < ui->deletedObjects->count(); ++i)
	{
		auto const & item = ui->deletedObjects->item(i);
		auto data = item->data(MapEditorRoles::ObjectInstanceIDRole);
		auto id = ObjectInstanceID(data.value<int>());
		deletedObjects.push_back(QVariant::fromValue(id.num));
	}
	descriptor["deletedObjectsInstances"] = QVariant::fromValue(deletedObjects);

	target->setData(Qt::UserRole, descriptor);
	target->setText(ui->eventNameText->text());
}

void TimedEvent::on_addObjectToDelete_clicked()
{
	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.highlight<const CGObjectInstance>();
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &TimedEvent::onObjectPicked);
	}
	hide();
	controller.settingsDialog->hide();
}

void TimedEvent::on_removeObjectToDelete_clicked()
{
	delete ui->deletedObjects->takeItem(ui->deletedObjects->currentRow());
}

void TimedEvent::onObjectPicked(const CGObjectInstance * obj)
{
	show();
	controller.settingsDialog->show();

	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &TimedEvent::onObjectPicked);
	}

	if(!obj) 
		return;
	insertObjectToDelete(obj);
}

void TimedEvent::insertObjectToDelete(const CGObjectInstance * obj)
{
	QString objectLabel = QString("%1, x: %2, y: %3, z: %4").arg(QString::fromStdString(obj->getObjectName())).arg(obj->pos.x).arg(obj->pos.y).arg(obj->pos.z);
	auto * item = new QListWidgetItem(objectLabel);
	item->setData(MapEditorRoles::ObjectInstanceIDRole, QVariant::fromValue(obj->id.num));
	ui->deletedObjects->addItem(item);
}

void TimedEvent::on_pushButton_clicked()
{
	close();
}


void TimedEvent::on_resources_itemDoubleClicked(QTableWidgetItem *item)
{
	if(item && item->column() == 1)
	{
		ui->resources->editItem(item);
	}
}

