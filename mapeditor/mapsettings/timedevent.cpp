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
	ui->eventFirstOccurance->setValue(params.value("firstOccurence").toInt());
	ui->eventRepeatAfter->setValue(params.value("nextOccurence").toInt());

	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		bool isAffected = (1 << i) & params.value("players").toInt();
		auto * item = new QListWidgetItem(QString::fromStdString(GameConstants::PLAYER_COLOR_NAMES[i]));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setCheckState(isAffected ? Qt::Checked : Qt::Unchecked);
		ui->playersAffected->addItem(item);
	}
	//result.resources = resourcesFromVariant(v.value("resources"));

	show();
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
	QVariantMap descriptor;
	descriptor["name"] = ui->eventNameText->text();
	descriptor["message"] = ui->eventMessageText->toPlainText();
	descriptor["humanAffected"] = QVariant::fromValue(ui->eventAffectsHuman->isChecked());
	descriptor["computerAffected"] = QVariant::fromValue(ui->eventAffectsCpu->isChecked());
	descriptor["firstOccurence"] = QVariant::fromValue(ui->eventFirstOccurance->value());
	descriptor["nextOccurence"] = QVariant::fromValue(ui->eventRepeatAfter->value());

	int players = 0;
	for(int i = 0; i < ui->playersAffected->count(); ++i)
	{
		auto * item = ui->playersAffected->item(i);
		if(item->checkState() == Qt::Checked)
			players |= 1 << i;
	}
	descriptor["players"] = QVariant::fromValue(players);
	target->setData(Qt::UserRole, descriptor);
	target->setText(ui->eventNameText->text());
}

