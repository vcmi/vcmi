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

TimedEvent::TimedEvent(QListWidgetItem * t, QWidget *parent) :
	QDialog(parent),
	target(t),
	ui(new Ui::TimedEvent)
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
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
	{
		auto * itemType = ui->resources->item(i, 0);
		auto * itemQty = ui->resources->item(i, 1);
		res[itemType->text()] = QVariant::fromValue(itemQty->text().toInt());
	}
	descriptor["resources"] = res;

	target->setData(Qt::UserRole, descriptor);
	target->setText(ui->eventNameText->text());
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

