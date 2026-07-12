/*
 * shrinewidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WidgetInitializationException.h"
#include "inspector.h"
#include "mapcontroller.h"
#include "shrinewidget.h"
#include "ui_shrinewidget.h"

#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjectConstructors/IObjectInfo.h"
#include "../../lib/rewardable/Info.h"
#include "../helper.h"
#include "lib/CSkillHandler.h"
#include "lib/GameLibrary.h"
#include "lib/constants/StringConstants.h"
#include "lib/mapping/CMap.h"
#include "lib/modding/IdentifierStorage.h"
#include "lib/modding/ModScope.h"
#include "lib/spells/CSpellHandler.h"

ShrineWidget::ShrineWidget(CRewardableObject & shrine, MapController & controller, QWidget * parent)
	: QDialog(parent), ui(new Ui::ShrineWidget), shrine(shrine), extractor(controller.getCallback()), controller(controller)
{
	ui->setupUi(this);
	Helper::decorateDialog(this);
}

ShrineWidget::~ShrineWidget()
{
	delete ui;
}

void ShrineWidget::loadData()
{
	int i = 0;
	si64 spellLevel = getSpellLevel();
	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(spell->isCommonHeroSpell() && spell->getLevel() == spellLevel)
		{
			ui->spells->insertItem(i, QString::fromStdString(spell->getNameTranslated()));
			ui->spells->setItemData(i, QString::fromStdString(spell->getJsonKey()));
			i++;
		}
	}

	ui->spells->completer()->setCompletionMode(QCompleter::PopupCompletion);
	ui->spells->completer()->setFilterMode(Qt::MatchContains);

	loadState();
}

void ShrineWidget::commitChanges()
{
	shrine.configuration.variables.preset.clear();
	if(ui->random->isChecked())
		return;

	auto & scb = ui->spells;
	JsonNode variable;
	variable.String() = scb->itemData(scb->currentIndex()).toString().toStdString();
	variable.setModScope(ModScope::scopeGame());
	shrine.configuration.presetVariable(PRESET_CATEGORY, PRESET_NAME, variable);
}

si64 ShrineWidget::getSpellLevel()
{
	static const QString genericWarningMessage = tr("MapEditor was unable to read intended spell level for this shrine type");
	std::unique_ptr<IObjectInfo> objectInfo = LIBRARY->objtypeh->getHandlerFor(shrine.ID, shrine.subID)->getObjectInfo();
	JsonNode spellLevelNode;
	try
	{
		spellLevelNode = dynamic_cast<Rewardable::Info &>(*objectInfo.get()).getParameters()["variables"][PRESET_CATEGORY][PRESET_NAME]["level"];
	}
	catch(std::runtime_error &)
	{
		throw WidgetInitializationException(genericWarningMessage);
	}
	if(!spellLevelNode.isNumber())
		throw WidgetInitializationException(genericWarningMessage);
	si64 spellLevel = spellLevelNode.Integer();
	if(spellLevel < 1 || spellLevel > GameConstants::SPELL_LEVELS)
		throw WidgetInitializationException(tr("Intended spell level %1 for this shrine type is invalid").arg(spellLevel));
	return spellLevel;
}

void ShrineWidget::loadState()
{
	JsonNode variableNode = shrine.configuration.getPresetVariable(PRESET_CATEGORY, PRESET_NAME);
	if(!variableNode.isNull())
	{
		const std::string & presetVariable = variableNode.String();
		int index = ui->spells->findData(QString::fromStdString(presetVariable));
		if(index != -1)
		{
			ui->custom->toggle();
			ui->spells->setCurrentIndex(index);
			return;
		}
		else
		{
			showInvalidPresetWarning(presetVariable);
		}
	}
	ui->random->toggle();
}

void ShrineWidget::changeComboBoxAllowedState()
{
	ui->spells->setDisabled(ui->random->isChecked());
}

void ShrineWidget::showInvalidPresetWarning(std::string name)
{
	auto warning = tr(presetNotFoundWarning).arg(name.c_str());
	ui->warning->setText(warning);
	adjustSize();
}

void ShrineWidget::on_custom_toggled(bool checked)
{
	changeComboBoxAllowedState();
}

void ShrineWidget::on_random_toggled(bool checked)
{
	changeComboBoxAllowedState();
}

ShrineDelegate::ShrineDelegate(MapController & controller, CRewardableObject & shrine) : BaseInspectorItemDelegate(), controller(controller), shrine(shrine) {}

QWidget * ShrineDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new ShrineWidget(shrine, controller, parent);
}

void ShrineDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if(auto * sw = qobject_cast<ShrineWidget *>(editor))
	{
		try
		{
			sw->loadData();
		}
		catch(const WidgetInitializationException & ex)
		{
			QWidget * parent = qobject_cast<QWidget *>(editor->parent());
			if(parent)
				QMessageBox::warning(parent, tr("Can't open editor!"), ex.message());
			destroyEditor(editor, index);
		}
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void ShrineDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if(auto * ed = qobject_cast<ShrineWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void ShrineDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList;
	JsonNode preset = shrine.configuration.getPresetVariable(ShrineWidget::PRESET_CATEGORY, ShrineWidget::PRESET_NAME);
	if(!preset.isNull())
		textList += QObject::tr(preset.String().c_str());
	else
		textList += QObject::tr("Random");
	setModelTextData(model, index, textList);
}
