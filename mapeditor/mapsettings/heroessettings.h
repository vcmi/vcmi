/*
 * heroessettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "abstractsettings.h"

namespace Ui
{
class HeroesSettings;
}

class HeroesSettings : public AbstractSettings
{
	Q_OBJECT

public:
	explicit HeroesSettings(QWidget * parent = nullptr);
	~HeroesSettings() override;

	void initialize(MapController & map) override;
	void update() override;

private slots:
	void on_textSearch_textChanged(const QString & keyword);
	void on_filterModeCombo_currentIndexChanged(int index);
	void on_heroList_itemSelectionChanged();
	void on_heroList_itemChanged(QListWidgetItem * item);
	void on_playerList_itemChanged(QListWidgetItem * item);

private:
	struct Filter
	{
		std::string description;
		std::function<bool(const QListWidgetItem *)> isVisible;
	};

	Ui::HeroesSettings * ui;
	std::vector<DisposedHero> disposedHeroesCopy;
	const std::vector<Filter> filters = {
		{QT_TR_NOOP("All heroes"), [](const QListWidgetItem * heroItem) -> bool { return true; }},
		{QT_TR_NOOP("Exclusive heroes"), [this](const QListWidgetItem * item) -> bool { return getDisposedHero(item->data(Qt::UserRole).toInt()).has_value(); }},
		{QT_TR_NOOP("Banned Heroes"), [](const QListWidgetItem * item) -> bool { return item->checkState() == Qt::Unchecked; }}
	};
	QGraphicsScene scene;
	QPixmap pixmap;

	void updatePlayersSelection();
	void showPlayerSelection(bool show);
	void filterHeroes();
	void drawPortrait();
	void setDisposedHero(int heroId, std::set<PlayerColor> players);
	void removeDisposedHero(int heroId);
	std::optional<const DisposedHero> getDisposedHero(int heroId);
	std::set<PlayerColor> getSelectedPlayers();
	int getSelectedHeroId();
	int getSelectedHeroImageIndex();
	bool allPlayersSelected();
	bool isHeroBanned(int heroId);
};
