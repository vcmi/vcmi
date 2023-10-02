/*
 * rumorsettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "rumorsettings.h"
#include "ui_rumorsettings.h"
#include "../mapcontroller.h"

RumorSettings::RumorSettings(QWidget *parent) :
	AbstractSettings(parent),
	ui(new Ui::RumorSettings)
{
	ui->setupUi(this);
}

RumorSettings::~RumorSettings()
{
	delete ui;
}

void RumorSettings::initialize(MapController & c)
{
	AbstractSettings::initialize(c);
	for(auto & rumor : controller->map()->rumors)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(rumor.name));
		item->setData(Qt::UserRole, QVariant(QString::fromStdString(rumor.text.toString())));
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		ui->rumors->addItem(item);
	}
}

void RumorSettings::update()
{
	controller->map()->rumors.clear();
	for(int i = 0; i < ui->rumors->count(); ++i)
	{
		Rumor rumor;
		rumor.name = ui->rumors->item(i)->text().toStdString();
		rumor.text.appendTextID(mapRegisterLocalizedString("map", *controller->map(), TextIdentifier("header", "rumor", i, "text"), ui->rumors->item(i)->data(Qt::UserRole).toString().toStdString()));
		controller->map()->rumors.push_back(rumor);
	}
}

void RumorSettings::on_message_textChanged()
{
	if(auto item = ui->rumors->currentItem())
		item->setData(Qt::UserRole, QVariant(ui->message->toPlainText()));
}

void RumorSettings::on_add_clicked()
{
	auto * item = new QListWidgetItem(tr("New rumor"));
	item->setData(Qt::UserRole, QVariant(""));
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	ui->rumors->addItem(item);
	emit ui->rumors->itemActivated(item);
}

void RumorSettings::on_remove_clicked()
{
	if(auto item = ui->rumors->currentItem())
	{
		ui->message->setPlainText("");
		ui->rumors->takeItem(ui->rumors->row(item));
	}
}


void RumorSettings::on_rumors_itemSelectionChanged()
{
	if(auto item = ui->rumors->currentItem())
		ui->message->setPlainText(item->data(Qt::UserRole).toString());
}



