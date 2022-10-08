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

QList<QString> RewardsWidget::getListForType(int typeId)
{
	assert(typeId < rewardTypes.size());
	QList<QString> result;
	
	switch (typeId) {
		case 4: //resources
				//to convert string to index WOOD = 0, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD, MITHRIL,
			result.append("Wood");
			result.append("Mercury");
			result.append("Ore");
			result.append("Sulfur");
			result.append("Crystals");
			result.append("Gems");
			result.append("Gold");
			break;
			
		case 5:
			for(auto s : PrimarySkill::names)
				result.append(QString::fromStdString(s));
			break;
			
		case 6:
			//abilities
			for(int i = 0; i < map.allowedAbilities.size(); ++i)
			{
				if(map.allowedAbilities[i])
					result.append(QString::fromStdString(VLC->skillh->objects.at(i)->getName()));
			}
			break;
			
		case 7:
			//arts
			for(int i = 0; i < map.allowedArtifact.size(); ++i)
			{
				if(map.allowedArtifact[i])
					result.append(QString::fromStdString(VLC->arth->objects.at(i)->getName()));
			}
			break;
			
		case 8:
			//spells
			for(int i = 0; i < map.allowedSpell.size(); ++i)
			{
				if(map.allowedSpell[i])
					result.append(QString::fromStdString(VLC->spellh->objects.at(i)->getName()));
			}
			break;
			
		case 9:
			//creatures
			for(auto creature : VLC->creh->objects)
			{
				result.append(QString::fromStdString(creature->getName()));
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
	
	auto l = getListForType(index);
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
			addReward(0, 0, pandora->gainedExp);
		if(pandora->manaDiff)
			addReward(1, 0, pandora->manaDiff);
		if(pandora->moraleDiff)
			addReward(2, 0, pandora->moraleDiff);
		if(pandora->luckDiff)
			addReward(3, 0, pandora->luckDiff);
		if(pandora->resources.nonZero())
		{
			for(Res::ResourceSet::nziterator resiter(pandora->resources); resiter.valid(); ++resiter)
				addReward(4, resiter->resType, resiter->resVal);
		}
		for(int idx = 0; idx < pandora->primskills.size(); ++idx)
		{
			if(pandora->primskills[idx])
				addReward(5, idx, pandora->primskills[idx]);
		}
		assert(pandora->abilities.size() == pandora->abilityLevels.size());
		for(int idx = 0; idx < pandora->abilities.size(); ++idx)
		{
			addReward(6, pandora->abilities[idx].getNum(), pandora->abilityLevels[idx]);
		}
		for(auto art : pandora->artifacts)
		{
			addReward(7, art.getNum(), 1);
		}
		for(auto spell : pandora->spells)
		{
			addReward(8, spell.getNum(), 1);
		}
		for(int i = 0; i < pandora->creatures.Slots().size(); ++i)
		{
			if(auto c = pandora->creatures.getCreature(SlotID(i)))
				addReward(9, c->getId(), pandora->creatures.getStackCount(SlotID(i)));
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
				case 0:
					pandora->gainedExp = amount;
					break;
					
				case 1:
					pandora->manaDiff = amount;
					break;
					
				case 2:
					pandora->moraleDiff = amount;
					break;
					
				case 3:
					pandora->luckDiff = amount;
					break;
					
				case 4:
					pandora->resources.at(listId) = amount;
					break;
					
				case 5:
					pandora->primskills[listId] = amount;
					break;
					
				case 6:
					pandora->abilities.push_back(SecondarySkill(listId));
					pandora->abilityLevels.push_back(amount);
					break;
					
				case 7:
					pandora->artifacts.push_back(ArtifactID(listId));
					break;
					
				case 8:
					pandora->spells.push_back(SpellID(listId));
					break;
					
				case 9:
					auto slot = pandora->creatures.getFreeSlot();
					if(slot != SlotID() && amount > 0)
						pandora->creatures.addToSlot(slot, CreatureID(listId), amount);
					break;
			}
		}
	}
	return haveRewards;
}

void RewardsWidget::on_rewardList_activated(int index)
{
	ui->rewardAmount->setText(QStringLiteral("1"));
}

void RewardsWidget::addReward(int typeId, int listId, int amount)
{
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
	addReward(ui->rewardType->currentIndex(), ui->rewardList->currentIndex(), ui->rewardAmount->text().toInt());
}


void RewardsWidget::on_buttonRemove_clicked()
{
	ui->rewardsTable->removeRow(ui->rewardsTable->currentRow());
	--rewards;
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


RewardsPandoraDelegate::RewardsPandoraDelegate(const CMap & m, CGPandoraBox & t): map(m), pandora(t), QStyledItemDelegate()
{
}

QWidget * RewardsPandoraDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new RewardsWidget(map, pandora, parent);
}

void RewardsPandoraDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
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

void RewardsPandoraDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<RewardsWidget *>(editor))
	{
		auto isArmed = ed->commitChanges();
		model->setData(index, "dummy");
		if(isArmed)
			model->setData(index, "HAS REWARD");
		else
			model->setData(index, "");
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}
