/*
 * townspellswidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "townspellswidget.h"
#include "ui_townspellswidget.h"
#include "inspector.h"
#include "mapeditorroles.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/spells/CSpellHandler.h"

TownSpellsWidget::TownSpellsWidget(CGTownInstance & town, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::TownSpellsWidget),
	town(town)
{
	ui->setupUi(this);

	possibleSpellLists = { ui->possibleSpellList1, ui->possibleSpellList2, ui->possibleSpellList3, ui->possibleSpellList4, ui->possibleSpellList5 };
	requiredSpellLists = { ui->requiredSpellList1, ui->requiredSpellList2, ui->requiredSpellList3, ui->requiredSpellList4, ui->requiredSpellList5 };

	std::array<BuildingID, 5> mageGuilds = {BuildingID::MAGES_GUILD_1, BuildingID::MAGES_GUILD_2, BuildingID::MAGES_GUILD_3, BuildingID::MAGES_GUILD_4, BuildingID::MAGES_GUILD_5};
	for (int i = 0; i < mageGuilds.size(); i++)
	{
		ui->tabWidget->setTabEnabled(i, vstd::contains(town.getTown()->buildings, mageGuilds[i]));
	}
}

TownSpellsWidget::~TownSpellsWidget()
{
	delete ui;
}


void TownSpellsWidget::obtainData()
{
	initSpellLists();
	if (vstd::contains(town.possibleSpells, SpellID::PRESET)) {
		ui->customizeSpells->setChecked(true);
	}
	else
	{
		ui->customizeSpells->setChecked(false);
		ui->tabWidget->setEnabled(false);
	}
}

void TownSpellsWidget::resetSpells()
{
	town.possibleSpells.clear();
	town.obligatorySpells.clear();
	for (auto spellID : LIBRARY->spellh->getDefaultAllowed())
		town.possibleSpells.push_back(spellID);
}

void TownSpellsWidget::initSpellLists()
{
	auto spells = LIBRARY->spellh->getDefaultAllowed();
	for (int i = 0; i < GameConstants::SPELL_LEVELS; i++)
	{
		std::vector<SpellID> spellsByLevel;
		auto getSpellsByLevel = [i](auto spellID) {
			return spellID.toEntity(LIBRARY)->getLevel() == i + 1;
		};
		vstd::copy_if(spells, std::back_inserter(spellsByLevel), getSpellsByLevel);
		possibleSpellLists[i]->clear();
		requiredSpellLists[i]->clear();
		for (auto spellID : spellsByLevel)
		{
			auto spell = spellID.toEntity(LIBRARY);
			auto * possibleItem = new QListWidgetItem(QString::fromStdString(spell->getNameTranslated()));
			possibleItem->setData(MapEditorRoles::SpellIDRole, QVariant::fromValue(spell->getIndex()));
			possibleItem->setFlags(possibleItem->flags() | Qt::ItemIsUserCheckable);
			possibleItem->setCheckState(vstd::contains(town.possibleSpells, spell->getId()) ? Qt::Checked : Qt::Unchecked);
			possibleSpellLists[i]->addItem(possibleItem);

			auto * requiredItem = new QListWidgetItem(QString::fromStdString(spell->getNameTranslated()));
			requiredItem->setData(MapEditorRoles::SpellIDRole, QVariant::fromValue(spell->getIndex()));
			requiredItem->setFlags(requiredItem->flags() | Qt::ItemIsUserCheckable);
			requiredItem->setCheckState(vstd::contains(town.obligatorySpells, spell->getId()) ? Qt::Checked : Qt::Unchecked);
			requiredSpellLists[i]->addItem(requiredItem);
		}
	}
}

void TownSpellsWidget::commitChanges()
{
	if (!ui->tabWidget->isEnabled())
	{
		resetSpells();
		return;
	}

	auto updateTownSpellList = [](auto uiSpellLists, auto & townSpellList) {
		for (const QListWidget * spellList : uiSpellLists)
		{
			for (int i = 0; i < spellList->count(); ++i)
			{
				const auto * item = spellList->item(i);
				if (item->checkState() == Qt::Checked)
				{
					townSpellList.push_back(item->data(MapEditorRoles::SpellIDRole).toInt());
				}
			}
		}
	};

	town.possibleSpells.clear();
	town.obligatorySpells.clear();
	town.possibleSpells.push_back(SpellID::PRESET);
	updateTownSpellList(possibleSpellLists, town.possibleSpells);
	updateTownSpellList(requiredSpellLists, town.obligatorySpells);
}

void TownSpellsWidget::on_customizeSpells_toggled(bool checked)
{
	if (checked)
	{
		town.possibleSpells.push_back(SpellID::PRESET);
	}
	else
	{
		resetSpells();
	}
	ui->tabWidget->setEnabled(checked);
	initSpellLists();
}

TownSpellsDelegate::TownSpellsDelegate(CGTownInstance & town) : BaseInspectorItemDelegate(), town(town)
{
}

QWidget * TownSpellsDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new TownSpellsWidget(town, parent);
}

void TownSpellsDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<TownSpellsWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void TownSpellsDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<TownSpellsWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void TownSpellsDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList;
	QStringList requiredSpellsList;
	QStringList possibleSpellsList;
	if(vstd::contains(town.possibleSpells, SpellID::PRESET)) {
		textList += QObject::tr("Custom Spells:");
		textList += QObject::tr("Required:");
		for(auto spellID : town.obligatorySpells)
		{
			auto spell = spellID.toEntity(LIBRARY);
			requiredSpellsList += QString::fromStdString(spell->getNameTranslated());
		}
		textList += requiredSpellsList.join(",");
		textList += QObject::tr("Possible:");
		for(auto spellID : town.possibleSpells)
		{
			if(spellID == SpellID::PRESET)
				continue;
			auto spell = spellID.toEntity(LIBRARY);
			possibleSpellsList += QString::fromStdString(spell->getNameTranslated());
		}
		textList += possibleSpellsList.join(",");
	}
	else
	{
		textList += QObject::tr("Default Spells");
	}
	setModelTextData(model, index, textList);
}
