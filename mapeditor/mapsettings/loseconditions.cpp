/*
 * loseconditions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "loseconditions.h"
#include "ui_loseconditions.h"
#include "../mapcontroller.h"
#include "../lib/texts/CGeneralTextHandler.h"

LoseConditions::LoseConditions(QWidget *parent) :
	AbstractSettings(parent),
	ui(new Ui::LoseConditions)
{
	ui->setupUi(this);
}

LoseConditions::~LoseConditions()
{
	delete ui;
}

void LoseConditions::initialize(MapController & c)
{
	AbstractSettings::initialize(c);

	//loss messages
	ui->defeatMessageEdit->setText(QString::fromStdString(controller->map()->defeatMessage.toString()));

	//loss conditions
	const std::array<std::string, 5> conditionStringsLose = {
		QT_TR_NOOP("No special loss"),
		QT_TR_NOOP("Lose castle"),
		QT_TR_NOOP("Lose hero"),
		QT_TR_NOOP("Time expired"),
		QT_TR_NOOP("Days without town")
	};

	for(auto & s : conditionStringsLose)
	{
		ui->loseComboBox->addItem(tr(s.c_str()));
	}
	ui->standardLoseCheck->setChecked(false);

	for(auto & ev : controller->map()->triggeredEvents)
	{
		if(ev.effect.type == EventEffect::DEFEAT)
		{
			if(ev.identifier == "standardDefeat")
				ui->standardLoseCheck->setChecked(true);

			if(ev.identifier == "specialDefeat")
			{
				auto readjson = ev.trigger.toJson(AbstractSettings::conditionToJson);
				auto linearNodes = linearJsonArray(readjson);

				for(auto & json : linearNodes)
				{
					switch(json["condition"].Integer())
					{
						case EventCondition::CONTROL: {
							auto objectType = MapObjectID::decode(json["objectType"].String());
							if(objectType == Obj::TOWN)
							{
								ui->loseComboBox->setCurrentIndex(1);
								assert(loseTypeWidget);
								int townIdx = getObjectByPos<const CGTownInstance>(*controller->map(), posFromJson(json["position"]));
								if(townIdx >= 0)
								{
									auto idx = loseTypeWidget->findData(townIdx);
									loseTypeWidget->setCurrentIndex(idx);
								}
							}
							if(objectType == Obj::HERO)
							{
								ui->loseComboBox->setCurrentIndex(2);
								assert(loseTypeWidget);
								int heroIdx = getObjectByPos<const CGHeroInstance>(*controller->map(), posFromJson(json["position"]));
								if(heroIdx >= 0)
								{
									auto idx = loseTypeWidget->findData(heroIdx);
									loseTypeWidget->setCurrentIndex(idx);
								}
							}

							break;
						}

						case EventCondition::DAYS_PASSED: {
							ui->loseComboBox->setCurrentIndex(3);
							assert(loseValueWidget);
							loseValueWidget->setText(expiredDate(json["value"].Integer()));
							break;
						}

						case EventCondition::DAYS_WITHOUT_TOWN: {
							ui->loseComboBox->setCurrentIndex(4);
							assert(loseValueWidget);
							loseValueWidget->setText(QString::number(json["value"].Integer()));
							break;
						}

						case EventCondition::IS_HUMAN: {
							break; //ignore because always applicable for defeat conditions
						}

					};
				}
			}
		}
	}
}

void LoseConditions::update()
{
	//loss messages
	bool customMessage = true;

	//loss conditions
	EventCondition defeatCondition(EventCondition::DAYS_WITHOUT_TOWN);
	defeatCondition.value = 7;

	//Loss condition - 7 days without town
	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage.appendTextID("core.genrltxt.8");
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description.clear(); // TODO: display in quest window
	standardDefeat.onFulfill.appendTextID("core.genrltxt.7");
	standardDefeat.trigger = EventExpression(defeatCondition);

	//DEFEAT
	if(ui->loseComboBox->currentIndex() == 0)
	{
		controller->map()->triggeredEvents.push_back(standardDefeat);
		controller->map()->defeatIconIndex = 3;
		controller->map()->defeatMessage = MetaString::createFromTextID("core.lcdesc.0");
		customMessage = false;
	}
	else
	{
		int lossCondition = ui->loseComboBox->currentIndex() - 1;

		TriggeredEvent specialDefeat;
		specialDefeat.effect.type = EventEffect::DEFEAT;
		specialDefeat.identifier = "specialDefeat";
		specialDefeat.description.clear(); // TODO: display in quest window

		controller->map()->defeatIconIndex = lossCondition;

		switch(lossCondition)
		{
			case 0: {
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj(Obj::TOWN);
				assert(loseTypeWidget);
				int townIdx = loseTypeWidget->currentData().toInt();
				cond.position = controller->map()->objects[townIdx]->pos;
				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill.appendTextID("core.genrltxt.251");
				specialDefeat.trigger = EventExpression(noneOf);
				controller->map()->defeatMessage = MetaString::createFromTextID("core.lcdesc.1");
				customMessage = false;
				break;
			}

			case 1: {
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj(Obj::HERO);
				assert(loseTypeWidget);
				int townIdx = loseTypeWidget->currentData().toInt();
				cond.position = controller->map()->objects[townIdx]->pos;
				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill.appendTextID("core.genrltxt.253");
				specialDefeat.trigger = EventExpression(noneOf);
				controller->map()->defeatMessage = MetaString::createFromTextID("core.lcdesc.2");
				customMessage = false;
				break;
			}

			case 2: {
				EventCondition cond(EventCondition::DAYS_PASSED);
				assert(loseValueWidget);
				cond.value = expiredDate(loseValueWidget->text());
				specialDefeat.onFulfill.appendTextID("core.genrltxt.254");
				specialDefeat.trigger = EventExpression(cond);
				controller->map()->defeatMessage = MetaString::createFromTextID("core.lcdesc.3");
				customMessage = false;
				break;
			}

			case 3: {
				EventCondition cond(EventCondition::DAYS_WITHOUT_TOWN);
				assert(loseValueWidget);
				cond.value = loseValueWidget->text().toInt();
				specialDefeat.onFulfill.appendTextID("core.genrltxt.7");
				specialDefeat.trigger = EventExpression(cond);
				break;
			}
		}

		EventExpression::OperatorAll allOf;
		EventCondition isHuman(EventCondition::IS_HUMAN);
		isHuman.value = 1;

		allOf.expressions.push_back(specialDefeat.trigger.get());
		allOf.expressions.push_back(isHuman);
		specialDefeat.trigger = EventExpression(allOf);

		if(ui->standardLoseCheck->isChecked())
		{
			controller->map()->triggeredEvents.push_back(standardDefeat);
		}
		controller->map()->triggeredEvents.push_back(specialDefeat);
	}

	if(customMessage)
	{
		controller->map()->defeatMessage = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller->map(), TextIdentifier("header", "defeatMessage"), ui->defeatMessageEdit->text().toStdString()));
	}
}

void LoseConditions::on_loseComboBox_currentIndexChanged(int index)
{
	delete loseTypeWidget;
	delete loseValueWidget;
	delete loseSelectWidget;
	delete pickObjectButton;
	loseTypeWidget = nullptr;
	loseValueWidget = nullptr;
	loseSelectWidget = nullptr;
	pickObjectButton = nullptr;

	if(index == 0)
	{
		ui->standardLoseCheck->setChecked(true);
		ui->standardLoseCheck->setEnabled(false);
		return;
	}
	ui->standardLoseCheck->setEnabled(true);

	int loseCondition = index - 1;
	switch(loseCondition)
	{
		case 0: {  //EventCondition::CONTROL (Obj::TOWN)
			loseTypeWidget = new QComboBox;
			ui->loseParamsLayout->addWidget(loseTypeWidget);
			for(int i : getObjectIndexes<const CGTownInstance>(*controller->map()))
				loseTypeWidget->addItem(QString::fromStdString(getTownName(*controller->map(), i).c_str()), QVariant::fromValue(i));
			pickObjectButton = new QToolButton;
			connect(pickObjectButton, &QToolButton::clicked, this, &LoseConditions::onObjectSelect);
			ui->loseParamsLayout->addWidget(pickObjectButton);
			break;
		}

		case 1: { //EventCondition::CONTROL (Obj::HERO)
			loseTypeWidget = new QComboBox;
			ui->loseParamsLayout->addWidget(loseTypeWidget);
			for(int i : getObjectIndexes<const CGHeroInstance>(*controller->map()))
				loseTypeWidget->addItem(QString::fromStdString(getHeroName(*controller->map(), i).c_str()), QVariant::fromValue(i));
			pickObjectButton = new QToolButton;
			connect(pickObjectButton, &QToolButton::clicked, this, &LoseConditions::onObjectSelect);
			ui->loseParamsLayout->addWidget(pickObjectButton);
			break;
		}

		case 2: { //EventCondition::DAYS_PASSED
			loseValueWidget = new QLineEdit;
			ui->loseParamsLayout->addWidget(loseValueWidget);
			loseValueWidget->setText("2m 1w 1d");
			break;
		}

		case 3: { //EventCondition::DAYS_WITHOUT_TOWN
			loseValueWidget = new QLineEdit;
			ui->loseParamsLayout->addWidget(loseValueWidget);
			loseValueWidget->setText("7");
			break;
		}
	}
}

void LoseConditions::onObjectSelect()
{
	int loseCondition = ui->loseComboBox->currentIndex() - 1;
	for(MapScene * level : controller->getScenes())
	{
		auto & l = level->objectPickerView;
		switch(loseCondition)
		{
			case 0: {  //EventCondition::CONTROL (Obj::TOWN)
				l.highlight<const CGTownInstance>();
				break;
			}
				
			case 1: { //EventCondition::CONTROL (Obj::HERO)
				l.highlight<const CGHeroInstance>();
				break;
			}
			default:
				return;
		}
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &LoseConditions::onObjectPicked);
	}
	
	controller->settingsDialog->hide();
}

void LoseConditions::onObjectPicked(const CGObjectInstance * obj)
{
	controller->settingsDialog->show();
	
	for(MapScene * level : controller->getScenes())
	{
		auto & l = level->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &LoseConditions::onObjectPicked);
	}
	
	if(!obj) //discarded
		return;
	
	for(int i = 0; i < loseTypeWidget->count(); ++i)
	{
		auto data = controller->map()->objects.at(loseTypeWidget->itemData(i).toInt());
		if(data.get() == obj)
		{
			loseTypeWidget->setCurrentIndex(i);
			break;
		}
	}
}
