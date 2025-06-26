/*
 * startingbonus.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "startingbonus.h"
#include "ui_startingbonus.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"

StartingBonus::StartingBonus(PlayerColor color, std::shared_ptr<CMap> map, CampaignBonus bonus):
	ui(new Ui::StartingBonus),
	bonus(bonus),
	map(map),
	color(color)
{
	ui->setupUi(this);

	setWindowTitle(tr("Edit Starting Bonus"));
	
	setWindowModality(Qt::ApplicationModal);

	ui->tabWidget->tabBar()->hide();
	ui->tabWidget->setStyleSheet("QTabWidget::pane { border: 0; }");

	initControls();

	loadBonus();

	show();
}

StartingBonus::~StartingBonus()
{
	delete ui;
}

void StartingBonus::initControls()
{
	std::map<int, std::string> heroSelection;
	for(ObjectInstanceID heroID : map->getHeroesOnMap())
	{
		const auto * hero = dynamic_cast<const CGHeroInstance*>(map->objects.at(heroID.getNum()).get());
		if(hero->getOwner() == color || color == PlayerColor::CANNOT_DETERMINE)
			heroSelection.emplace(hero->getHeroTypeID(), hero->getNameTranslated());
	}
	heroSelection.emplace(HeroTypeID::CAMP_STRONGEST, tr("Strongest").toStdString());
	heroSelection.emplace(HeroTypeID::CAMP_GENERATED, tr("Generated").toStdString());
	heroSelection.emplace(HeroTypeID::CAMP_RANDOM, tr("Random").toStdString());

	for(auto & hero : heroSelection)
	{
		ui->comboBoxSpellRecipient->addItem(QString::fromStdString(hero.second), QVariant(hero.first));
		ui->comboBoxCreatureRecipient->addItem(QString::fromStdString(hero.second), QVariant(hero.first));
		ui->comboBoxArtifactRecipient->addItem(QString::fromStdString(hero.second), QVariant(hero.first));
		ui->comboBoxSpellScrollRecipient->addItem(QString::fromStdString(hero.second), QVariant(hero.first));
		ui->comboBoxPrimarySkillRecipient->addItem(QString::fromStdString(hero.second), QVariant(hero.first));
		ui->comboBoxSecondarySkillRecipient->addItem(QString::fromStdString(hero.second), QVariant(hero.first));
	}

	for(auto const & objectPtr : LIBRARY->spellh->objects)
	{
		ui->comboBoxSpellSpell->addItem(QString::fromStdString(objectPtr->getNameTranslated()), QVariant(objectPtr->getId()));
		ui->comboBoxSpellScrollSpell->addItem(QString::fromStdString(objectPtr->getNameTranslated()), QVariant(objectPtr->getId()));
	}

	for(auto const & objectPtr : LIBRARY->creh->objects)
		ui->comboBoxCreatureCreatureType->addItem(QString::fromStdString(objectPtr->getNameSingularTranslated()), QVariant(objectPtr->getId()));
	
	if(map->players[color].hasMainTown)
	{
		for(ObjectInstanceID townID : map->getAllTowns())
		{
			const auto * town = dynamic_cast<const CGTownInstance*>(map->objects.at(townID.getNum()).get());

			if(town->pos == map->players[color].posOfMainTown)
			{
				for(auto & building : town->getTown()->buildings)
					ui->comboBoxBuildingBuilding->addItem(QString::fromStdString(building.second->getNameTranslated()), QVariant(building.first.getNum()));
				break;
			}
		}
	}

	for(auto const & objectPtr : LIBRARY->arth->objects)
		if(!vstd::contains(std::vector<ArtifactID>({
			ArtifactID::SPELL_SCROLL,
			ArtifactID::SPELLBOOK,
			ArtifactID::GRAIL,
			ArtifactID::AMMO_CART,
			ArtifactID::BALLISTA,
			ArtifactID::CATAPULT,
			ArtifactID::FIRST_AID_TENT,
		}), objectPtr->getId()))
			ui->comboBoxArtifactArtifact->addItem(QString::fromStdString(objectPtr->getNameTranslated()), QVariant(objectPtr->getId()));

	for(auto const & objectPtr : LIBRARY->skillh->objects)
		ui->comboBoxSecondarySkillSecondarySkill->addItem(QString::fromStdString(objectPtr->getNameTranslated()), QVariant(objectPtr->getId()));
	
	for(int i = 0; i < 3; i++) // Basic, Advanced, Expert
		ui->comboBoxSecondarySkillMastery->addItem(QString::fromStdString(LIBRARY->generaltexth->translate("core.skilllev", i)), QVariant(i));

	for(auto & res : std::vector<EGameResID>({EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS, EGameResID::GOLD}))
		ui->comboBoxResourceResourceType->addItem(QString::fromStdString(MetaString::createFromName(res).toString()), QVariant(res));
	ui->comboBoxResourceResourceType->addItem(
		tr("Common (%1 and %2)")
		.arg(QString::fromStdString(MetaString::createFromName(EGameResID::WOOD).toString()))
		.arg(QString::fromStdString(MetaString::createFromName(EGameResID::ORE).toString()))
		, QVariant(EGameResID::COMMON));
	ui->comboBoxResourceResourceType->addItem(
		tr("Rare (%1, %2, %3, %4)")
		.arg(QString::fromStdString(MetaString::createFromName(EGameResID::MERCURY).toString()))
		.arg(QString::fromStdString(MetaString::createFromName(EGameResID::SULFUR).toString()))
		.arg(QString::fromStdString(MetaString::createFromName(EGameResID::CRYSTAL).toString()))
		.arg(QString::fromStdString(MetaString::createFromName(EGameResID::GEMS).toString()))
		, QVariant(EGameResID::RARE));
}

void StartingBonus::loadBonus()
{
	auto setComboBoxValue = [](QComboBox * comboBox, QVariant data){
		int index = comboBox->findData(data);
		if(index != -1)
			comboBox->setCurrentIndex(index);
	};

	switch(bonus.getType())
	{
		case CampaignBonusType::SPELL:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusSpell>();
			ui->radioButtonSpell->setChecked(true);
			on_radioButtonSpell_toggled();
			setComboBoxValue(ui->comboBoxSpellRecipient, bonusValue.hero.getNum());
			setComboBoxValue(ui->comboBoxSpellSpell, bonusValue.spell.getNum());
			break;
		}
		case CampaignBonusType::MONSTER:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusCreatures>();
			ui->radioButtonCreature->setChecked(true);
			on_radioButtonCreature_toggled();
			setComboBoxValue(ui->comboBoxCreatureRecipient, bonusValue.hero.getNum());
			setComboBoxValue(ui->comboBoxCreatureCreatureType, bonusValue.creature.getNum());
			ui->spinBoxCreatureQuantity->setValue(bonusValue.amount);
			break;
		}
		case CampaignBonusType::BUILDING:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusBuilding>();
			ui->radioButtonBuilding->setChecked(true);
			on_radioButtonBuilding_toggled();
			setComboBoxValue(ui->comboBoxBuildingBuilding, bonusValue.buildingDecoded.getNum());
			break;
		}
		case CampaignBonusType::ARTIFACT:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusArtifact>();
			ui->radioButtonArtifact->setChecked(true);
			on_radioButtonArtifact_toggled();
			setComboBoxValue(ui->comboBoxArtifactRecipient, bonusValue.hero.getNum());
			setComboBoxValue(ui->comboBoxArtifactArtifact, bonusValue.artifact.getNum());
			break;
		}
		case CampaignBonusType::SPELL_SCROLL:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusSpellScroll>();
			ui->radioButtonSpellScroll->setChecked(true);
			on_radioButtonSpellScroll_toggled();
			setComboBoxValue(ui->comboBoxSpellScrollRecipient, bonusValue.hero.getNum());
			setComboBoxValue(ui->comboBoxSpellScrollSpell, bonusValue.spell.getNum());
			break;
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusPrimarySkill>();
			ui->radioButtonPrimarySkill->setChecked(true);
			on_radioButtonPrimarySkill_toggled();
			setComboBoxValue(ui->comboBoxPrimarySkillRecipient, bonusValue.hero.getNum());
			ui->spinBoxPrimarySkillAttack->setValue(bonusValue.amounts[0]);
			ui->spinBoxPrimarySkillDefense->setValue(bonusValue.amounts[0]);
			ui->spinBoxPrimarySkillSpell->setValue(bonusValue.amounts[0]);
			ui->spinBoxPrimarySkillKnowledge->setValue(bonusValue.amounts[0]);
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusSecondarySkill>();
			ui->radioButtonSecondarySkill->setChecked(true);
			on_radioButtonSecondarySkill_toggled();
			setComboBoxValue(ui->comboBoxSecondarySkillRecipient, bonusValue.hero.getNum());
			setComboBoxValue(ui->comboBoxSecondarySkillSecondarySkill, bonusValue.skill.getNum());
			setComboBoxValue(ui->comboBoxSecondarySkillMastery, bonusValue.mastery);
			break;
		}
		case CampaignBonusType::RESOURCE:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusStartingResources>();
			ui->radioButtonResource->setChecked(true);
			on_radioButtonResource_toggled();
			setComboBoxValue(ui->comboBoxResourceResourceType, bonusValue.resource.getNum());
			ui->spinBoxResourceQuantity->setValue(bonusValue.amount);
			break;
		}

		default:
			break;
	}
}

void StartingBonus::saveBonus()
{
	if(ui->radioButtonSpell->isChecked())
		bonus = CampaignBonusSpell{
			HeroTypeID(ui->comboBoxSpellRecipient->currentData().toInt()),
			SpellID(ui->comboBoxSpellSpell->currentData().toInt())
		};
	else if(ui->radioButtonCreature->isChecked())
		bonus = CampaignBonusCreatures{
			HeroTypeID(ui->comboBoxCreatureRecipient->currentData().toInt()),
			CreatureID(ui->comboBoxCreatureCreatureType->currentData().toInt()),
			int32_t(ui->spinBoxCreatureQuantity->value())
		};
	else if(ui->radioButtonBuilding->isChecked())
		bonus = CampaignBonusBuilding{
			BuildingID{},
			BuildingID(ui->comboBoxBuildingBuilding->currentData().toInt())
		};
	else if(ui->radioButtonArtifact->isChecked())
		bonus = CampaignBonusArtifact{
			HeroTypeID(ui->comboBoxCreatureRecipient->currentData().toInt()),
			ArtifactID(ui->comboBoxArtifactArtifact->currentData().toInt())
		};
	else if(ui->radioButtonSpellScroll->isChecked())
		bonus = CampaignBonusSpellScroll{
			HeroTypeID(ui->comboBoxCreatureRecipient->currentData().toInt()),
			SpellID(ui->comboBoxSpellScrollSpell->currentData().toInt())
		};
	else if(ui->radioButtonPrimarySkill->isChecked())
		bonus = CampaignBonusPrimarySkill{
			HeroTypeID(ui->comboBoxCreatureRecipient->currentData().toInt()),
			{
				uint8_t(ui->spinBoxPrimarySkillAttack->value()),
				uint8_t(ui->spinBoxPrimarySkillDefense->value()),
				uint8_t(ui->spinBoxPrimarySkillSpell->value()),
				uint8_t(ui->spinBoxPrimarySkillKnowledge->value()),
			}
		};
	else if(ui->radioButtonSecondarySkill->isChecked())
		bonus = CampaignBonusSecondarySkill{
			HeroTypeID(ui->comboBoxCreatureRecipient->currentData().toInt()),
			SecondarySkill(ui->comboBoxSecondarySkillSecondarySkill->currentData().toInt()),
			int32_t(ui->comboBoxSecondarySkillMastery->currentData().toInt())
		};
	else if(ui->radioButtonResource->isChecked())
		bonus = CampaignBonusStartingResources{
			GameResID(ui->comboBoxResourceResourceType->currentData().toInt()),
			int32_t(ui->spinBoxResourceQuantity->value())
		};
}

void StartingBonus::on_buttonBox_clicked(QAbstractButton * button)
{
	if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
	{
		saveBonus();
		accept();
	}
	close();
}

bool StartingBonus::showStartingBonus(PlayerColor color, std::shared_ptr<CMap> map, CampaignBonus & bonus)
{
	auto * dialog = new StartingBonus(color, map, bonus);

	dialog->setAttribute(Qt::WA_DeleteOnClose);

	auto result = dialog->exec();

	if(result == QDialog::Accepted)
		bonus = dialog->bonus;

	return result == QDialog::Accepted;
}

QString StartingBonus::getBonusListTitle(CampaignBonus bonus, std::shared_ptr<CMap> map)
{
	auto getHeroName = [map](int id){
		MetaString tmp;
		if(id == HeroTypeID::CAMP_STRONGEST)
			tmp.appendRawString(tr("strongest hero").toStdString());
		else if(id == HeroTypeID::CAMP_GENERATED)
			tmp.appendRawString(tr("generated hero").toStdString());
		else if(id == HeroTypeID::CAMP_RANDOM)
			tmp.appendRawString(tr("random hero").toStdString());
		else
		{
			for(auto o : map->objects)
				if(auto * ins = dynamic_cast<CGHeroInstance *>(o.get()))
					if(ins->getHeroTypeID().getNum() == id)
						tmp.appendTextID(ins->getNameTextID());
		}
		return QString::fromStdString(tmp.toString());
	};
	auto getSpellName = [](SpellID id){
		MetaString tmp;
		tmp.appendName(id);
		return QString::fromStdString(tmp.toString());
	};
	auto getMonsterName = [](CreatureID id, int amount){
		MetaString tmp;
		tmp.appendName(id, amount);
		return QString::fromStdString(tmp.toString());
	};
	auto getArtifactName = [](ArtifactID id){
		MetaString tmp;
		tmp.appendName(id);
		return QString::fromStdString(tmp.toString());
	};

	switch(bonus.getType())
	{
		case CampaignBonusType::SPELL:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusSpell>();
			return tr("%1 spell for %2").arg(getSpellName(bonusValue.spell)).arg(getHeroName(bonusValue.hero));
		}
		case CampaignBonusType::MONSTER:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusCreatures>();
			return tr("%1 %2 for %3").arg(bonusValue.amount).arg(getMonsterName(bonusValue.creature, bonusValue.amount)).arg(getHeroName(bonusValue.hero));
		}
		case CampaignBonusType::BUILDING:
		{
			return tr("Building");
		}
		case CampaignBonusType::ARTIFACT:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusArtifact>();
			return tr("%1 artifact for %2").arg(getArtifactName(bonusValue.artifact)).arg(getHeroName(bonusValue.hero));
		}
		case CampaignBonusType::SPELL_SCROLL:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusSpellScroll>();
			return tr("%1 spell scroll for %2").arg(getSpellName(bonusValue.spell)).arg(getHeroName(bonusValue.hero));
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			const auto & bonusValue = bonus.getValue<CampaignBonusPrimarySkill>();
			return tr("Primary skill (Attack: %1, Defense: %2, Spell: %3, Knowledge: %4) for %5")
				.arg(bonusValue.amounts[0])
				.arg(bonusValue.amounts[1])
				.arg(bonusValue.amounts[2])
				.arg(bonusValue.amounts[3])
				.arg(getHeroName(bonusValue.hero));
		}
		case CampaignBonusType::SECONDARY_SKILL:
		{
			return tr("Secondary skill");
		}
		case CampaignBonusType::RESOURCE:
		{
			return tr("Resource");
		}
	}
	return {};
}

void StartingBonus::on_radioButtonSpell_toggled()
{
	ui->tabWidget->setCurrentIndex(0);
}

void StartingBonus::on_radioButtonCreature_toggled()
{
	ui->tabWidget->setCurrentIndex(1);
}

void StartingBonus::on_radioButtonBuilding_toggled()
{
	ui->tabWidget->setCurrentIndex(2);
}

void StartingBonus::on_radioButtonArtifact_toggled()
{
	ui->tabWidget->setCurrentIndex(3);
}

void StartingBonus::on_radioButtonSpellScroll_toggled()
{
	ui->tabWidget->setCurrentIndex(4);
}

void StartingBonus::on_radioButtonPrimarySkill_toggled()
{
	ui->tabWidget->setCurrentIndex(5);
}

void StartingBonus::on_radioButtonSecondarySkill_toggled()
{
	ui->tabWidget->setCurrentIndex(6);
}

void StartingBonus::on_radioButtonResource_toggled()
{
	ui->tabWidget->setCurrentIndex(7);
}

