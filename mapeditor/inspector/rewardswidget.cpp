/*
 * rewardswidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../helper.h"
#include "rewardswidget.h"
#include "ui_rewardswidget.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/CBonusTypeHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/ResourceTypeHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/rewardable/Configuration.h"
#include "../../lib/rewardable/Limiter.h"
#include "../../lib/rewardable/Reward.h"
#include "../../lib/mapObjects/CGPandoraBox.h"
#include "../../lib/mapObjects/Quest.h"

#include <vcmi/ArtifactService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/HeroType.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroClass.h>
#include <vcmi/spells/Service.h>
#include <vcmi/spells/Spell.h>
#include "../translator.h"

RewardsWidget::RewardsWidget(CMap & m, CRewardableObject & p, QWidget *parent) :
	QDialog(parent),
	map(m),
	object(p),
	ui(new Ui::RewardsWidget)
{
	ui->setupUi(this);
	Helper::decorateDialog(this);
	rewardWidget = std::make_unique<RewardWidget>(map, object.id , this);
	ui->rewardLayout->addWidget(rewardWidget.get());
	//fill core elements
	for(const auto & s : Rewardable::VisitModeString)
		ui->visitMode->addItem(QString::fromUtf8(s.data(), s.size()));
	
	for(const auto & s : Rewardable::SelectModeString)
		ui->selectMode->addItem(QString::fromUtf8(s.data(), s.size()));
	
	for(auto s : {"AUTO", "MODAL", "INFO"})
		ui->windowMode->addItem(QString::fromStdString(s));
	
	ui->lDayOfWeek->addItem(tr("None"));
	for(int i = 1; i <= 7; ++i)
		ui->lDayOfWeek->addItem(tr("Day %1").arg(i));
	
	//fill resources
	ui->lResources->setRowCount(LIBRARY->resourceTypeHandler->getAllObjects().size());
	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		MetaString str;
		str.appendName(GameResID(i));
			auto * item = new QTableWidgetItem(QString::fromStdString(str.toString(&Translator::instance())));
			item->setData(Qt::UserRole, QVariant::fromValue(i.getNum()));
			ui->lResources->setItem(i, 0, item);
			auto * spinBox = new QSpinBox;
			spinBox->setMaximum(i == GameResID::GOLD ? 999999 : 999);
			ui->lResources->setCellWidget(i, 1, spinBox);
	}
	
	//fill artifacts
	for(int i = 0; i < map.allowedArtifact.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(LIBRARY->artifacts()->getByIndex(i)->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		if(map.allowedArtifact.count(i) == 0)
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		ui->lArtifacts->addItem(item);
	}
	
	//fill spells
	for(int i = 0; i < map.allowedSpells.size(); ++i)
	{

			auto * item = new QListWidgetItem(QString::fromStdString(LIBRARY->spells()->getByIndex(i)->getNameTranslated()));
			item->setData(Qt::UserRole, QVariant::fromValue(i));
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
			if(map.allowedSpells.count(i) == 0)
				item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
			ui->lSpells->addItem(item);
	}
	
	//fill skills
	ui->lSkills->setRowCount(map.allowedAbilities.size());
	for(int i = 0; i < map.allowedAbilities.size(); ++i)
	{

		auto * item = new QTableWidgetItem(QString::fromStdString(LIBRARY->skills()->getByIndex(i)->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));

		auto * widget = new QComboBox;
		for(auto & s : NSecondarySkill::levels)
			widget->addItem(QString::fromUtf8(s));

		if(map.allowedAbilities.count(i) == 0)
		{
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
			widget->setEnabled(false);
		}

		ui->lSkills->setItem(i, 0, item);
		ui->lSkills->setCellWidget(i, 1, widget);
	}
	
	//fill creatures
	for(auto & creature : LIBRARY->creh->objects)
	{
		ui->lCreatureId->addItem(QString::fromStdString(creature->getNameSingularTranslated()));
		ui->lCreatureId->setItemData(ui->lCreatureId->count() - 1, creature->getIndex());
	}
	
	//fill heroes
	LIBRARY->heroTypes()->forEach([this](const HeroType * hero, bool &)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(hero->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(hero->getId().getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lHeroes->addItem(item);
	});
	
	//fill hero classes
	LIBRARY->heroClasses()->forEach([this](const HeroClass * heroClass, bool &)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(heroClass->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(heroClass->getId().getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lHeroClasses->addItem(item);
	});
	
	//fill players
	for(auto color = PlayerColor(0); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		MetaString str;
		str.appendName(color);
		auto * item = new QListWidgetItem(QString::fromStdString(str.toString(&Translator::instance())));
		item->setData(Qt::UserRole, QVariant::fromValue(color.getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lPlayers->addItem(item);
	}
	
	//set default values
	if(dynamic_cast<CGPandoraBox*>(&object))
	{
		ui->visitMode->setCurrentIndex(vstd::find_pos(Rewardable::VisitModeString, "once"));
		ui->visitMode->setEnabled(false);
		ui->selectMode->setCurrentIndex(vstd::find_pos(Rewardable::SelectModeString, "selectFirst"));
		ui->selectMode->setEnabled(false);
		ui->windowMode->setEnabled(false);
		ui->canRefuse->setEnabled(false);
	}
	
	if(auto * e = dynamic_cast<CGEvent*>(&object))
	{
		ui->selectMode->setEnabled(true);
		if(!e->removeAfterVisit)
			ui->visitMode->setCurrentIndex(vstd::find_pos(Rewardable::VisitModeString, "unlimited"));
	}
	
	if(dynamic_cast<SeerHut*>(&object))
	{
		ui->visitMode->setCurrentIndex(vstd::find_pos(Rewardable::VisitModeString, "once"));
		ui->visitMode->setEnabled(false);
		ui->windowMode->setEnabled(false);
		ui->canRefuse->setChecked(true);
		ui->canRefuse->setEnabled(false);
	}
	
	//hide elements
	ui->eventInfoGroup->hide();
}

RewardsWidget::~RewardsWidget()
{
	delete ui;
}


void RewardsWidget::obtainData()
{
	//common parameters
	ui->visitMode->setCurrentIndex(object.configuration.visitMode);
	ui->selectMode->setCurrentIndex(object.configuration.selectMode);
	ui->windowMode->setCurrentIndex(int(object.configuration.infoWindowType));
	ui->onSelectText->setText(QString::fromStdString(object.configuration.onSelect.toString(&Translator::instance())));
	ui->canRefuse->setChecked(object.configuration.canRefuse);
	
	//reset parameters
	ui->resetPeriod->setValue(object.configuration.resetParameters.days);
	ui->resetWeeks->setValue(object.configuration.resetParameters.weeks);
	ui->resetMonths->setValue(object.configuration.resetParameters.months);
	ui->resetVisitors->setChecked(object.configuration.resetParameters.visitors);
	ui->resetRewards->setChecked(object.configuration.resetParameters.rewards);
	
	ui->visitInfoList->clear();
	
	for([[maybe_unused]] auto & a : object.configuration.info)
		ui->visitInfoList->addItem(tr("Reward %1").arg(ui->visitInfoList->count() + 1));
	
	if(ui->visitInfoList->currentItem())
		loadCurrentVisitInfo(ui->visitInfoList->currentRow());
}

bool RewardsWidget::commitChanges()
{
	//common parameters
	object.configuration.visitMode = static_cast<Rewardable::EVisitMode>(ui->visitMode->currentIndex());
	object.configuration.selectMode = static_cast<Rewardable::ESelectMode>(ui->selectMode->currentIndex());
	object.configuration.infoWindowType = EInfoWindowMode(ui->windowMode->currentIndex());
	if(ui->onSelectText->text().isEmpty())
		object.configuration.onSelect.clear();
	else
		object.configuration.onSelect = MetaString::createFromTextID(mapRegisterLocalizedString("map", map, TextIdentifier("reward", object.instanceName, "onSelect"), ui->onSelectText->text().toStdString()));
	object.configuration.canRefuse = ui->canRefuse->isChecked();
	
	//reset parameters
	object.configuration.resetParameters.days = ui->resetPeriod->value();
	object.configuration.resetParameters.weeks = ui->resetWeeks->value();
	object.configuration.resetParameters.months = ui->resetMonths->value();
	object.configuration.resetParameters.visitors = ui->resetVisitors->isChecked();
	object.configuration.resetParameters.rewards = ui->resetRewards->isChecked();
	
	if(ui->visitInfoList->currentItem())
		saveCurrentVisitInfo(ui->visitInfoList->currentRow());
	
	return true;
}

void RewardsWidget::saveCurrentVisitInfo(int index)
{
	rewardWidget->commit();
	auto & vinfo = object.configuration.info.at(index);
	vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	if(ui->rewardMessage->text().isEmpty())
		vinfo.message.clear();
	else
		vinfo.message = MetaString::createFromTextID(mapRegisterLocalizedString("map", map, TextIdentifier("reward", object.instanceName, "info", index, "message"), ui->rewardMessage->text().toStdString()));
	
	vinfo.limiter.dayOfWeek = ui->lDayOfWeek->currentIndex();
	vinfo.limiter.daysPassed = ui->lDaysPassed->value();
	vinfo.limiter.heroLevel = ui->lHeroLevel->value();
	vinfo.limiter.heroExperience = ui->lHeroExperience->value();
	vinfo.limiter.manaPoints = ui->lManaPoints->value();
	vinfo.limiter.manaPercentage = ui->lManaPercentage->value();
	vinfo.limiter.primary.resize(4);
	vinfo.limiter.primary[0] = ui->lAttack->value();
	vinfo.limiter.primary[1] = ui->lDefence->value();
	vinfo.limiter.primary[2] = ui->lPower->value();
	vinfo.limiter.primary[3] = ui->lKnowledge->value();
	for(int i = 0; i < ui->lResources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lResources->cellWidget(i, 1)))
			vinfo.limiter.resources[i] = widget->value();
	}
	
	vinfo.limiter.artifacts.clear();
	for(int i = 0; i < ui->lArtifacts->count(); ++i)
	{
		if(ui->lArtifacts->item(i)->checkState() == Qt::Checked)
			vinfo.limiter.artifacts.push_back(LIBRARY->artifacts()->getByIndex(i)->getId());
	}
	vinfo.limiter.spells.clear();
	for(int i = 0; i < ui->lSpells->count(); ++i)
	{
		if(ui->lSpells->item(i)->checkState() == Qt::Checked)
			vinfo.limiter.spells.push_back(LIBRARY->spells()->getByIndex(i)->getId());
	}
	
	vinfo.limiter.secondary.clear();
	for(int i = 0; i < ui->lSkills->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QComboBox*>(ui->lSkills->cellWidget(i, 1)))
		{
			if(widget->currentIndex() > 0)
				vinfo.limiter.secondary[LIBRARY->skills()->getByIndex(i)->getId()] = widget->currentIndex();
		}
	}
	
	vinfo.limiter.creatures.clear();
	for(int i = 0; i < ui->lCreatures->rowCount(); ++i)
	{
		int index = ui->lCreatures->item(i, 0)->data(Qt::UserRole).toInt();
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lCreatures->cellWidget(i, 1)))
			if(widget->value())
				vinfo.limiter.creatures.emplace_back(LIBRARY->creatures()->getByIndex(index)->getId(), widget->value());
	}
	
	vinfo.limiter.heroes.clear();
	for(int i = 0; i < ui->lHeroes->count(); ++i)
	{
		if(ui->lHeroes->item(i)->checkState() == Qt::Checked)
			vinfo.limiter.heroes.emplace_back(ui->lHeroes->item(i)->data(Qt::UserRole).toInt());
	}
	
	vinfo.limiter.heroClasses.clear();
	for(int i = 0; i < ui->lHeroClasses->count(); ++i)
	{
		if(ui->lHeroClasses->item(i)->checkState() == Qt::Checked)
			vinfo.limiter.heroClasses.emplace_back(ui->lHeroClasses->item(i)->data(Qt::UserRole).toInt());
	}
	
	vinfo.limiter.players.clear();
	for(int i = 0; i < ui->lPlayers->count(); ++i)
	{
		if(ui->lPlayers->item(i)->checkState() == Qt::Checked)
			vinfo.limiter.players.emplace_back(ui->lPlayers->item(i)->data(Qt::UserRole).toInt());
	}
}

void RewardsWidget::loadCurrentVisitInfo(int index)
{
	for(auto * w : {ui->lArtifacts, ui->lSpells})
		for(int i = 0; i < w->count(); ++i)
			w->item(i)->setCheckState(Qt::Unchecked);

	ui->lCreatures->setRowCount(0);
	
	auto & vinfo = object.configuration.info.at(index);
	rewardWidget->loadReward(&vinfo.reward);
	ui->rewardMessage->setText(QString::fromStdString(vinfo.message.toString(&Translator::instance())));
	
	ui->lDayOfWeek->setCurrentIndex(vinfo.limiter.dayOfWeek);
	ui->lDaysPassed->setValue(vinfo.limiter.daysPassed);
	ui->lHeroLevel->setValue(vinfo.limiter.heroLevel);
	ui->lHeroExperience->setValue(vinfo.limiter.heroExperience);
	ui->lManaPoints->setValue(vinfo.limiter.manaPoints);
	ui->lManaPercentage->setValue(vinfo.limiter.manaPercentage);
	ui->lAttack->setValue(vinfo.limiter.primary[0]);
	ui->lDefence->setValue(vinfo.limiter.primary[1]);
	ui->lPower->setValue(vinfo.limiter.primary[2]);
	ui->lKnowledge->setValue(vinfo.limiter.primary[3]);
	for(int i = 0; i < ui->lResources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lResources->cellWidget(i, 1)))
			widget->setValue(vinfo.limiter.resources[i]);
	}
	
	for(auto i : vinfo.limiter.artifacts)
		ui->lArtifacts->item(LIBRARY->artifacts()->getById(i)->getIndex())->setCheckState(Qt::Checked);
	for(auto i : vinfo.limiter.spells)
		ui->lArtifacts->item(LIBRARY->spells()->getById(i)->getIndex())->setCheckState(Qt::Checked);
	for(auto & i : vinfo.limiter.secondary)
	{
		int index = LIBRARY->skills()->getById(i.first)->getIndex();
		if(auto * widget = qobject_cast<QComboBox*>(ui->lSkills->cellWidget(index, 1)))
			widget->setCurrentIndex(i.second);
	}
	for(auto & i : vinfo.limiter.creatures)
	{
		int index = i.getType()->getIndex();
		ui->lCreatureId->setCurrentIndex(index);
		ui->lCreatureAmount->setValue(i.getCount());
		onCreatureAdd(ui->lCreatures, ui->lCreatureId, ui->lCreatureAmount);
	}
	
	for(auto & i : vinfo.limiter.heroes)
	{
		for(int e = 0; e < ui->lHeroes->count(); ++e)
		{
			if(ui->lHeroes->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->lHeroes->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}
	for(auto & i : vinfo.limiter.heroClasses)
	{
		for(int e = 0; e < ui->lHeroClasses->count(); ++e)
		{
			if(ui->lHeroClasses->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->lHeroClasses->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}
	for(auto & i : vinfo.limiter.players)
	{
		for(int e = 0; e < ui->lPlayers->count(); ++e)
		{
			if(ui->lPlayers->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->lPlayers->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}
}

void RewardsWidget::onCreatureAdd(QTableWidget * listWidget, QComboBox * comboWidget, QSpinBox * spinWidget)
{
	QTableWidgetItem * item = nullptr;
	QSpinBox * widget = nullptr;
	for(int i = 0; i < listWidget->rowCount(); ++i)
	{
		if(auto * cname = listWidget->item(i, 0))
		{
			if(cname->data(Qt::UserRole).toInt() == comboWidget->currentData().toInt())
			{
				item = cname;
				widget = qobject_cast<QSpinBox*>(listWidget->cellWidget(i, 1));
				break;
			}
		}
	}
	
	if(!item)
	{
		listWidget->setRowCount(listWidget->rowCount() + 1);
		item = new QTableWidgetItem(comboWidget->currentText());
		listWidget->setItem(listWidget->rowCount() - 1, 0, item);
	}
	
	item->setData(Qt::UserRole, comboWidget->currentData());
	
	if(!widget)
	{
		widget = new QSpinBox;
		widget->setRange(spinWidget->minimum(), spinWidget->maximum());
		listWidget->setCellWidget(listWidget->rowCount() - 1, 1, widget);
	}
	
	widget->setValue(spinWidget->value());
}

void RewardsWidget::on_addVisitInfo_clicked()
{
	ui->visitInfoList->addItem(tr("Reward %1").arg(ui->visitInfoList->count() + 1));
	object.configuration.info.emplace_back();
}


void RewardsWidget::on_removeVisitInfo_clicked()
{
	int index = ui->visitInfoList->currentRow();
	object.configuration.info.erase(std::next(object.configuration.info.begin(), index));
	ui->visitInfoList->blockSignals(true);
	delete ui->visitInfoList->currentItem();
	ui->visitInfoList->blockSignals(false);
	on_visitInfoList_itemSelectionChanged();
	if(ui->visitInfoList->currentItem())
		loadCurrentVisitInfo(ui->visitInfoList->currentRow());
}

void RewardsWidget::on_selectMode_currentIndexChanged(int index)
{
	ui->onSelectText->setEnabled(index == vstd::find_pos(Rewardable::SelectModeString, "selectPlayer"));
}

void RewardsWidget::on_resetPeriod_valueChanged(int arg1)
{
	bool anyPeriodSet = arg1 > 0 || ui->resetWeeks->value() > 0 || ui->resetMonths->value() > 0;
	ui->resetRewards->setEnabled(anyPeriodSet);
	ui->resetVisitors->setEnabled(anyPeriodSet);
}

void RewardsWidget::on_resetWeeks_valueChanged(int arg1)
{
	bool anyPeriodSet = arg1 > 0 || ui->resetPeriod->value() > 0 || ui->resetMonths->value() > 0;
	ui->resetRewards->setEnabled(anyPeriodSet);
	ui->resetVisitors->setEnabled(anyPeriodSet);
}

void RewardsWidget::on_resetMonths_valueChanged(int arg1)
{
	bool anyPeriodSet = arg1 > 0 || ui->resetPeriod->value() > 0 || ui->resetWeeks->value() > 0;
	ui->resetRewards->setEnabled(anyPeriodSet);
	ui->resetVisitors->setEnabled(anyPeriodSet);
}


void RewardsWidget::on_visitInfoList_itemSelectionChanged()
{
	if(ui->visitInfoList->currentItem() == nullptr)
	{
		ui->eventInfoGroup->hide();
		ui->removeVisitInfo->setEnabled(false);
		return;
	}
	
	ui->eventInfoGroup->show();
	ui->removeVisitInfo->setEnabled(true);
}

void RewardsWidget::on_visitInfoList_currentItemChanged(QListWidgetItem * current, QListWidgetItem * previous)
{
	if(previous)
		saveCurrentVisitInfo(ui->visitInfoList->row(previous));
	
	if(current)
		loadCurrentVisitInfo(ui->visitInfoList->currentRow());
}


void RewardsWidget::on_lCreatureAdd_clicked()
{
	onCreatureAdd(ui->lCreatures, ui->lCreatureId, ui->lCreatureAmount);
}


void RewardsWidget::on_lCreatureRemove_clicked()
{
	std::set<int, std::greater<int>> rowsToRemove;
	for(auto * i : ui->lCreatures->selectedItems())
		rowsToRemove.insert(i->row());
	
	for(auto i : rowsToRemove)
		ui->lCreatures->removeRow(i);
}


void RewardsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<RewardsWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void RewardsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<RewardsWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

RewardsDelegate::RewardsDelegate(CMap & m, CRewardableObject & t): map(m), object(t)
{
}

QWidget * RewardsDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new RewardsWidget(map, object, parent);
}

void RewardsDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList(QObject::tr("Rewards:"));
	for (const auto & vinfo : object.configuration.info)
	{
		textList += QObject::tr("Reward Message: %1").arg(QString::fromStdString(vinfo.message.toString(&Translator::instance())));
		textList += QObject::tr("Hero Level: %1").arg(vinfo.reward.heroLevel);
		textList += QObject::tr("Hero Experience: %1").arg(vinfo.reward.heroExperience);
		textList += QObject::tr("Mana Diff: %1").arg(vinfo.reward.manaDiff);
		textList += QObject::tr("Mana Percentage: %1").arg(vinfo.reward.manaPercentage);
		textList += QObject::tr("Move Points: %1").arg(vinfo.reward.movePoints);
		textList += QObject::tr("Move Percentage: %1").arg(vinfo.reward.movePercentage);
		textList += QObject::tr("Primary Skills: %1/%2/%3/%4").arg(vinfo.reward.primary[0]).arg(vinfo.reward.primary[1]).arg(vinfo.reward.primary[2]).arg(vinfo.reward.primary[3]);
		QStringList resourcesList;
		for(GameResID resource = GameResID::WOOD; resource < GameResID::COUNT ; resource++)
		{
			if(vinfo.reward.resources[resource] == 0)
				continue;
			MetaString str;
			str.appendName(resource);
			resourcesList += QString("%1: %2").arg(QString::fromStdString(str.toString(&Translator::instance()))).arg(vinfo.reward.resources[resource]);
		}
		textList += QObject::tr("Resources: %1").arg(resourcesList.join(", "));
		QStringList artifactsList;
		for (auto artifact : vinfo.reward.grantedArtifacts)
		{
			artifactsList += QString::fromStdString(LIBRARY->artifacts()->getById(artifact)->getNameTranslated());
		}
		textList += QObject::tr("Artifacts: %1").arg(artifactsList.join(", "));
		QStringList spellsList;
		for (auto spell : vinfo.reward.spells)
		{
			spellsList += QString::fromStdString(LIBRARY->spells()->getById(spell)->getNameTranslated());
		}
		textList += QObject::tr("Spells: %1").arg(spellsList.join(", "));
		QStringList secondarySkillsList;
		for(auto & [skill, skillLevel] : vinfo.reward.secondary)
		{
			secondarySkillsList += QString("%1 (%2)").arg(QString::fromStdString(LIBRARY->skills()->getById(skill)->getNameTranslated())).arg(skillLevel);
		}
		textList += QObject::tr("Secondary Skills: %1").arg(secondarySkillsList.join(", "));
		QStringList creaturesList;
		for (auto & creature : vinfo.reward.creatures)
		{
			creaturesList += QString("%1 %2").arg(creature.getCount()).arg(QString::fromStdString(creature.getType()->getNamePluralTranslated()));
		}
		textList += QObject::tr("Creatures: %1").arg(creaturesList.join(", "));
		if (vinfo.reward.spellCast.first != SpellID::NONE)
		{
			textList += QObject::tr("Spell Cast: %1 (%2)").arg(QString::fromStdString(LIBRARY->spells()->getById(vinfo.reward.spellCast.first)->getNameTranslated())).arg(vinfo.reward.spellCast.second);
		}
		QStringList bonusesList;
		for (auto & bonus : vinfo.reward.heroBonuses)
		{
			std::string bonusName = LIBRARY->bth->bonusToString(bonus->type);
			bonusesList += QString("%1 %2 (%3)").arg(QString::fromStdString(vstd::findKey(bonusDurationMap, bonus->duration))).arg(QString::fromStdString(bonusName)).arg(bonus->val);
		}
		textList += QObject::tr("Bonuses: %1").arg(bonusesList.join(", "));
	}
	setModelTextData(model, index, textList);
}
