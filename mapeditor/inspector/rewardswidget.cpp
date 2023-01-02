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
#include "rewardswidget.h"
#include "ui_rewardswidget.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/StringConstants.h"

RewardsWidget::RewardsWidget(const CMap & m, CGPandoraBox & p, QWidget *parent) :
	QDialog(parent),
	map(m),
	pandora(&p),
	seerhut(nullptr),
	ui(new Ui::RewardsWidget)
{
	ui->setupUi(this);
	
	for(auto & type : rewardTypes)
		ui->rewardType->addItem(QString::fromStdString(type));
}

RewardsWidget::RewardsWidget(const CMap & m, CGSeerHut & p, QWidget *parent) :
	QDialog(parent),
	map(m),
	pandora(nullptr),
	seerhut(&p),
	ui(new Ui::RewardsWidget)
{
	ui->setupUi(this);
	
	for(auto & type : rewardTypes)
		ui->rewardType->addItem(QString::fromStdString(type));
}

RewardsWidget::~RewardsWidget()
{
	delete ui;
}

QList<QString> RewardsWidget::getListForType(RewardType typeId)
{
	assert(typeId < rewardTypes.size());
	QList<QString> result;
	
	switch (typeId) {
		case RewardType::RESOURCE:
			//to convert string to index WOOD = 0, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD, MITHRIL,
			result.append("Wood");
			result.append("Mercury");
			result.append("Ore");
			result.append("Sulfur");
			result.append("Crystals");
			result.append("Gems");
			result.append("Gold");
			break;
			
		case RewardType::PRIMARY_SKILL:
			for(auto s : PrimarySkill::names)
				result.append(QString::fromStdString(s));
			break;
			
		case RewardType::SECONDARY_SKILL:
			for(int i = 0; i < map.allowedAbilities.size(); ++i)
			{
				if(map.allowedAbilities[i])
					result.append(QString::fromStdString(VLC->skillh->objects.at(i)->getNameTranslated()));
			}
			break;
			
		case RewardType::ARTIFACT:
			for(int i = 0; i < map.allowedArtifact.size(); ++i)
			{
				if(map.allowedArtifact[i])
					result.append(QString::fromStdString(VLC->arth->objects.at(i)->getName()));
			}
			break;
			
		case RewardType::SPELL:
			for(int i = 0; i < map.allowedSpell.size(); ++i)
			{
				if(map.allowedSpell[i])
					result.append(QString::fromStdString(VLC->spellh->objects.at(i)->getName()));
			}
			break;
			
		case RewardType::CREATURE:
			for(auto creature : VLC->creh->objects)
			{
				result.append(QString::fromStdString(creature->getNameSingularTranslated()));
			}
			break;
	}
	return result;
}

void RewardsWidget::on_rewardType_activated(int index)
{
	ui->rewardList->clear();
	ui->rewardList->setEnabled(true);
	assert(index < rewardTypes.size());
	
	auto l = getListForType(RewardType(index));
	if(l.empty())
		ui->rewardList->setEnabled(false);
	
	for(auto & s : l)
		ui->rewardList->addItem(s);
}

void RewardsWidget::obtainData()
{
	if(pandora)
	{
		if(pandora->gainedExp > 0)
			addReward(RewardType::EXPERIENCE, 0, pandora->gainedExp);
		if(pandora->manaDiff)
			addReward(RewardType::MANA, 0, pandora->manaDiff);
		if(pandora->moraleDiff)
			addReward(RewardType::MORALE, 0, pandora->moraleDiff);
		if(pandora->luckDiff)
			addReward(RewardType::LUCK, 0, pandora->luckDiff);
		if(pandora->resources.nonZero())
		{
			for(Res::ResourceSet::nziterator resiter(pandora->resources); resiter.valid(); ++resiter)
				addReward(RewardType::RESOURCE, resiter->resType, resiter->resVal);
		}
		for(int idx = 0; idx < pandora->primskills.size(); ++idx)
		{
			if(pandora->primskills[idx])
				addReward(RewardType::PRIMARY_SKILL, idx, pandora->primskills[idx]);
		}
		assert(pandora->abilities.size() == pandora->abilityLevels.size());
		for(int idx = 0; idx < pandora->abilities.size(); ++idx)
		{
			addReward(RewardType::SECONDARY_SKILL, pandora->abilities[idx].getNum(), pandora->abilityLevels[idx]);
		}
		for(auto art : pandora->artifacts)
		{
			addReward(RewardType::ARTIFACT, art.getNum(), 1);
		}
		for(auto spell : pandora->spells)
		{
			addReward(RewardType::SPELL, spell.getNum(), 1);
		}
		for(int i = 0; i < pandora->creatures.Slots().size(); ++i)
		{
			if(auto c = pandora->creatures.getCreature(SlotID(i)))
				addReward(RewardType::CREATURE, c->getId(), pandora->creatures.getStackCount(SlotID(i)));
		}
	}
	
	if(seerhut)
	{
		switch(seerhut->rewardType)
		{
			case CGSeerHut::ERewardType::EXPERIENCE:
				addReward(RewardType::EXPERIENCE, 0, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::MANA_POINTS:
				addReward(RewardType::MANA, 0, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::MORALE_BONUS:
				addReward(RewardType::MORALE, 0, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::LUCK_BONUS:
				addReward(RewardType::LUCK, 0, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::RESOURCES:
				addReward(RewardType::RESOURCE, seerhut->rID, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::PRIMARY_SKILL:
				addReward(RewardType::PRIMARY_SKILL, seerhut->rID, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::SECONDARY_SKILL:
				addReward(RewardType::SECONDARY_SKILL, seerhut->rID, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::ARTIFACT:
				addReward(RewardType::ARTIFACT, seerhut->rID, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::SPELL:
				addReward(RewardType::SPELL, seerhut->rID, seerhut->rVal);
				break;
				
			case CGSeerHut::ERewardType::CREATURE:
				addReward(RewardType::CREATURE, seerhut->rID, seerhut->rVal);
				break;
				
			default:
				break;
		}
	}
}

bool RewardsWidget::commitChanges()
{
	bool haveRewards = false;
	if(pandora)
	{
		pandora->abilities.clear();
		pandora->abilityLevels.clear();
		pandora->primskills.resize(GameConstants::PRIMARY_SKILLS, 0);
		pandora->resources = Res::ResourceSet();
		pandora->artifacts.clear();
		pandora->spells.clear();
		pandora->creatures.clear();
		
		for(int row = 0; row < rewards; ++row)
		{
			haveRewards = true;
			int typeId = ui->rewardsTable->item(row, 0)->data(Qt::UserRole).toInt();
			int listId = ui->rewardsTable->item(row, 1) ? ui->rewardsTable->item(row, 1)->data(Qt::UserRole).toInt() : 0;
			int amount = ui->rewardsTable->item(row, 2)->data(Qt::UserRole).toInt();
			switch(typeId)
			{
				case RewardType::EXPERIENCE:
					pandora->gainedExp = amount;
					break;
					
				case RewardType::MANA:
					pandora->manaDiff = amount;
					break;
					
				case RewardType::MORALE:
					pandora->moraleDiff = amount;
					break;
					
				case RewardType::LUCK:
					pandora->luckDiff = amount;
					break;
					
				case RewardType::RESOURCE:
					pandora->resources.at(listId) = amount;
					break;
					
				case RewardType::PRIMARY_SKILL:
					pandora->primskills[listId] = amount;
					break;
					
				case RewardType::SECONDARY_SKILL:
					pandora->abilities.push_back(SecondarySkill(listId));
					pandora->abilityLevels.push_back(amount);
					break;
					
				case RewardType::ARTIFACT:
					pandora->artifacts.push_back(ArtifactID(listId));
					break;
					
				case RewardType::SPELL:
					pandora->spells.push_back(SpellID(listId));
					break;
					
				case RewardType::CREATURE:
					auto slot = pandora->creatures.getFreeSlot();
					if(slot != SlotID() && amount > 0)
						pandora->creatures.addToSlot(slot, CreatureID(listId), amount);
					break;
			}
		}
	}
	if(seerhut)
	{
		for(int row = 0; row < rewards; ++row)
		{
			haveRewards = true;
			int typeId = ui->rewardsTable->item(row, 0)->data(Qt::UserRole).toInt();
			int listId = ui->rewardsTable->item(row, 1) ? ui->rewardsTable->item(row, 1)->data(Qt::UserRole).toInt() : 0;
			int amount = ui->rewardsTable->item(row, 2)->data(Qt::UserRole).toInt();
			seerhut->rewardType = CGSeerHut::ERewardType(typeId + 1);
			seerhut->rID = listId;
			seerhut->rVal = amount;
		}
	}
	return haveRewards;
}

void RewardsWidget::on_rewardList_activated(int index)
{
	ui->rewardAmount->setText(QStringLiteral("1"));
}

void RewardsWidget::addReward(RewardsWidget::RewardType typeId, int listId, int amount)
{
	//for seerhut there could be the only one reward
	if(!pandora && seerhut && rewards)
		return;
	
	ui->rewardsTable->setRowCount(++rewards);
	
	auto itemType = new QTableWidgetItem(QString::fromStdString(rewardTypes[typeId]));
	itemType->setData(Qt::UserRole, typeId);
	ui->rewardsTable->setItem(rewards - 1, 0, itemType);
	
	auto l = getListForType(typeId);
	if(!l.empty())
	{
		auto itemCurr = new QTableWidgetItem(getListForType(typeId)[listId]);
		itemCurr->setData(Qt::UserRole, listId);
		ui->rewardsTable->setItem(rewards - 1, 1, itemCurr);
	}
	
	QString am = QString::number(amount);
	switch(ui->rewardType->currentIndex())
	{
		case 6:
			if(amount <= 1)
				am = "Basic";
			if(amount == 2)
				am = "Advanced";
			if(amount >= 3)
				am = "Expert";
			break;
			
		case 7:
		case 8:
			am = "";
			amount = 1;
			break;
	}
	auto itemCount = new QTableWidgetItem(am);
	itemCount->setData(Qt::UserRole, amount);
	ui->rewardsTable->setItem(rewards - 1, 2, itemCount);
}


void RewardsWidget::on_buttonAdd_clicked()
{
	addReward(RewardType(ui->rewardType->currentIndex()), ui->rewardList->currentIndex(), ui->rewardAmount->text().toInt());
}


void RewardsWidget::on_buttonRemove_clicked()
{
	auto currentRow = ui->rewardsTable->currentRow();
	if(currentRow != -1)
	{
		ui->rewardsTable->removeRow(currentRow);
		--rewards;
	}
}


void RewardsWidget::on_buttonClear_clicked()
{
	ui->rewardsTable->clear();
	rewards = 0;
}


void RewardsWidget::on_rewardsTable_itemSelectionChanged()
{
	/*auto type = ui->rewardsTable->item(ui->rewardsTable->currentRow(), 0);
	ui->rewardType->setCurrentIndex(type->data(Qt::UserRole).toInt());
	ui->rewardType->activated(ui->rewardType->currentIndex());
	
	type = ui->rewardsTable->item(ui->rewardsTable->currentRow(), 1);
	ui->rewardList->setCurrentIndex(type->data(Qt::UserRole).toInt());
	ui->rewardList->activated(ui->rewardList->currentIndex());
	
	type = ui->rewardsTable->item(ui->rewardsTable->currentRow(), 2);
	ui->rewardAmount->setText(QString::number(type->data(Qt::UserRole).toInt()));*/
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
		auto hasReward = ed->commitChanges();
		model->setData(index, "dummy");
		if(hasReward)
			model->setData(index, "HAS REWARD");
		else
			model->setData(index, "");
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

RewardsPandoraDelegate::RewardsPandoraDelegate(const CMap & m, CGPandoraBox & t): map(m), pandora(t), RewardsDelegate()
{
}

QWidget * RewardsPandoraDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new RewardsWidget(map, pandora, parent);
}

RewardsSeerhutDelegate::RewardsSeerhutDelegate(const CMap & m, CGSeerHut & t): map(m), seerhut(t), RewardsDelegate()
{
}

QWidget * RewardsSeerhutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new RewardsWidget(map, seerhut, parent);
}
