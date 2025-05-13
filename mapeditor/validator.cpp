/*
 * validator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "validator.h"
#include "mapcontroller.h"
#include "ui_validator.h"
#include "../lib/entities/hero/CHero.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapObjects/MapObjects.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModDescription.h"
#include "../lib/spells/CSpellHandler.h"

Validator::Validator(const CMap * map, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::Validator)
{
	ui->setupUi(this);

	screenGeometry = QApplication::primaryScreen()->availableGeometry();
	
	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	
	showValidationResults(map);
}

Validator::~Validator()
{
	delete ui;
}

std::set<Validator::Issue> Validator::validate(const CMap * map)
{
	std::set<Validator::Issue> issues;
	
	if(!map)
	{
		issues.insert({ tr("Map is not loaded"), true });
		return issues;
	}
	
	try
	{
		//check player settings
		int hplayers = 0;
		int cplayers = 0;
		std::map<PlayerColor, int> amountOfTowns;
		std::map<PlayerColor, int> amountOfHeroes;

		for(int i = 0; i < map->players.size(); ++i)
		{
			auto & p = map->players[i];
			if (p.canAnyonePlay())
				amountOfTowns[PlayerColor(i)] = 0;
			if(p.canComputerPlay)
				++cplayers;
			if(p.canHumanPlay)
				++hplayers;
			if(p.allowedFactions.empty())
				issues.insert({ tr("No factions allowed for player %1").arg(i), true });
		}
		if(hplayers + cplayers == 0)
			issues.insert({ tr("No players allowed to play this map"), true });
		if(hplayers + cplayers == 1)
			issues.insert({ tr("Map is allowed for one player and cannot be started"), true });
		if(!hplayers)
			issues.insert({ tr("No human players allowed to play this map"), true });

		std::set<const CHero * > allHeroesOnMap; //used to find hero duplicated
		
		//checking all objects in the map
		for(auto o : map->objects)
		{
			//owners for objects
			if(o->getOwner() == PlayerColor::UNFLAGGABLE)
			{
				if(o->asOwnable())
				{
					issues.insert({ tr("Ownable object %1 is UNFLAGGABLE but must have NEUTRAL or player owner").arg(o->instanceName.c_str()), true });
				}
			}
			if(o->getOwner() != PlayerColor::NEUTRAL && o->getOwner().getNum() < map->players.size())
			{
				if(!map->players[o->getOwner().getNum()].canAnyonePlay())
					issues.insert({ tr("Object %1 is assigned to non-playable player %2").arg(o->instanceName.c_str(), o->getOwner().toString().c_str()), true });
			}
			//count towns
			if(auto * ins = dynamic_cast<CGTownInstance *>(o.get()))
			{
					++amountOfTowns[ins->getOwner()];
			}
			//checking and counting heroes and prisons
			if(auto * ins = dynamic_cast<CGHeroInstance *>(o.get()))
			{
				if(ins->ID == Obj::PRISON)
				{
					if(ins->getOwner() != PlayerColor::NEUTRAL)
						issues.insert({ tr("Prison %1 must be a NEUTRAL").arg(ins->instanceName.c_str()), true });
				}
				else
				{
					if(ins->getOwner() == PlayerColor::NEUTRAL)
						issues.insert({ tr("Hero %1 must have an owner").arg(ins->instanceName.c_str()), true });

					++amountOfHeroes[ins->getOwner()];
				}
				if(ins->getHeroTypeID().hasValue())
				{
					if(map->allowedHeroes.count(ins->getHeroTypeID()) == 0)
						issues.insert({ tr("Hero %1 is prohibited by map settings").arg(ins->getHeroType()->getNameTranslated().c_str()), false });
					
					if(!allHeroesOnMap.insert(ins->getHeroType()).second)
						issues.insert({ tr("Hero %1 has duplicate on map").arg(ins->getHeroType()->getNameTranslated().c_str()), false });
				}
				else if(ins->ID != Obj::RANDOM_HERO)
					issues.insert({ tr("Hero %1 has an empty type and must be removed").arg(ins->instanceName.c_str()), true });
			}
			
			//checking for arts
			if(auto * ins = dynamic_cast<CGArtifact *>(o.get()))
			{
				if(ins->ID == Obj::SPELL_SCROLL)
				{
					if (ins->getArtifactInstance())
					{
						if (map->allowedSpells.count(ins->getArtifactInstance()->getScrollSpellID()) == 0)
							issues.insert({ tr("Spell scroll %1 is prohibited by map settings").arg(ins->getArtifactInstance()->getScrollSpellID().toEntity(LIBRARY->spells())->getNameTranslated().c_str()), false });
					}
					else
						issues.insert({ tr("Spell scroll %1 doesn't have instance assigned and must be removed").arg(ins->instanceName.c_str()), true });
				}
				else
				{
					if(ins->ID == Obj::ARTIFACT && map->allowedArtifact.count(ins->getArtifactType()) == 0)
					{
						issues.insert({ tr("Artifact %1 is prohibited by map settings").arg(ins->getObjectName().c_str()), false });
					}
				}
			}
		}

		//verification of starting towns
		for (const auto & [player, counter] : amountOfTowns)
		{
			if (counter == 0)
			{
				// FIXME: heroesNames are empty even though heroes are on the map
				// if(map->players[playerTCounter.first].heroesNames.empty())
				if(amountOfHeroes.count(player) == 0)
					issues.insert({ tr("Player %1 has no towns and heroes assigned").arg(player + 1), true });
				else
					issues.insert({ tr("Player %1 doesn't have any starting town").arg(player + 1), false });
			}
		}

		//verification of map name and description
		if(map->name.empty())
			issues.insert({ tr("Map name is not specified"), false });
		if(map->description.empty())
			issues.insert({ tr("Map description is not specified"), false });
		
		//verification for mods
		for(auto & mod : MapController::modAssessmentMap(*map))
		{
			if(!map->mods.count(mod.first))
				issues.insert({ MapController::modMissingMessage(mod.second), true });
		}
	}
	catch(const std::exception & e)
	{
		issues.insert({ tr("Exception occurs during validation: %1").arg(e.what()), true });
	}
	catch(...)
	{
		issues.insert({ tr("Unknown exception occurs during validation"), true });
	}
	
	return issues;
}

void Validator::showValidationResults(const CMap * map)
{
	show();
	setAttribute(Qt::WA_DeleteOnClose);
	ui->listWidget->setItemDelegate(new ValidatorItemDelegate(ui->listWidget));

	for(auto const & issue : Validator::validate(map))
	{
		auto * item = new QListWidgetItem(QIcon(issue.critical ? ":/icons/mod-delete.png" : ":/icons/mod-update.png"),
			issue.message, ui->listWidget);

		ui->listWidget->addItem(item);
	}

	if(ui->listWidget->count() == 0)
	{
		QPixmap greenTick(":/icons/mod-enabled.png");
		QString validMessage = tr("The map is valid and has no issues.");
		auto * item = new QListWidgetItem(QIcon(greenTick), validMessage, ui->listWidget);
		ui->listWidget->addItem(item);
	}

	ui->listWidget->updateGeometry();

	adjustWindowSize();
}

void Validator::adjustWindowSize()
{
	const int minWidth = 350;
	const int minHeight = 50;
	const int padding = 30;		// reserved space for eventual scrollbars
	const int screenMarginVertical = 300;
	const int screenMarginHorizontal = 350;

	int contentHeight = minHeight;
	int contentWidth = minWidth;

	QStyleOptionViewItem option;
	option.initFrom(ui->listWidget);

	int listWidgetWidth = ui->listWidget->viewport()->width();

	for(int i = 0; i < ui->listWidget->count(); ++i)
	{
		option.rect = QRect(0, 0, listWidgetWidth, 0);
		auto itemSize = ui->listWidget->itemDelegate()->sizeHint(option, ui->listWidget->model()->index(i, 0));
		contentHeight += itemSize.height();
		contentWidth = qMax(contentWidth, itemSize.width());
	}

	int screenWidth = screenGeometry.width();
	int screenHeight = screenGeometry.height();

	int finalWidth = qMin(contentWidth + padding, screenWidth - screenMarginHorizontal);
	int finalHeight = qMin(contentHeight + padding, screenHeight - screenMarginVertical);

	QWidget * parentWidget = ui->listWidget->parentWidget();
	if(parentWidget)
	{
		parentWidget->setMinimumWidth(finalWidth + padding);
		parentWidget->setMinimumHeight(finalHeight + padding);
	}

	ui->listWidget->resize(finalWidth, finalHeight);

	move((screenWidth - finalWidth) / 2, (screenHeight - finalHeight) / 2);
}

void ValidatorItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	painter->save();

	QStyleOptionViewItem opt(option);
	QFontMetrics metrics(option.fontMetrics);
	initStyleOption(&opt, index);

	const QRect iconRect = option.rect.adjusted(iconPadding, iconPadding, 0, 0);

	const QRect textRect = option.rect.adjusted(offsetForIcon, 0, -textPaddingRight, 0);

	if(!opt.icon.isNull())
	{
		opt.icon.paint(painter, iconRect, Qt::AlignTop | Qt::AlignLeft);
	}

	QTextOption textOption;

	int textWidth = metrics.horizontalAdvance(opt.text);
	if(textWidth + offsetForIcon + textPaddingRight > screenGeometry.width() - screenMargin)
	{
		textOption.setWrapMode(QTextOption::WordWrap);
	}

	painter->drawText(textRect, opt.text, textOption);

	painter->restore();
}

QSize ValidatorItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QFontMetrics metrics(option.fontMetrics);
	QString text = index.data(Qt::DisplayRole).toString();
	QStringList lines = text.split('\n');
	int textWidth = minItemWidth;
	int requiredHeight = 0;
	for(auto line : lines)
		textWidth = std::max(metrics.horizontalAdvance(line), textWidth);

	requiredHeight = qMax(requiredHeight, lines.size() * metrics.height());

	int finalWidth = qMax(textWidth + offsetForIcon, minItemWidth);
	finalWidth = qMin(finalWidth, screenGeometry.width() - screenMargin - offsetForIcon);

	QRect textBoundingRect = metrics.boundingRect(QRect(0, 0, finalWidth, 0),
		Qt::TextWordWrap, text);

	int finalHeight = qMax(textBoundingRect.height() + itemPaddingBottom, requiredHeight);

	return QSize(finalWidth, finalHeight);
}
