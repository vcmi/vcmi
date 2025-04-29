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

	switch (bonus.type)
	{
	case CampaignBonusType::SPELL:
		ui->radioButtonSpell->setChecked(true);
		on_radioButtonSpell_toggled();
		setComboBoxValue(ui->comboBoxSpellRecipient, bonus.info1);
		setComboBoxValue(ui->comboBoxSpellSpell, bonus.info2);
		break;
	case CampaignBonusType::MONSTER:
		ui->radioButtonCreature->setChecked(true);
		on_radioButtonCreature_toggled();
		setComboBoxValue(ui->comboBoxCreatureRecipient, bonus.info1);
		setComboBoxValue(ui->comboBoxCreatureCreatureType, bonus.info2);
		ui->spinBoxCreatureQuantity->setValue(bonus.info3);
		break;
	case CampaignBonusType::BUILDING:
		ui->radioButtonBuilding->setChecked(true);
		on_radioButtonBuilding_toggled();
		setComboBoxValue(ui->comboBoxBuildingBuilding, bonus.info1);
		break;
	case CampaignBonusType::ARTIFACT:
		ui->radioButtonArtifact->setChecked(true);
		on_radioButtonArtifact_toggled();
		setComboBoxValue(ui->comboBoxArtifactRecipient, bonus.info1);
		setComboBoxValue(ui->comboBoxArtifactArtifact, bonus.info2);
		break;
	case CampaignBonusType::SPELL_SCROLL:
		ui->radioButtonSpellScroll->setChecked(true);
		on_radioButtonSpellScroll_toggled();
		setComboBoxValue(ui->comboBoxSpellScrollRecipient, bonus.info1);
		setComboBoxValue(ui->comboBoxSpellScrollSpell, bonus.info2);
		break;
	case CampaignBonusType::PRIMARY_SKILL:
		ui->radioButtonPrimarySkill->setChecked(true);
		on_radioButtonPrimarySkill_toggled();
		setComboBoxValue(ui->comboBoxPrimarySkillRecipient, bonus.info1);
		ui->spinBoxPrimarySkillAttack->setValue((bonus.info2 >> 0) & 0xff);
		ui->spinBoxPrimarySkillDefense->setValue((bonus.info2 >> 8) & 0xff);
		ui->spinBoxPrimarySkillSpell->setValue((bonus.info2 >> 16) & 0xff);
		ui->spinBoxPrimarySkillKnowledge->setValue((bonus.info2 >> 24) & 0xff);
		break;
	case CampaignBonusType::SECONDARY_SKILL:
		ui->radioButtonSecondarySkill->setChecked(true);
		on_radioButtonSecondarySkill_toggled();
		setComboBoxValue(ui->comboBoxSecondarySkillRecipient, bonus.info1);
		setComboBoxValue(ui->comboBoxSecondarySkillSecondarySkill, bonus.info2);
		setComboBoxValue(ui->comboBoxSecondarySkillMastery, bonus.info3);
		break;
	case CampaignBonusType::RESOURCE:
		ui->radioButtonResource->setChecked(true);
		on_radioButtonResource_toggled();
		setComboBoxValue(ui->comboBoxResourceResourceType, bonus.info1);
		ui->spinBoxResourceQuantity->setValue(bonus.info2);
		break;
	
	default:
		break;
	}
}

void StartingBonus::saveBonus()
{
	if(ui->radioButtonSpell->isChecked())
		bonus.type = CampaignBonusType::SPELL;
	else if(ui->radioButtonCreature->isChecked())
		bonus.type = CampaignBonusType::MONSTER;
	else if(ui->radioButtonBuilding->isChecked())
		bonus.type = CampaignBonusType::BUILDING;
	else if(ui->radioButtonArtifact->isChecked())
		bonus.type = CampaignBonusType::ARTIFACT;
	else if(ui->radioButtonSpellScroll->isChecked())
		bonus.type = CampaignBonusType::SPELL_SCROLL;
	else if(ui->radioButtonPrimarySkill->isChecked())
		bonus.type = CampaignBonusType::PRIMARY_SKILL;
	else if(ui->radioButtonSecondarySkill->isChecked())
		bonus.type = CampaignBonusType::SECONDARY_SKILL;
	else if(ui->radioButtonResource->isChecked())
		bonus.type = CampaignBonusType::RESOURCE;

	bonus.info1 = 0;
	bonus.info2 = 0;
	bonus.info3 = 0;

	switch (bonus.type)
	{
	case CampaignBonusType::SPELL:
		bonus.info1 = ui->comboBoxSpellRecipient->currentData().toInt();
		bonus.info2 = ui->comboBoxSpellSpell->currentData().toInt();
		break;
	case CampaignBonusType::MONSTER:
		bonus.info1 = ui->comboBoxCreatureRecipient->currentData().toInt();
		bonus.info2 = ui->comboBoxCreatureCreatureType->currentData().toInt();
		bonus.info3 = ui->spinBoxCreatureQuantity->value();
		break;
	case CampaignBonusType::BUILDING:
		bonus.info1 = ui->comboBoxBuildingBuilding->currentData().toInt();
		break;
	case CampaignBonusType::ARTIFACT:
		bonus.info1 = ui->comboBoxArtifactRecipient->currentData().toInt();
		bonus.info2 = ui->comboBoxArtifactArtifact->currentData().toInt();
		break;
	case CampaignBonusType::SPELL_SCROLL:
		bonus.info1 = ui->comboBoxSpellScrollRecipient->currentData().toInt();
		bonus.info2 = ui->comboBoxSpellScrollSpell->currentData().toInt();
		break;
	case CampaignBonusType::PRIMARY_SKILL:
		bonus.info1 = ui->comboBoxPrimarySkillRecipient->currentData().toInt();
		bonus.info2 |= ui->spinBoxPrimarySkillAttack->value() << 0;
		bonus.info2 |= ui->spinBoxPrimarySkillDefense->value() << 8;
		bonus.info2 |= ui->spinBoxPrimarySkillSpell->value() << 16;
		bonus.info2 |= ui->spinBoxPrimarySkillKnowledge->value() << 24;
		break;
	case CampaignBonusType::SECONDARY_SKILL:
		bonus.info1 = ui->comboBoxSecondarySkillRecipient->currentData().toInt();
		bonus.info2 = ui->comboBoxSecondarySkillSecondarySkill->currentData().toInt();
		bonus.info3 = ui->comboBoxSecondarySkillMastery->currentData().toInt();
		break;
	case CampaignBonusType::RESOURCE:
		bonus.info1 = ui->comboBoxResourceResourceType->currentData().toInt();
		bonus.info2 = ui->spinBoxResourceQuantity->value();
		break;
	
	default:
		break;
	}
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
	auto getSpellName = [](int id){
		MetaString tmp;
		tmp.appendName(SpellID(id));
		return QString::fromStdString(tmp.toString());
	};
	auto getMonsterName = [](int id, int amount){
		MetaString tmp;
		tmp.appendName(CreatureID(id), amount);
		return QString::fromStdString(tmp.toString());
	};
	auto getArtifactName = [](int id){
		MetaString tmp;
		tmp.appendName(ArtifactID(id));
		return QString::fromStdString(tmp.toString());
	};

	switch (bonus.type)
	{
	case CampaignBonusType::SPELL:
		return tr("%1 spell for %2").arg(getSpellName(bonus.info2)).arg(getHeroName(bonus.info1));
	case CampaignBonusType::MONSTER:
		return tr("%1 %2 for %3").arg(bonus.info3).arg(getMonsterName(bonus.info2, bonus.info3)).arg(getHeroName(bonus.info1));
	case CampaignBonusType::BUILDING:
		return tr("Building");
	case CampaignBonusType::ARTIFACT:
		return tr("%1 artifact for %2").arg(getArtifactName(bonus.info2)).arg(getHeroName(bonus.info1));
	case CampaignBonusType::SPELL_SCROLL:
		return tr("%1 spell scroll for %2").arg(getSpellName(bonus.info2)).arg(getHeroName(bonus.info1));
	case CampaignBonusType::PRIMARY_SKILL:
		return tr("Primary skill (Attack: %1, Defense: %2, Spell: %3, Knowledge: %4) for %5").arg((bonus.info2 >> 0) & 0xff).arg((bonus.info2 >> 8) & 0xff).arg((bonus.info2 >> 16) & 0xff).arg((bonus.info2 >> 24) & 0xff).arg(getHeroName(bonus.info1));
	case CampaignBonusType::SECONDARY_SKILL:
		return tr("Secondary skill");
	case CampaignBonusType::RESOURCE:
		return tr("Resource");
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

