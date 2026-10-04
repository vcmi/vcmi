#include "rewardwidget.h"
#include "StdInc.h"
#include "ui_rewardwidget.h"
#include "lib/GameLibrary.h"
#include "lib/CSkillHandler.h"
#include "lib/CBonusTypeHandler.h"
#include "lib/CCreatureHandler.h"
#include "lib/constants/StringConstants.h"
#include "lib/entities/artifact/CArtifact.h"
#include "lib/entities/ResourceTypeHandler.h"
#include "lib/mapping/CMap.h"
#include "lib/modding/IdentifierStorage.h"
#include "lib/modding/ModScope.h"
#include "lib/rewardable/Configuration.h"
#include "lib/mapObjects/CGPandoraBox.h"
#include "lib/mapObjects/Quest.h"
#include "translator.h"

#include <vcmi/ArtifactService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/HeroType.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroClass.h>
#include <vcmi/spells/Service.h>
#include <vcmi/spells/Spell.h>

RewardWidget::RewardWidget(CMap & m, ObjectInstanceID id, QWidget *parent) :
	QWidget(parent),
	map(m),
	id(id),
	ui(new Ui::RewardWidget)
{
	ui->setupUi(this);
	ui->resources->setRowCount(LIBRARY->resourceTypeHandler->getAllObjects().size());
	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		MetaString str;
		str.appendName(GameResID(i));
		for(auto * w : {ui->resources})
		{
			auto * item = new QTableWidgetItem(QString::fromStdString(str.toString(&Translator::instance())));
			item->setData(Qt::UserRole, QVariant::fromValue(i.getNum()));
			w->setItem(i, 0, item);
			auto * spinBox = new QSpinBox;
			spinBox->setMaximum(i == GameResID::GOLD ? 999999 : 999);
			if(w == ui->resources)
				spinBox->setMinimum(i == GameResID::GOLD ? -999999 : -999);
			w->setCellWidget(i, 1, spinBox);
		}
	}

	for(int i = 0; i < map.allowedArtifact.size(); ++i)
	{
		for(auto * w : {ui->artifacts})
		{
			auto * item = new QListWidgetItem(QString::fromStdString(LIBRARY->artifacts()->getByIndex(i)->getNameTranslated()));
			item->setData(Qt::UserRole, QVariant::fromValue(i));
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
			if(map.allowedArtifact.count(i) == 0)
				item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
			w->addItem(item);
		}
	}

	for(int i = 0; i < map.allowedSpells.size(); ++i)
	{
		for(auto * w : {ui->spells})
		{
			auto * item = new QListWidgetItem(QString::fromStdString(LIBRARY->spells()->getByIndex(i)->getNameTranslated()));
			item->setData(Qt::UserRole, QVariant::fromValue(i));
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
			if(map.allowedSpells.count(i) == 0)
				item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
			w->addItem(item);
		}


		if(LIBRARY->spells()->getByIndex(i)->isAdventure())
		{
			ui->castSpell->addItem(QString::fromStdString(LIBRARY->spells()->getByIndex(i)->getNameTranslated()));
			ui->castSpell->setItemData(ui->castSpell->count() - 1, QVariant::fromValue(i));
		}
	}


	ui->skills->setRowCount(map.allowedAbilities.size());
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

		ui->skills->setItem(i, 0, item);
		ui->skills->setCellWidget(i, 1, widget);

	}


	for(auto & creature : LIBRARY->creh->objects)
	{

		ui->creatureId->addItem(QString::fromStdString(creature->getNameSingularTranslated()));
		ui->creatureId->setItemData(ui->creatureId->count() - 1, creature->getIndex());

	}

	//fill spell cast
	for(auto & s : NSecondarySkill::levels)
		ui->castLevel->addItem(QString::fromUtf8(s));

	//fill bonuses
	for(auto & s : bonusDurationMap)
		ui->bonusDuration->addItem(QString::fromStdString(s.first));
	for(auto & s : LIBRARY->bth->getAllObjets())
		ui->bonusType->addItem(QString::fromStdString(LIBRARY->bth->bonusToString(s)));

}

RewardWidget::~RewardWidget()
{
	delete ui;
}

void RewardWidget::loadReward(Rewardable::Reward * reward)
{
	this->reward = reward;
	showData(this->reward);
}

void RewardWidget::showData(Rewardable::Reward * reward)
{
	if (ui->removeObject)
	{
		ui->removeObject->setChecked(reward->removeObject);
	}

	for(auto * w : {ui->artifacts, ui->spells})
		for(int i = 0; i < w->count(); ++i)
			w->item(i)->setCheckState(Qt::Unchecked);

	for(auto * w : {ui->skills})
		for(int i = 0; i < w->rowCount(); ++i)
			if(auto * widget = qobject_cast<QComboBox*>(ui->skills->cellWidget(i, 1)))
				widget->setCurrentIndex(0);

	ui->creatures->setRowCount(0);
	ui->bonuses->setRowCount(0);

	ui->heroLevel->setValue(reward->heroLevel);
	ui->heroExperience->setValue(reward->heroExperience);
	ui->manaDiff->setValue(reward->manaDiff);
	ui->manaPercentage->setValue(reward->manaPercentage);
	ui->overflowFactor->setValue(reward->manaOverflowFactor);
	ui->movePoints->setValue(reward->movePoints);
	ui->movePercentage->setValue(reward->movePercentage);
	ui->attack->setValue(reward->primary[0]);
	ui->defence->setValue(reward->primary[1]);
	ui->power->setValue(reward->primary[2]);
	ui->knowledge->setValue(reward->primary[3]);
	for(int i = 0; i < ui->resources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->resources->cellWidget(i, 1)))
			widget->setValue(reward->resources[i]);
	}

	for(auto i : reward->grantedArtifacts)
		ui->artifacts->item(LIBRARY->artifacts()->getById(i)->getIndex())->setCheckState(Qt::Checked);
	for(auto i : reward->spells)
		ui->spells->item(LIBRARY->spells()->getById(i)->getIndex())->setCheckState(Qt::Checked);
	for(auto & i : reward->secondary)
	{
		int index = LIBRARY->skills()->getById(i.first)->getIndex();
		if(auto * widget = qobject_cast<QComboBox*>(ui->skills->cellWidget(index, 1)))
			widget->setCurrentIndex(i.second);
	}
	for(auto & i : reward->creatures)
	{
		int index = i.getType()->getIndex();
		ui->creatureId->setCurrentIndex(index);
		ui->creatureAmount->setValue(i.getCount());
		onCreatureAdd(ui->creatures, ui->creatureId, ui->creatureAmount);
	}

	ui->castSpellCheck->setChecked(reward->spellCast.first != SpellID::NONE);
	if(ui->castSpellCheck->isChecked())
	{
		int index = LIBRARY->spells()->getById(reward->spellCast.first)->getIndex();
		ui->castSpell->setCurrentIndex(index);
		ui->castLevel->setCurrentIndex(reward->spellCast.second);
	}

	for(auto & i : reward->heroBonuses)
	{
		auto dur = vstd::findKey(bonusDurationMap, i->duration);
		for(int i = 0; i < ui->bonusDuration->count(); ++i)
		{
			if(ui->bonusDuration->itemText(i) == QString::fromStdString(dur))
			{
				ui->bonusDuration->setCurrentIndex(i);
				break;
			}
		}

		std::string typ = LIBRARY->bth->bonusToString(i->type);
		for(int i = 0; i < ui->bonusType->count(); ++i)
		{
			if(ui->bonusType->itemText(i) == QString::fromStdString(typ))
			{
				ui->bonusType->setCurrentIndex(i);
				break;
			}
		}

		ui->bonusValue->setValue(i->val);
		on_bonusAdd_clicked();
	}
	printRewardInformation();
}

void RewardWidget::commit()
{
	if (!reward)
		return;

	if (ui->removeObject)
		reward->removeObject = ui->removeObject->isChecked();
	else
		reward->removeObject = false;

	reward->heroLevel = ui->heroLevel->value();
	reward->heroExperience = ui->heroExperience->value();
	reward->manaDiff = ui->manaDiff->value();
	reward->manaPercentage = ui->manaPercentage->value();
	reward->manaOverflowFactor = ui->overflowFactor->value();
	reward->movePoints = ui->movePoints->value();
	reward->movePercentage = ui->movePercentage->value();
	reward->primary.resize(4);
	reward->primary[0] = ui->attack->value();
	reward->primary[1] = ui->defence->value();
	reward->primary[2] = ui->power->value();
	reward->primary[3] = ui->knowledge->value();
	for(int i = 0; i < ui->resources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->resources->cellWidget(i, 1)))
			reward->resources[i] = widget->value();
	}

	reward->grantedArtifacts.clear();
	for(int i = 0; i < ui->artifacts->count(); ++i)
	{
		if(ui->artifacts->item(i)->checkState() == Qt::Checked)
			reward->grantedArtifacts.push_back(LIBRARY->artifacts()->getByIndex(i)->getId());
	}
	reward->spells.clear();
	for(int i = 0; i < ui->spells->count(); ++i)
	{
		if(ui->spells->item(i)->checkState() == Qt::Checked)
			reward->spells.push_back(LIBRARY->spells()->getByIndex(i)->getId());
	}

	reward->secondary.clear();
	for(int i = 0; i < ui->skills->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QComboBox*>(ui->skills->cellWidget(i, 1)))
		{
			if(widget->currentIndex() > 0)
				reward->secondary[LIBRARY->skills()->getByIndex(i)->getId()] = widget->currentIndex();
		}
	}

	reward->creatures.clear();
	for(int i = 0; i < ui->creatures->rowCount(); ++i)
	{
		int index = ui->creatures->item(i, 0)->data(Qt::UserRole).toInt();
		if(auto * widget = qobject_cast<QSpinBox*>(ui->creatures->cellWidget(i, 1)))
			if(widget->value())
				reward->creatures.emplace_back(LIBRARY->creatures()->getByIndex(index)->getId(), widget->value());
	}

	reward->spellCast.first = SpellID::NONE;
	if(ui->castSpellCheck->isChecked())
	{
		reward->spellCast.first = LIBRARY->spells()->getByIndex(ui->castSpell->itemData(ui->castSpell->currentIndex()).toInt())->getId();
		reward->spellCast.second = ui->castLevel->currentIndex();
	}

	reward->heroBonuses.clear();
	for(int i = 0; i < ui->bonuses->rowCount(); ++i)
	{
		auto dur = bonusDurationMap.at(ui->bonuses->item(i, 0)->text().toStdString());
		auto typ = static_cast<BonusType>(*LIBRARY->identifiers()->getIdentifier(ModScope::scopeBuiltin(), "bonus", ui->bonuses->item(i, 1)->text().toStdString()));
		auto val = ui->bonuses->item(i, 2)->data(Qt::UserRole).toInt();
		reward->heroBonuses.push_back(std::make_shared<Bonus>(dur, typ, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(id))); // to fix
	}

}

void RewardWidget::clearData()
{
	reward = nullptr;
	auto emptyReward = Rewardable::Reward();
	showData(&emptyReward);
}

void RewardWidget::disableRemoveObjectOption()
{
	ui->removeObject->hide();
	delete ui->removeObject;
	ui->removeObject = nullptr;
}

bool RewardWidget::isRewardLoaded() const
{
	return reward;
}

void RewardWidget::onCreatureAdd(QTableWidget * listWidget, QComboBox * comboWidget, QSpinBox * spinWidget)
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

void RewardWidget::on_rCreatureAdd_clicked()
{
	onCreatureAdd(ui->creatures, ui->creatureId, ui->creatureAmount);
}


void RewardWidget::on_rCreatureRemove_clicked()
{
	std::set<int, std::greater<int>> rowsToRemove;
	for(auto * i : ui->creatures->selectedItems())
		rowsToRemove.insert(i->row());

	for(auto i : rowsToRemove)
		ui->creatures->removeRow(i);
}

void RewardWidget::on_castSpellCheck_toggled(bool checked)
{
	ui->castSpell->setEnabled(checked);
	ui->castLevel->setEnabled(checked);
}

void RewardWidget::on_bonusAdd_clicked()
{
	auto * itemType = new QTableWidgetItem(ui->bonusType->currentText());
	auto * itemDur = new QTableWidgetItem(ui->bonusDuration->currentText());
	auto * itemVal = new QTableWidgetItem(QString::number(ui->bonusValue->value()));
	itemVal->setData(Qt::UserRole, ui->bonusValue->value());

	ui->bonuses->setRowCount(ui->bonuses->rowCount() + 1);
	ui->bonuses->setItem(ui->bonuses->rowCount() - 1, 0, itemDur);
	ui->bonuses->setItem(ui->bonuses->rowCount() - 1, 1, itemType);
	ui->bonuses->setItem(ui->bonuses->rowCount() - 1, 2, itemVal);
}

void RewardWidget::on_bonusRemove_clicked()
{
	std::set<int, std::greater<int>> rowsToRemove;
	for(auto * i : ui->bonuses->selectedItems())
		rowsToRemove.insert(i->row());

	for(auto i : rowsToRemove)
		ui->bonuses->removeRow(i);
}


void RewardWidget::printRewardInformation()
{
	if (!reward)
	{
		ui->rewardInfo->setText(QString());
		return;
	}
	QStringList textList;
	if (reward->heroLevel)
		textList += QObject::tr("Hero Level: %1").arg(reward->heroLevel);
	if (reward->heroExperience)
		textList += QObject::tr("Hero Experience: %1").arg(reward->heroExperience);
	if (reward->manaDiff)
		textList += QObject::tr("Mana Diff: %1").arg(reward->manaDiff);
	if (reward->manaPercentage > 0)
		textList += QObject::tr("Mana Percentage: %1").arg(reward->manaPercentage);
	if (reward->movePoints)
		textList += QObject::tr("Move Points: %1").arg(reward->movePoints);
	if (reward->movePercentage > 0)
		textList += QObject::tr("Move Percentage: %1").arg(reward->movePercentage);
	if (reward->primary[0] || reward->primary[1] || reward->primary[2] || reward->primary[3])
		textList += QObject::tr("Primary Skills: %1/%2/%3/%4").arg(reward->primary[0]).arg(reward->primary[1]).arg(reward->primary[2]).arg(reward->primary[3]);
	QStringList resourcesList;
	for(GameResID resource = GameResID::WOOD; resource < GameResID::COUNT ; resource++)
	{
		if(reward->resources[resource] == 0)
			continue;
		MetaString str;
		str.appendName(resource);
		resourcesList += QString("%1: %2").arg(QString::fromStdString(str.toString(&Translator::instance()))).arg(reward->resources[resource]);
	}
	if (!resourcesList.isEmpty())
		textList += QObject::tr("Resources: %1").arg(resourcesList.join(", "));
	if (!reward->grantedArtifacts.empty())
	{
		QStringList artifactsList;
		for (auto artifact : reward->grantedArtifacts)
			artifactsList += QString::fromStdString(LIBRARY->artifacts()->getById(artifact)->getNameTranslated());

		textList += QObject::tr("Artifacts: %1").arg(artifactsList.join(", "));
	}
	if (!reward->spells.empty())
	{
	QStringList spellsList;
	for (auto spell : reward->spells)
		spellsList += QString::fromStdString(LIBRARY->spells()->getById(spell)->getNameTranslated());

	textList += QObject::tr("Spells: %1").arg(spellsList.join(", "));
	}

	if (!reward->secondary.empty())
	{
		QStringList secondarySkillsList;
		for(auto & [skill, skillLevel] : reward->secondary)
			secondarySkillsList += QString("%1 (%2)").arg(QString::fromStdString(LIBRARY->skills()->getById(skill)->getNameTranslated())).arg(skillLevel);

		textList += QObject::tr("Secondary Skills: %1").arg(secondarySkillsList.join(", "));
	}
	if (!reward->creatures.empty())
	{
		QStringList creaturesList;
		for (auto & creature : reward->creatures)
		{
			creaturesList += QString("%1 %2").arg(creature.getCount()).arg(QString::fromStdString(creature.getType()->getNamePluralTranslated()));
		}
		textList += QObject::tr("Creatures: %1").arg(creaturesList.join(", "));
	}
	if (reward->spellCast.first != SpellID::NONE)
	{
		textList += QObject::tr("Spell Cast: %1 (%2)").arg(QString::fromStdString(LIBRARY->spells()->getById(reward->spellCast.first)->getNameTranslated())).arg(reward->spellCast.second);
	}

	if (!reward->heroBonuses.empty())
	{
		QStringList bonusesList;
		for (auto & bonus : reward->heroBonuses)
		{
			std::string bonusName = LIBRARY->bth->bonusToString(bonus->type);
			bonusesList += QString("%1 %2 (%3)").arg(QString::fromStdString(vstd::findKey(bonusDurationMap, bonus->duration))).arg(QString::fromStdString(bonusName)).arg(bonus->val);
		}
		textList += QObject::tr("Bonuses: %1").arg(bonusesList.join(", "));
	}

	if (!textList.isEmpty())
		ui->rewardInfo->setText(textList.join("\n"));
	else
		ui->rewardInfo->setText(QString(QObject::tr("Empty Reward")));

}
