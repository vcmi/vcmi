/*
 * heroessettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/ModDescription.h"
#include "../mapcontroller.h"
#include "heroessettings.h"
#include "ui_heroessettings.h"

#include <entities/hero/CHeroHandler.h>

HeroesSettings::HeroesSettings(QWidget * parent) : AbstractSettings(parent), ui(new Ui::HeroesSettings)
{
	ui->setupUi(this);
	ui->heroPortrait->setScene(&scene);
}

HeroesSettings::~HeroesSettings()
{
	delete ui;
}

void HeroesSettings::initialize(MapController & c)
{
	AbstractSettings::initialize(c);

	for(const auto & objectPtr : LIBRARY->heroh->objects)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(objectPtr->getNameTranslated()));
		QVariantList data;
		data.push_back(QVariant::fromValue(objectPtr->getIndex()));
		data.push_back(QVariant::fromValue(objectPtr->imageIndex));
		item->setData(Qt::UserRole, data);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller->map()->allowedHeroes.count(objectPtr->getId()) ? Qt::Checked : Qt::Unchecked);
		ui->heroList->addItem(item);
	}

	for(auto color = PlayerColor(0); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		MetaString str;
		str.appendName(color);
		auto * item = new QListWidgetItem(QString::fromStdString(str.toString()));
		item->setData(Qt::UserRole, QVariant::fromValue(color.getNum()));
		ui->playerList->addItem(item);
	}

	for(auto & f : filters)
	{
		ui->filterModeCombo->addItem(tr(f.description.c_str()));
	}

	disposedHeroesCopy = controller->map()->disposedHeroes;
}

void HeroesSettings::update()
{
	controller->map()->allowedHeroes.clear();
	for(int i = 0; i < ui->heroList->count(); ++i)
	{
		auto * item = ui->heroList->item(i);
		if(item->checkState() == Qt::Checked)
			controller->map()->allowedHeroes.emplace(i);
	}

	controller->map()->disposedHeroes = disposedHeroesCopy;
}

void HeroesSettings::on_textSearch_textChanged(const QString & keyword)
{
	filterHeroes();
}

void HeroesSettings::on_filterModeCombo_currentIndexChanged(int index)
{
	filterHeroes();
}

void HeroesSettings::on_heroList_itemSelectionChanged()
{
	updatePlayersSelection();
	drawPortrait();
}

void HeroesSettings::on_heroList_itemChanged(QListWidgetItem * item)
{
	updatePlayersSelection();
	if(item->checkState() == Qt::Unchecked)
		removeDisposedHero(item->data(Qt::UserRole).toList().at(0).toInt());
}

void HeroesSettings::on_playerList_itemChanged(QListWidgetItem * item)
{
	if(allPlayersSelected())
		removeDisposedHero(getSelectedHeroId());
	else
		setDisposedHero(getSelectedHeroId(), getSelectedPlayers());
}

void HeroesSettings::updatePlayersSelection()
{
	ui->playerList->blockSignals(true);

	auto * currentItem = ui->heroList->currentItem();
	if(!currentItem || currentItem->checkState() == Qt::Unchecked)
	{
		showPlayerSelection(false);
		return;
	}

	showPlayerSelection(true);

	auto disposedHero = getDisposedHero(getSelectedHeroId());
	for(auto color = PlayerColor(0); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		auto * item = ui->playerList->item(color.num);
		bool isAllowedPlayer = controller->map()->players[color.num].canAnyonePlay();
		if(isAllowedPlayer)
		{
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState((!disposedHero || disposedHero->players.contains(color)) ? Qt::Checked : Qt::Unchecked);
		}
		else
		{
			item->setFlags(Qt::NoItemFlags);
			item->setCheckState(Qt::Checked);
		}
	}

	ui->playerList->blockSignals(false);
}

void HeroesSettings::showPlayerSelection(bool show)
{
	ui->playerList->setHidden(!show);
}

void HeroesSettings::filterHeroes()
{
	for(int i = 0; i < ui->heroList->count(); ++i)
	{
		QListWidgetItem * item = ui->heroList->item(i);

		auto textToMatch = ui->textSearch->text();
		bool modeCheck = filters[ui->filterModeCombo->currentIndex()].isVisible(item);
		bool textMatchCheck = item->text().contains(textToMatch, Qt::CaseInsensitive);
		item->setHidden(!modeCheck || !textMatchCheck);
	}

	if(!ui->heroList->currentItem() || ui->heroList->currentItem()->isHidden()) //clear player list if the selected hero has been hidden
	{
		ui->heroList->clearSelection();
		ui->heroList->setCurrentIndex(QModelIndex());
		updatePlayersSelection();
		drawPortrait();
	}
}

void HeroesSettings::drawPortrait()
{
	if(!ui->heroList->currentItem())
	{
		ui->heroPortrait->setHidden(true);
		return;
	}
	ui->heroPortrait->setHidden(false);

	Animation portraitAnimation(AnimationPath::builtin("PortraitsLarge").getOriginalName());
	portraitAnimation.load(getSelectedHeroImageIndex());
	pixmap = QPixmap::fromImage(*portraitAnimation.getImage(getSelectedHeroImageIndex()));
	scene.addPixmap(pixmap);
	ui->heroPortrait->fitInView(scene.itemsBoundingRect(), Qt::KeepAspectRatio);
}

void HeroesSettings::setDisposedHero(int heroId, std::set<PlayerColor> players)
{
	for(auto & disposedHero : disposedHeroesCopy)
	{
		if(disposedHero.heroId == heroId)
		{
			disposedHero.players = std::move(players);
			return;
		}
	}

	DisposedHero newHero; // portrait and name is used only in h3m; in vcmi format you can change those by adding custom heroes
	newHero.heroId = HeroTypeID(heroId);
	newHero.players = std::move(players);
	disposedHeroesCopy.push_back(newHero);
}

void HeroesSettings::removeDisposedHero(int heroId)
{
	auto position = std::find_if(
		disposedHeroesCopy.begin(),
		disposedHeroesCopy.end(),
		[&heroId](const DisposedHero & dh) -> bool
		{
			return dh.heroId == heroId;
		}
	);
	if(position != disposedHeroesCopy.end())
		disposedHeroesCopy.erase(position);
}

std::optional<const DisposedHero> HeroesSettings::getDisposedHero(int heroId)
{
	for(auto & disposedHero : disposedHeroesCopy)
	{
		if(disposedHero.heroId == heroId)
			return disposedHero;
	}
	return std::nullopt;
}

std::set<PlayerColor> HeroesSettings::getSelectedPlayers()
{
	std::set<PlayerColor> result;
	for(int i = 0; i < ui->playerList->count(); ++i)
	{
		QListWidgetItem * item = ui->playerList->item(i);
		if(item->checkState() == Qt::Checked)
			result.insert(PlayerColor(i));
	}
	return result;
}

int HeroesSettings::getSelectedHeroId()
{
	return ui->heroList->currentItem()->data(Qt::UserRole).toList().at(0).toInt();
}

int HeroesSettings::getSelectedHeroImageIndex()
{
	return ui->heroList->currentItem()->data(Qt::UserRole).toList().at(1).toInt();
}

bool HeroesSettings::allPlayersSelected()
{
	return getSelectedPlayers().size() == PlayerColor::PLAYER_LIMIT;
}

bool HeroesSettings::isHeroBanned(int heroId)
{
	return ui->heroList->item(heroId)->checkState() == Qt::Unchecked;
}
