#include "StdInc.h"
#include "victoryconditions.h"
#include "ui_victoryconditions.h"

#include "../lib/CGeneralTextHandler.h"
#include "../lib/constants/StringConstants.h"

#include "../inspector/townbulidingswidget.h" //to convert BuildingID to string

VictoryConditions::VictoryConditions(QWidget *parent) :
	AbstractSettings(parent),
	ui(new Ui::VictoryConditions)
{
	ui->setupUi(this);
}

void VictoryConditions::initialize(const CMap & map)
{
	mapPointer = &map;

	//victory message
	ui->victoryMessageEdit->setText(QString::fromStdString(map.victoryMessage.toString()));

	//victory conditions
	const std::array<std::string, 8> conditionStringsWin = {
		QT_TR_NOOP("No special victory"),
		QT_TR_NOOP("Capture artifact"),
		QT_TR_NOOP("Hire creatures"),
		QT_TR_NOOP("Accumulate resources"),
		QT_TR_NOOP("Construct building"),
		QT_TR_NOOP("Capture town"),
		QT_TR_NOOP("Defeat hero"),
		QT_TR_NOOP("Transport artifact")
	};

	for(auto & s : conditionStringsWin)
	{
		ui->victoryComboBox->addItem(QString::fromStdString(s));
	}
	ui->standardVictoryCheck->setChecked(false);
	ui->onlyForHumansCheck->setChecked(false);

	for(auto & ev : map.triggeredEvents)
	{
		if(ev.effect.type == EventEffect::VICTORY)
		{
			if(ev.identifier == "standardVictory")
				ui->standardVictoryCheck->setChecked(true);

			if(ev.identifier == "specialVictory")
			{
				auto readjson = ev.trigger.toJson(AbstractSettings::conditionToJson);
				auto linearNodes = linearJsonArray(readjson);

				for(auto & json : linearNodes)
				{
					switch(json["condition"].Integer())
					{
						case EventCondition::HAVE_ARTIFACT: {
							ui->victoryComboBox->setCurrentIndex(1);
							assert(victoryTypeWidget);
							victoryTypeWidget->setCurrentIndex(json["objectType"].Integer());
							break;
						}

						case EventCondition::HAVE_CREATURES: {
							ui->victoryComboBox->setCurrentIndex(2);
							assert(victoryTypeWidget);
							assert(victoryValueWidget);
							auto idx = victoryTypeWidget->findData(int(json["objectType"].Integer()));
							victoryTypeWidget->setCurrentIndex(idx);
							victoryValueWidget->setText(QString::number(json["value"].Integer()));
							break;
						}

						case EventCondition::HAVE_RESOURCES: {
							ui->victoryComboBox->setCurrentIndex(3);
							assert(victoryTypeWidget);
							assert(victoryValueWidget);
							auto idx = victoryTypeWidget->findData(int(json["objectType"].Integer()));
							victoryTypeWidget->setCurrentIndex(idx);
							victoryValueWidget->setText(QString::number(json["value"].Integer()));
							break;
						}

						case EventCondition::HAVE_BUILDING: {
							ui->victoryComboBox->setCurrentIndex(4);
							assert(victoryTypeWidget);
							assert(victorySelectWidget);
							auto idx = victoryTypeWidget->findData(int(json["objectType"].Integer()));
							victoryTypeWidget->setCurrentIndex(idx);
							int townIdx = getObjectByPos<const CGTownInstance>(*mapPointer, posFromJson(json["position"]));
							if(townIdx >= 0)
							{
								auto idx = victorySelectWidget->findData(townIdx);
								victorySelectWidget->setCurrentIndex(idx);
							}
							break;
						}

						case EventCondition::CONTROL: {
							ui->victoryComboBox->setCurrentIndex(5);
							assert(victoryTypeWidget);
							if(json["objectType"].Integer() == Obj::TOWN)
							{
								int townIdx = getObjectByPos<const CGTownInstance>(*mapPointer, posFromJson(json["position"]));
								if(townIdx >= 0)
								{
									auto idx = victoryTypeWidget->findData(townIdx);
									victoryTypeWidget->setCurrentIndex(idx);
								}
							}
							//TODO: support control other objects (dwellings, mines)
							break;
						}

						case EventCondition::DESTROY: {
							ui->victoryComboBox->setCurrentIndex(6);
							assert(victoryTypeWidget);
							if(json["objectType"].Integer() == Obj::HERO)
							{
								int heroIdx = getObjectByPos<const CGHeroInstance>(*mapPointer, posFromJson(json["position"]));
								if(heroIdx >= 0)
								{
									auto idx = victoryTypeWidget->findData(heroIdx);
									victoryTypeWidget->setCurrentIndex(idx);
								}
							}
							//TODO: support control other objects (monsters)
							break;
						}

						case EventCondition::TRANSPORT: {
							ui->victoryComboBox->setCurrentIndex(7);
							assert(victoryTypeWidget);
							assert(victorySelectWidget);
							victoryTypeWidget->setCurrentIndex(json["objectType"].Integer());
							int townIdx = getObjectByPos<const CGTownInstance>(*mapPointer, posFromJson(json["position"]));
							if(townIdx >= 0)
							{
								auto idx = victorySelectWidget->findData(townIdx);
								victorySelectWidget->setCurrentIndex(idx);
							}
							break;
						}

						case EventCondition::IS_HUMAN: {
							ui->onlyForHumansCheck->setChecked(true);
							break;
						}
					};
				}
			}
		}
	}
}

void VictoryConditions::update(CMap & map)
{
	//victory messages
	map.victoryMessage = MetaString::createFromRawString(ui->victoryMessageEdit->text().toStdString());

	//victory conditions
	EventCondition victoryCondition(EventCondition::STANDARD_WIN);

	//Victory condition - defeat all
	TriggeredEvent standardVictory;
	standardVictory.effect.type = EventEffect::VICTORY;
	standardVictory.effect.toOtherMessage.appendTextID("core.genrltxt.5");
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill.appendTextID("core.genrltxt.659");
	standardVictory.trigger = EventExpression(victoryCondition);

	//VICTORY
	if(ui->victoryComboBox->currentIndex() == 0)
	{
		map.triggeredEvents.push_back(standardVictory);
		map.victoryIconIndex = 11;
		map.victoryMessage.appendTextID(VLC->generaltexth->victoryConditions[0]);
	}
	else
	{
		int vicCondition = ui->victoryComboBox->currentIndex() - 1;

		TriggeredEvent specialVictory;
		specialVictory.effect.type = EventEffect::VICTORY;
		specialVictory.identifier = "specialVictory";
		specialVictory.description.clear(); // TODO: display in quest window

		map.victoryIconIndex = vicCondition;
		map.victoryMessage.appendTextID(VLC->generaltexth->victoryConditions[size_t(vicCondition) + 1]);

		switch(vicCondition)
		{
			case 0: {
				EventCondition cond(EventCondition::HAVE_ARTIFACT);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.281");
				specialVictory.onFulfill.appendTextID("core.genrltxt.280");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 1: {
				EventCondition cond(EventCondition::HAVE_CREATURES);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				cond.value = victoryValueWidget->text().toInt();
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.277");
				specialVictory.onFulfill.appendTextID("core.genrltxt.276");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 2: {
				EventCondition cond(EventCondition::HAVE_RESOURCES);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				cond.value = victoryValueWidget->text().toInt();
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.279");
				specialVictory.onFulfill.appendTextID("core.genrltxt.278");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 3: {
				EventCondition cond(EventCondition::HAVE_BUILDING);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				int townIdx = victorySelectWidget->currentData().toInt();
				if(townIdx > -1)
					cond.position = map.objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.283");
				specialVictory.onFulfill.appendTextID("core.genrltxt.282");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 4: {
				EventCondition cond(EventCondition::CONTROL);
				assert(victoryTypeWidget);
				cond.objectType = Obj::TOWN;
				int townIdx = victoryTypeWidget->currentData().toInt();
				cond.position = map.objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.250");
				specialVictory.onFulfill.appendTextID("core.genrltxt.249");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 5: {
				EventCondition cond(EventCondition::DESTROY);
				assert(victoryTypeWidget);
				cond.objectType = Obj::HERO;
				int heroIdx = victoryTypeWidget->currentData().toInt();
				cond.position = map.objects[heroIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.253");
				specialVictory.onFulfill.appendTextID("core.genrltxt.252");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 6: {
				EventCondition cond(EventCondition::TRANSPORT);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				int townIdx = victorySelectWidget->currentData().toInt();
				if(townIdx > -1)
					cond.position = map.objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.293");
				specialVictory.onFulfill.appendTextID("core.genrltxt.292");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

		}

		// if condition is human-only turn it into following construction: AllOf(human, condition)
		if(ui->onlyForHumansCheck->isChecked())
		{
			EventExpression::OperatorAll oper;
			EventCondition notAI(EventCondition::IS_HUMAN);
			notAI.value = 1;
			oper.expressions.push_back(notAI);
			oper.expressions.push_back(specialVictory.trigger.get());
			specialVictory.trigger = EventExpression(oper);
		}

		// if normal victory allowed - add one more quest
		if(ui->standardVictoryCheck->isChecked())
		{
			map.victoryMessage.appendRawString(" / ");
			map.victoryMessage.appendTextID(VLC->generaltexth->victoryConditions[0]);
			map.triggeredEvents.push_back(standardVictory);
		}
		map.triggeredEvents.push_back(specialVictory);
	}
}

VictoryConditions::~VictoryConditions()
{
	delete ui;
}

void VictoryConditions::on_victoryComboBox_currentIndexChanged(int index)
{
	delete victoryTypeWidget;
	delete victoryValueWidget;
	delete victorySelectWidget;
	victoryTypeWidget = nullptr;
	victoryValueWidget = nullptr;
	victorySelectWidget = nullptr;

	if(index == 0)
	{
		ui->standardVictoryCheck->setChecked(true);
		ui->standardVictoryCheck->setEnabled(false);
		ui->onlyForHumansCheck->setChecked(false);
		ui->onlyForHumansCheck->setEnabled(false);
		return;
	}
	ui->onlyForHumansCheck->setEnabled(true);
	ui->standardVictoryCheck->setEnabled(true);

	int vicCondition = index - 1;
	switch(vicCondition)
	{
		case 0: { //EventCondition::HAVE_ARTIFACT
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < mapPointer->allowedArtifact.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(VLC->arth->objects[i]->getNameTranslated()), QVariant::fromValue(i));
			break;
		}

		case 1: { //EventCondition::HAVE_CREATURES
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < VLC->creh->objects.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(VLC->creh->objects[i]->getNamePluralTranslated()), QVariant::fromValue(i));

			victoryValueWidget = new QLineEdit;
			ui->victoryParamsLayout->addWidget(victoryValueWidget);
			victoryValueWidget->setText("1");
			break;
		}

		case 2: { //EventCondition::HAVE_RESOURCES
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			{
				for(int resType = 0; resType < GameConstants::RESOURCE_QUANTITY; ++resType)
				{
					auto resName = QString::fromStdString(GameConstants::RESOURCE_NAMES[resType]);
					victoryTypeWidget->addItem(resName, QVariant::fromValue(resType));
				}
			}

			victoryValueWidget = new QLineEdit;
			ui->victoryParamsLayout->addWidget(victoryValueWidget);
			victoryValueWidget->setText("1");
			break;
		}

		case 3: { //EventCondition::HAVE_BUILDING
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			auto * ctown = VLC->townh->randomTown;
			for(int bId : ctown->getAllBuildings())
				victoryTypeWidget->addItem(QString::fromStdString(defaultBuildingIdConversion(BuildingID(bId))), QVariant::fromValue(bId));

			victorySelectWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victorySelectWidget);
			victorySelectWidget->addItem("Any town", QVariant::fromValue(-1));
			for(int i : getObjectIndexes<const CGTownInstance>(*mapPointer))
				victorySelectWidget->addItem(getTownName(*mapPointer, i).c_str(), QVariant::fromValue(i));
			break;
		}

		case 4: { //EventCondition::CONTROL (Obj::TOWN)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<const CGTownInstance>(*mapPointer))
				victoryTypeWidget->addItem(tr(getTownName(*mapPointer, i).c_str()), QVariant::fromValue(i));
			break;
		}

		case 5: { //EventCondition::DESTROY (Obj::HERO)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<const CGHeroInstance>(*mapPointer))
				victoryTypeWidget->addItem(tr(getHeroName(*mapPointer, i).c_str()), QVariant::fromValue(i));
			break;
		}

		case 6: { //EventCondition::TRANSPORT (Obj::ARTEFACT)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < mapPointer->allowedArtifact.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(VLC->arth->objects[i]->getNameTranslated()), QVariant::fromValue(i));

			victorySelectWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victorySelectWidget);
			for(int i : getObjectIndexes<const CGTownInstance>(*mapPointer))
				victorySelectWidget->addItem(tr(getTownName(*mapPointer, i).c_str()), QVariant::fromValue(i));
			break;
		}


		//TODO: support this vectory type
		// in order to do that, need to implement finding creature by position
		// selecting from map would be the best user experience
		/*case 7: { //EventCondition::DESTROY (Obj::MONSTER)
			victoryTypeWidget = new QComboBox;
			ui->loseParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<const CGCreature>(*mapPointer))
				victoryTypeWidget->addItem(tr(getMonsterName(i).c_str()), QVariant::fromValue(i));
			break;
		}*/


	}
}

