/*
 * abilitieswidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "abilitieswidget.h"
#include "inspector.h"
#include "ui_abilitieswidget.h"
#include "../mapcontroller.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/rewardable/Info.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjectConstructors/IObjectInfo.h"

const std::string AbilitiesWidget::V_CATEGORY="secondarySkill";
const std::string AbilitiesWidget::V_NAME="gainedSkill";

AbilitiesWidget::AbilitiesWidget(CRewardableObject & hut, MapController & controller, QWidget * parent)
	: hut(hut), controller(controller), QDialog(parent), extractor(controller.getCallback()), ui(new Ui::AbilitiesWidget)
{
	ui->setupUi(this);
}

AbilitiesWidget::~AbilitiesWidget()
{
	delete ui;
}

void AbilitiesWidget::loadData()
{
	for(const auto & objectPtr : LIBRARY->skillh->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		int skillId = objectPtr->getIndex();
		item->setData(Qt::UserRole, QVariant::fromValue(skillId));
		ui->skills->addItem(item);
	}
	ui->customize->setChecked(!getPresetAbilities().empty());
	prepareDisplay();
}

void AbilitiesWidget::prepareDisplay()
{
	bool customized = !isSetToDefault();
	ui->defaultSkillsWarning->setHidden(customized);
	ui->selectAll->setDisabled(!customized);
	ui->deselectAll->setDisabled(!customized);

	if(customized)
	{
		std::set<SecondarySkill> presetAbilities = getPresetAbilities();
		if(presetAbilities.empty())
			presetAbilities = getDefaultAbilities();
		for(int i = 0; i < ui->skills->count(); ++i)
		{
			auto * item = ui->skills->item(i);
			int skillId = item->data(Qt::UserRole).toInt();
			bool isAllowed = controller.getMapUniquePtr()->allowedAbilities.contains(skillId);
				item->setFlags(isAllowed ? Qt::ItemIsEnabled | Qt::ItemIsUserCheckable : Qt::NoItemFlags);
				item->setCheckState(isAllowed && presetAbilities.contains(skillId) ? Qt::Checked : Qt::Unchecked);
		}
	}
	else
	{
		std::set<SecondarySkill> defaultAbilities = getDefaultAbilities();
		for(int i = 0; i < ui->skills->count(); ++i)
		{
			auto * item = ui->skills->item(i);
			int skillId = item->data(Qt::UserRole).toInt();
			item->setFlags(Qt::NoItemFlags);
			item->setCheckState(defaultAbilities.contains(skillId) ? Qt::Checked : Qt::Unchecked);
		}
	}
}

void AbilitiesWidget::commitChanges()
{
	if(isSetToDefault())
	{
		hut.configuration.variables.preset.clear();
		return;
	}

	std::set<SecondarySkill> presetAbilities;
	for(int i = 0; i < ui->skills->count(); ++i)
	{
		auto * item = ui->skills->item(i);
		if(item->checkState() == Qt::Checked)
		{
			int skillId = item->data(Qt::UserRole).toInt();
			presetAbilities.insert(SecondarySkill(skillId));
		}
	}

	JsonNode paNode;
	JsonVector anyOfList;
	for(const auto & skill : presetAbilities)
	{
		JsonNode entry;
		entry.String() = LIBRARY->skills()->getById(skill)->getJsonKey();
		anyOfList.push_back(entry);
	}
	paNode["anyOf"].Vector() = anyOfList;

	paNode.setModScope(ModScope::scopeGame()); // list may include skills from all mods
	hut.configuration.presetVariable(V_CATEGORY, V_NAME, paNode);
}

std::set<SecondarySkill> AbilitiesWidget::getPresetAbilities()
{
	std::set<SecondarySkill> presetAbilities;
	JsonNode preset = hut.configuration.getPresetVariable(V_CATEGORY, V_NAME);
	if(!preset.isNull())
		presetAbilities = extractor.filterKeys(preset, LIBRARY->skillh->getDefaultAllowed());
	return presetAbilities;
}

std::set<SecondarySkill> AbilitiesWidget::getDefaultAbilities()
{
	std::unique_ptr<IObjectInfo> objectInfo = LIBRARY->objtypeh->getHandlerFor(hut.ID, hut.subID)->getObjectInfo();
	JsonNode defaultParameters = static_cast<Rewardable::Info *>(objectInfo.get())->getParameters()["variables"][V_CATEGORY][V_NAME];
	if(defaultParameters.isNull())
		return controller.getMapUniquePtr()->allowedAbilities;
	else
		return extractor.filterKeys(defaultParameters, controller.getMapUniquePtr()->allowedAbilities);
}

void AbilitiesWidget::on_selectAll_clicked()
{
	changeCheckStateForAll(true);
}

void AbilitiesWidget::on_deselectAll_clicked()
{
	changeCheckStateForAll(false);
}

void AbilitiesWidget::on_filter_textChanged(QString text)
{
	for(int i = 0; i < ui->skills->count(); ++i)
	{
		auto * item = ui->skills->item(i);
		item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
	}
}

void AbilitiesWidget::on_customize_toggled(bool checked)
{
	prepareDisplay();
}

void AbilitiesWidget::changeCheckStateForAll(bool checked)
{
	for(int i = 0; i < ui->skills->count(); ++i)
	{
		auto * item = ui->skills->item(i);
		if(item->flags().testFlag(Qt::ItemIsUserCheckable))
			item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	}
}

bool AbilitiesWidget::isSetToDefault()
{
	return !ui->customize->isChecked();
}

AbilitiesDelegate::AbilitiesDelegate(MapController & controller, CRewardableObject & hut) : BaseInspectorItemDelegate(), controller(controller), hut(hut) {}

QWidget * AbilitiesDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new AbilitiesWidget(hut, controller, parent);
}

void AbilitiesDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	logGlobal->error("set editor data!");
	if(auto * aw = qobject_cast<AbilitiesWidget *>(editor))
	{
		aw->loadData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void AbilitiesDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if(auto * ed = qobject_cast<AbilitiesWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void AbilitiesDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList;
	JsonNode preset = hut.configuration.getPresetVariable(AbilitiesWidget::V_CATEGORY, AbilitiesWidget::V_NAME);
	if(preset.isNull())
		textList += QObject::tr("Default");
	else
		textList += QObject::tr("Custom");
	setModelTextData(model, index, textList);
}
