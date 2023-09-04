#include "StdInc.h"
#include "loseconditions.h"
#include "ui_loseconditions.h"

#include "../lib/CGeneralTextHandler.h"

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

void LoseConditions::initialize(const CMap & map)
{
	mapPointer = &map;

	//loss messages
	ui->defeatMessageEdit->setText(QString::fromStdString(map.defeatMessage.toString()));

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
		ui->loseComboBox->addItem(QString::fromStdString(s));
	}
	ui->standardLoseCheck->setChecked(false);

	for(auto & ev : map.triggeredEvents)
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
							if(json["objectType"].Integer() == Obj::TOWN)
							{
								ui->loseComboBox->setCurrentIndex(1);
								assert(loseTypeWidget);
								int townIdx = getObjectByPos<const CGTownInstance>(*mapPointer, posFromJson(json["position"]));
								if(townIdx >= 0)
								{
									auto idx = loseTypeWidget->findData(townIdx);
									loseTypeWidget->setCurrentIndex(idx);
								}
							}
							if(json["objectType"].Integer() == Obj::HERO)
							{
								ui->loseComboBox->setCurrentIndex(2);
								assert(loseTypeWidget);
								int heroIdx = getObjectByPos<const CGHeroInstance>(*mapPointer, posFromJson(json["position"]));
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

						case EventCondition::IS_HUMAN:
							break; //ignore because always applicable for defeat conditions
						}

					};
				}
			}
		}
	}
}

void LoseConditions::update(CMap & map)
{
	//loss messages
	map.defeatMessage = MetaString::createFromRawString(ui->defeatMessageEdit->text().toStdString());

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
		map.triggeredEvents.push_back(standardDefeat);
		map.defeatIconIndex = 3;
		map.defeatMessage.appendTextID("core.lcdesc.0");
	}
	else
	{
		int lossCondition = ui->loseComboBox->currentIndex() - 1;

		TriggeredEvent specialDefeat;
		specialDefeat.effect.type = EventEffect::DEFEAT;
		specialDefeat.identifier = "specialDefeat";
		specialDefeat.description.clear(); // TODO: display in quest window

		map.defeatIconIndex = lossCondition;

		switch(lossCondition)
		{
			case 0: {
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::TOWN;
				assert(loseTypeWidget);
				int townIdx = loseTypeWidget->currentData().toInt();
				cond.position = map.objects[townIdx]->pos;
				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill.appendTextID("core.genrltxt.251");
				specialDefeat.trigger = EventExpression(noneOf);
				map.defeatMessage.appendTextID("core.lcdesc.1");
				break;
			}

			case 1: {
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::HERO;
				assert(loseTypeWidget);
				int townIdx = loseTypeWidget->currentData().toInt();
				cond.position = map.objects[townIdx]->pos;
				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill.appendTextID("core.genrltxt.253");
				specialDefeat.trigger = EventExpression(noneOf);
				map.defeatMessage.appendTextID("core.lcdesc.2");
				break;
			}

			case 2: {
				EventCondition cond(EventCondition::DAYS_PASSED);
				assert(loseValueWidget);
				cond.value = expiredDate(loseValueWidget->text());
				specialDefeat.onFulfill.appendTextID("core.genrltxt.254");
				specialDefeat.trigger = EventExpression(cond);
				map.defeatMessage.appendTextID("core.lcdesc.3");
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
			map.triggeredEvents.push_back(standardDefeat);
		}
		map.triggeredEvents.push_back(specialDefeat);
	}

}

void LoseConditions::on_loseComboBox_currentIndexChanged(int index)
{
	delete loseTypeWidget;
	delete loseValueWidget;
	delete loseSelectWidget;
	loseTypeWidget = nullptr;
	loseValueWidget = nullptr;
	loseSelectWidget = nullptr;

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
			for(int i : getObjectIndexes<const CGTownInstance>(*mapPointer))
				loseTypeWidget->addItem(tr(getTownName(*mapPointer, i).c_str()), QVariant::fromValue(i));
			break;
		}

		case 1: { //EventCondition::CONTROL (Obj::HERO)
			loseTypeWidget = new QComboBox;
			ui->loseParamsLayout->addWidget(loseTypeWidget);
			for(int i : getObjectIndexes<const CGHeroInstance>(*mapPointer))
				loseTypeWidget->addItem(tr(getHeroName(*mapPointer, i).c_str()), QVariant::fromValue(i));
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

