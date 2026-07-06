/*
 * scholarwidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "inspector.h"
#include "scholarwidget.h"
#include "ui_scholarwidget.h"
#include "mapcontroller.h"

#include "lib/CSkillHandler.h"
#include "lib/GameLibrary.h"
#include "lib/constants/StringConstants.h"
#include "lib/mapping/CMap.h"
#include "lib/modding/ModScope.h"
#include "lib/modding/IdentifierStorage.h"
#include "lib/spells/CSpellHandler.h"

ScholarWidget::ScholarWidget(CRewardableObject & scholar, MapController & controller, QWidget * parent)
	: QDialog(parent), ui(new Ui::ScholarWidget), scholar(scholar), extractor(controller.getCallback()), controller(controller)
{
	ui->setupUi(this);
	rewardsData = {
		{ui->pSkillsCheck, ui->pSkills, {"primarySkill", "gainedStat"},    "primary skill",   JsonNode(2)},
		{ui->sSkillsCheck, ui->sSkills, {"secondarySkill", "gainedSkill"}, "secondary skill", JsonNode(1)},
		{ui->spellsCheck,  ui->spells,  {"spell", "gainedSpell"},          "spell",           JsonNode(0)}
	};
}

ScholarWidget::~ScholarWidget()
{
	delete ui;
}

void ScholarWidget::loadData()
{
	for(int i = 0; i < GameConstants::PRIMARY_SKILLS; i++)
	{
		ui->pSkills->insertItem(i, QString::fromStdString(LIBRARY->generaltexth->primarySkillNames[i]));
		ui->pSkills->setItemData(i, QString::fromStdString(NPrimarySkill::names[i]));
	}
	int ssi = 0;
	for(const auto & skill : LIBRARY->skillh->objects)
	{
		ui->sSkills->insertItem(ssi, QString::fromStdString(skill->getNameTranslated()));
		ui->sSkills->setItemData(ssi, QString::fromStdString(skill->getJsonKey()));
		ssi++;
	}
	int spi = 0;
	for(const auto & spell : LIBRARY->spellh->objects)
	{
		ui->spells->insertItem(spi, QString::fromStdString(spell->getNameTranslated()));
		ui->spells->setItemData(spi, QString::fromStdString(spell->getJsonKey()));
		spi++;
	}

	auto allowCompletion = [](QComboBox * comboBox)
	{
		comboBox->completer()->setCompletionMode(QCompleter::PopupCompletion);
		comboBox->completer()->setFilterMode(Qt::MatchContains);
	};

	allowCompletion(ui->pSkills);
	allowCompletion(ui->sSkills);
	allowCompletion(ui->spells);

	loadState();
	changeComboBoxesAllowedState();
}

void ScholarWidget::commitChanges()
{
	if(ui->random->isChecked())
	{
		scholar.configuration.variables.preset.clear();
		return;
	}
	for(const RewardData & rd : rewardsData)
	{
		if(rd.radioButton->isChecked())
		{
			scholar.configuration.variables.preset.clear();
			JsonNode variable;
			variable.String() = rd.comboBox->itemData(rd.comboBox->currentIndex()).toString().toStdString();
			variable.setModScope(ModScope::scopeGame());
			scholar.configuration.presetVariable(rd.variables[0], rd.variables[1], variable);
			scholar.configuration.presetVariable("dice", "map", rd.dice);
			return;
		}
	}
}

void ScholarWidget::on_pSkillsCheck_toggled(bool checked)
{
	changeComboBoxesAllowedState();
}
void ScholarWidget::on_sSkillsCheck_toggled(bool checked)
{
	changeComboBoxesAllowedState();
}
void ScholarWidget::on_spellsCheck_toggled(bool checked)
{
	changeComboBoxesAllowedState();
}

void ScholarWidget::loadState()
{
	for(const RewardData & rd : rewardsData)
	{
		JsonNode variableNode = scholar.configuration.getPresetVariable(rd.variables[0], rd.variables[1]);
		if(!variableNode.isNull())
		{
			std::string presetVariable = variableNode.String();
			int index = rd.comboBox->findData(QString::fromStdString(presetVariable));
			if(index != -1)
			{
				rd.radioButton->toggle();
				rd.comboBox->setCurrentIndex(index);
				return;
			}
			else
			{
				showInvalidPresetWarning(rd.name, presetVariable);
				return;
			}
		}
	}
	ui->random->toggle();
}

void ScholarWidget::changeComboBoxesAllowedState()
{
	for(const RewardData & rd : rewardsData)
		rd.comboBox->setDisabled(!rd.radioButton->isChecked());
}

void ScholarWidget::showInvalidPresetWarning(std::string type, std::string name)
{
	auto warning = tr(presetNotFoundWarning).arg(type.c_str()).arg(name.c_str());
	ui->label->setText(warning);
	adjustSize();
}

ScholarDelegate::ScholarDelegate(MapController & controller, CRewardableObject & scholar)
	: BaseInspectorItemDelegate(), scholar(scholar), controller(controller)
{
}

QWidget * ScholarDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new ScholarWidget(scholar, controller, parent);
}

void ScholarDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if(auto * aw = qobject_cast<ScholarWidget *>(editor))
	{
		aw->loadData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void ScholarDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if(auto * ed = qobject_cast<ScholarWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void ScholarDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList;
	JsonNode presetPs = scholar.configuration.getPresetVariable("primarySkill", "gainedStat");
	JsonNode presetSs = scholar.configuration.getPresetVariable("secondarySkill", "gainedSkill");
	JsonNode presetSp = scholar.configuration.getPresetVariable("spell", "gainedSpell");
	if(!presetPs.isNull())
	{
		textList += QObject::tr(presetPs.String().c_str());
	}
	else if(!presetSs.isNull())
	{
		addEntityName(textList, static_cast<const Entity *>(LIBRARY->skillh->getByName(presetSs.String())));
	}
	else if(!presetSp.isNull())
	{
		addEntityName(textList, static_cast<const Entity *>(LIBRARY->spellh->getByName(presetSp.String())));
	}
	else
		textList += QObject::tr("Default");
	setModelTextData(model, index, textList);
}

void ScholarDelegate::addEntityName(QStringList & textList, const Entity * const entity) const
{
	if(entity)
		textList += QString::fromStdString(entity->getNameTranslated());
	else
		textList += QObject::tr("Invalid");
}
