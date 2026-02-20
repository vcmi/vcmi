/*
 * entitiesselector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "entitiesselector.h"
#include "ui_entitiesselector.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/hero/CHeroHandler.h"

template<typename T, typename Variant>
size_t countInVariantSet(const Variant& var, const T& val) {
    return std::visit([&val](const auto& setRefWrapper) -> size_t {
        const auto& setRef = setRefWrapper.get();
        using SetType = std::decay_t<decltype(setRef)>;
        using ValueType = typename SetType::value_type;
        if constexpr (std::is_same_v<ValueType, T>) {
            return setRef.count(val);
        } else {
            return 0;
        }
    }, var);
}

template<typename HandlerType, typename VariantSet>
void fillListWidgetFromHandler(QListWidget* listWidget, const HandlerType* handler, const VariantSet& entities)
{
    listWidget->clear();

    for (auto const& objectPtr : handler->objects)
    {
        auto* item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
        item->setData(Qt::UserRole, QVariant::fromValue(objectPtr->getIndex()));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(countInVariantSet(entities, objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
        listWidget->addItem(item);
    }
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using HandlerVariant = std::variant<
    const TerrainTypeHandler*,
    const CSpellHandler*,
    const CArtHandler*,
    const CSkillHandler*,
    const CHeroHandler*
>;

EntitiesSelector::EntitiesSelector(EntityIds & entities) :
	ui(new Ui::EntitiesSelector),
	entitiesSelected(entities)
{
	ui->setupUi(this);
	
	setWindowModality(Qt::ApplicationModal);

	HandlerVariant handler;
	std::visit(overloaded{
		[&](std::reference_wrapper<std::set<TerrainId>> terrainSet) {
			handler = LIBRARY->terrainTypeHandler.get();
			setWindowTitle(tr("Terrain Selector"));
		},
		[&](std::reference_wrapper<std::set<SpellID>> spellSet) {
			handler = LIBRARY->spellh.get();
			setWindowTitle(tr("Spell Selector"));
		},
		[&](std::reference_wrapper<std::set<ArtifactID>> artifactSet) {
			handler = LIBRARY->arth.get();
			setWindowTitle(tr("Artifact Selector"));
		},
		[&](std::reference_wrapper<std::set<SecondarySkill>> secondarySkillSet) {
			handler = LIBRARY->skillh.get();
			setWindowTitle(tr("Skill Selector"));
		},
		[&](std::reference_wrapper<std::set<HeroTypeID>> heroTypeSet) {
			handler = LIBRARY->heroh.get();
			setWindowTitle(tr("Hero Type Selector"));
		}
	}, entitiesSelected);
	std::visit([&](auto const* handlerPtr){
		fillListWidgetFromHandler(ui->listWidgetEntities, handlerPtr, entities);
	}, handler);

	show();
}

void EntitiesSelector::showEntitiesSelector(EntityIds & entities)
{
	auto * dialog = new EntitiesSelector(entities);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->exec();
}

void EntitiesSelector::on_buttonBoxResult_accepted()
{
	std::visit([](auto& setRefWrapper) {
		setRefWrapper.get().clear();
	}, entitiesSelected);
	for(int i = 0; i < ui->listWidgetEntities->count(); ++i)
	{
		auto * item = ui->listWidgetEntities->item(i);
		if(item->checkState() == Qt::Checked)
		{
			int id = item->data(Qt::UserRole).toInt();

			std::visit([id](auto& setRefWrapper) {
				using SetType = std::decay_t<decltype(setRefWrapper.get())>;
				using ValueType = typename SetType::value_type;
				ValueType value{id};  
				setRefWrapper.get().insert(std::move(value));
			}, entitiesSelected);
		}
	}

	close();
}

void EntitiesSelector::on_buttonBoxResult_rejected()
{
	close();
}
