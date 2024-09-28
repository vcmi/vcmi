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
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/constants/StringConstants.h"

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
	ui->eventFirstOccurrence->setValue(params.value("firstOccurrence").toInt());
	ui->eventRepeatAfter->setValue(params.value("nextOccurrence").toInt());

	auto playerList = params.value("players").toList();
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		bool isAffected = playerList.contains(toQString(PlayerColor(i)));
		auto * item = new QListWidgetItem(QString::fromStdString(GameConstants::PLAYER_COLOR_NAMES[i]));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setCheckState(isAffected ? Qt::Checked : Qt::Unchecked);
		ui->playersAffected->addItem(item);
	}

	ui->resources->setRowCount(GameConstants::RESOURCE_QUANTITY);
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
	{
		auto name = QString::fromStdString(GameConstants::RESOURCE_NAMES[i]);
		int val = params.value("resources").toMap().value(name).toInt();
		ui->resources->setItem(i, 0, new QTableWidgetItem(name));
		auto nval = new QTableWidgetItem(QString::number(val));
		nval->setFlags(nval->flags() | Qt::ItemIsEditable);
		ui->resources->setItem(i, 1, nval);
	}
	auto deletedObjectPositions = params.value("deletedObjectsPositions").toList();
	for(auto const & pos : deletedObjectPositions)
	{
		int3 position = pos.value<int3>();
		ui->deletedObjects->addItem(QString("x: %1, y: %2, z: %3").arg(position.x).arg(position.y).arg(position.z));
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
	descriptor["firstOccurrence"] = QVariant::fromValue(ui->eventFirstOccurrence->value());
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
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
	{
		auto * itemType = ui->resources->item(i, 0);
		auto * itemQty = ui->resources->item(i, 1);
		res[itemType->text()] = QVariant::fromValue(itemQty->text().toInt());
	}
	descriptor["resources"] = res;

	QVariantList deletedObjects;
	for(int i = 0; i < ui->deletedObjects->count(); ++i)
	{
		auto const & pos = ui->deletedObjects->item(i)->text();
		int3 position;
		position.x = pos.split(", ").at(0).split(": ").at(1).toInt();
		position.y = pos.split(", ").at(1).split(": ").at(1).toInt();
		position.z = pos.split(", ").at(2).split(": ").at(1).toInt();
		deletedObjects.push_back(QVariant::fromValue<int3>(position));
	}
	descriptor["deletedObjectsPositions"] = QVariant::fromValue(deletedObjects);

	target->setData(Qt::UserRole, descriptor);
	target->setText(ui->eventNameText->text());
}

void TimedEvent::on_addObjectToDelete_clicked()
{
	for(int lvl : {0, 1})
	{
		auto & l = controller.scene(lvl)->objectPickerView;
		l.highlight<const CGObjectInstance>();
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &TimedEvent::onObjectPicked);
	}
	hide();
	dynamic_cast<QWidget *>(parent()->parent()->parent()->parent()->parent()->parent()->parent())->hide();
}

void TimedEvent::on_removeObjectToDelete_clicked()
{
	delete ui->deletedObjects->takeItem(ui->deletedObjects->currentRow());
}

void TimedEvent::onObjectPicked(const CGObjectInstance * obj)
{
	show();
	dynamic_cast<QWidget *>(parent()->parent()->parent()->parent()->parent()->parent()->parent())->show();

	for(int lvl : {0, 1})
	{
		auto & l = controller.scene(lvl)->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &TimedEvent::onObjectPicked);
	}

	if(!obj) 
		return;
	ui->deletedObjects->addItem(QString("x: %1, y: %2, z: %3").arg(obj->pos.x).arg(obj->pos.y).arg(obj->pos.z));
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

