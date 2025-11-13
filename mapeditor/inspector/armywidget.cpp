/*
 * armywidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "armywidget.h"
#include "ui_armywidget.h"
#include "CCreatureHandler.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjects/army/CStackInstance.h"

ArmyWidget::ArmyWidget(CArmedInstance & a, QWidget *parent) :
	QDialog(parent),
	army(a),
	ui(new Ui::ArmyWidget)
{
	ui->setupUi(this);
	
	uiCounts[0] = ui->count0; uiSlots[0] = ui->slot0;
	uiCounts[1] = ui->count1; uiSlots[1] = ui->slot1;
	uiCounts[2] = ui->count2; uiSlots[2] = ui->slot2;
	uiCounts[3] = ui->count3; uiSlots[3] = ui->slot3;
	uiCounts[4] = ui->count4; uiSlots[4] = ui->slot4;
	uiCounts[5] = ui->count5; uiSlots[5] = ui->slot5;
	uiCounts[6] = ui->count6; uiSlots[6] = ui->slot6;
	
	for(int i = 0; i < TOTAL_SLOTS; ++i)
	{
		uiSlots[i]->addItem("");
		uiSlots[i]->setItemData(0, -1);
		
		for(int c = 0; c < LIBRARY->creh->objects.size(); ++c)
		{
			auto const & creature = LIBRARY->creh->objects[c];
			uiSlots[i]->insertItem(c + 1, creature->getNamePluralTranslated().c_str());
			uiSlots[i]->setItemData(c + 1, creature->getIndex());
		}

		uiSlots[i]->completer()->setCompletionMode(QCompleter::PopupCompletion);
		uiSlots[i]->completer()->setFilterMode(Qt::MatchContains);
	}
	
	ui->formationTight->setChecked(true);
}

int ArmyWidget::searchItemIndex(int slotId, CreatureID creId) const
{
	for(int i = 0; i < uiSlots[slotId]->count(); ++i)
	{
		if(creId.getNum() == uiSlots[slotId]->itemData(i).toInt())
			return i;
	}
	return 0;
}

void ArmyWidget::obtainData()
{
	for(int i = 0; i < TOTAL_SLOTS; ++i)
	{
		if(army.hasStackAtSlot(SlotID(i)))
		{
			auto * creature = army.getCreature(SlotID(i));
			uiSlots[i]->setCurrentIndex(searchItemIndex(i, creature->getId()));
			uiCounts[i]->setValue(army.getStackCount(SlotID(i)));
		}
	}
	
	if(army.formation == EArmyFormation::TIGHT)
		ui->formationTight->setChecked(true);
	else
		ui->formationWide->setChecked(true);
}

bool ArmyWidget::commitChanges()
{
	bool isArmed = false;
	for(int i = 0; i < TOTAL_SLOTS; ++i)
	{
		CreatureID creId(uiSlots[i]->itemData(uiSlots[i]->currentIndex()).toInt());
		if(creId == CreatureID::NONE)
		{
			if(army.hasStackAtSlot(SlotID(i)))
				army.eraseStack(SlotID(i));
		}
		else
		{
			isArmed = true;
			int amount = uiCounts[i]->value();
			if(amount)
			{
				army.setCreature(SlotID(i), creId, amount);
			}
			else
			{
				if(army.hasStackAtSlot(SlotID(i)))
					army.eraseStack(SlotID(i));
				army.putStack(SlotID(i), std::make_unique<CStackInstance>(army.cb, creId, amount, false));
			}
		}
	}
	
	army.setFormation(ui->formationTight->isChecked() ? EArmyFormation::TIGHT : EArmyFormation::LOOSE );
	return isArmed;
}

ArmyWidget::~ArmyWidget()
{
	delete ui;
}

ArmyDelegate::ArmyDelegate(CArmedInstance & t)
	: BaseInspectorItemDelegate()
	, army(t)
{
}

QWidget * ArmyDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new ArmyWidget(army, parent);
}

void ArmyDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<ArmyWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void ArmyDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<ArmyWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void ArmyDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList;
	for(const auto & [_, stack] : army.stacks)
	{
		if(stack->getCount() != 0 && stack->getCreature() != nullptr)
			textList += QString("%1 %2").arg(stack->getCount()).arg(QString::fromStdString(stack->getCreature()->getNamePluralTranslated()));
	}

	setModelTextData(model, index, textList);
}
