#pragma once

#include "../widgets/CArtifactHolder.h"
#include "CWindowObject.h"
#include "../../lib/FunctionList.h"

/*
 * CTradeWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class IMarket;
class CSlider;
class CTextBox;

class CTradeWindow : public CWindowObject, public CWindowWithArtifacts //base for markets and altar of sacrifice
{
public:
	enum EType
	{
		RESOURCE, PLAYER, ARTIFACT_TYPE, CREATURE, CREATURE_PLACEHOLDER, ARTIFACT_PLACEHOLDER, ARTIFACT_INSTANCE
	};
	class CTradeableItem : public CIntObject
	{
		CAnimImage * image;

		std::string getFilename();
		int getIndex();
	public:
		const CArtifactInstance *hlp; //holds ptr to artifact instance id type artifact
		EType type;
		int id;
		const int serial;
		const bool left;
		std::string subtitle; //empty if default

		void setType(EType newType);
		void setID(int newID);

		const CArtifactInstance *getArtInstance() const;
		void setArtInstance(const CArtifactInstance *art);

		CFunctionList<void()> callback;
		bool downSelection;

		void showAllAt(const Point &dstPos, const std::string &customSub, SDL_Surface * to);

		void clickRight(tribool down, bool previousState);
		void hover (bool on);
		void showAll(SDL_Surface * to);
		void clickLeft(tribool down, bool previousState);
		std::string getName(int number = -1) const;
		CTradeableItem(Point pos, EType Type, int ID, bool Left, int Serial);
	};

	const IMarket *market;
	const CGHeroInstance *hero;

	CArtifactsOfHero *arts;
	//all indexes: 1 = left, 0 = right
	std::vector<CTradeableItem*> items[2];
	CTradeableItem *hLeft, *hRight; //highlighted items (nullptr if no highlight)
	EType itemsType[2];

	EMarketMode::EMarketMode mode;//0 - res<->res; 1 - res<->plauer; 2 - buy artifact; 3 - sell artifact
	CButton *ok, *max, *deal;
	CSlider *slider; //for choosing amount to be exchanged
	bool readyToTrade;

	CTradeWindow(std::string bgName, const IMarket *Market, const CGHeroInstance *Hero, EMarketMode::EMarketMode Mode); //c

	void showAll(SDL_Surface * to);

	void initSubs(bool Left);
	void initTypes();
	void initItems(bool Left);
	std::vector<int> *getItemsIds(bool Left); //nullptr if default
	void getPositionsFor(std::vector<Rect> &poss, bool Left, EType type) const;
	void removeItems(const std::set<CTradeableItem *> &toRemove);
	void removeItem(CTradeableItem * t);
	void getEmptySlots(std::set<CTradeableItem *> &toRemove);
	void setMode(EMarketMode::EMarketMode Mode); //mode setter

	void artifactSelected(CArtPlace *slot); //used when selling artifacts -> called when user clicked on artifact slot

	virtual void getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const = 0;
	virtual void selectionChanged(bool side) = 0; //true == left
	virtual Point selectionOffset(bool Left) const = 0;
	virtual std::string selectionSubtitle(bool Left) const = 0;
	virtual void garrisonChanged() = 0;
	virtual void artifactsChanged(bool left) = 0;
};

class CMarketplaceWindow : public CTradeWindow
{
	bool printButtonFor(EMarketMode::EMarketMode M) const;

	std::string getBackgroundForMode(EMarketMode::EMarketMode mode);
public:
	int r1, r2; //suggested amounts of traded resources
	bool madeTransaction; //if player made at least one transaction
	CTextBox *traderText;

	void setMax();
	void sliderMoved(int to);
	void makeDeal();
	void selectionChanged(bool side); //true == left
	CMarketplaceWindow(const IMarket *Market, const CGHeroInstance *Hero = nullptr, EMarketMode::EMarketMode Mode = EMarketMode::RESOURCE_RESOURCE); //c-tor
	~CMarketplaceWindow(); //d-tor

	Point selectionOffset(bool Left) const;
	std::string selectionSubtitle(bool Left) const;


	void garrisonChanged(); //removes creatures with count 0 from the list (apparently whole stack has been sold)
	void artifactsChanged(bool left);
	void resourceChanged(int type, int val);

	void getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const;
	void updateTraderText();
};

class CAltarWindow : public CTradeWindow
{
	CAnimImage * artIcon;
public:
	CAltarWindow(const IMarket *Market, const CGHeroInstance *Hero, EMarketMode::EMarketMode Mode); //c-tor

	void getExpValues();
	~CAltarWindow(); //d-tor

	std::vector<int> sacrificedUnits, //[slot_nr] -> how many creatures from that slot will be sacrificed
		expPerUnit;

	CButton *sacrificeAll, *sacrificeBackpack;
	CLabel *expToLevel, *expOnAltar;


	void selectionChanged(bool side); //true == left
	void SacrificeAll();
	void SacrificeBackpack();

	void putOnAltar(int backpackIndex);
	bool putOnAltar(CTradeableItem* altarSlot, const CArtifactInstance *art);
	void makeDeal();
	void showAll(SDL_Surface * to);

	void blockTrade();
	void sliderMoved(int to);
	void getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const;
	void mimicCres();

	Point selectionOffset(bool Left) const;
	std::string selectionSubtitle(bool Left) const;
	void garrisonChanged();
	void artifactsChanged(bool left);
	void calcTotalExp();
	void setExpToLevel();
	void updateRight(CTradeableItem *toUpdate);

	void artifactPicked();
	int firstFreeSlot();
	void moveFromSlotToAltar(ArtifactPosition slotID, CTradeableItem* altarSlot, const CArtifactInstance *art);
};
