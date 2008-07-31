#include "../global.h"
struct IPack
{
	virtual ui16 getType()const = 0 ;
	//template<ui16 Type>
	//static bool isType(const IPack * ip)
	//{
	//	return Type == ip->getType();
	//}
	template<ui16 Type>
	static bool isType(IPack * ip)
	{
		return Type == ip->getType();
	}
	//template<ui16 Type>
	//static bool isType(const IPack & ip)
	//{
	//	return Type == ip.getType();
	//}
};
template <typename T> struct CPack
	:public IPack
{
	ui16 type; 
	ui16 getType() const{return type;}
	T* This(){return static_cast<T*>(this);};
};
struct NewTurn : public CPack<NewTurn> //101
{
	struct Hero
	{
		ui32 id, move, mana; //id is a general serial id
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & id & move & mana;
		}
		bool operator<(const Hero&h)const{return id < h.id;}
	};
	struct Resources
	{
		ui8 player;
		si32 resources[RESOURCE_QUANTITY];
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & player & resources;
		}
		bool operator<(const Resources&h)const{return player < h.player;}
	};

	std::set<Hero> heroes; //updates movement and mana points
	std::set<Resources> res;//resource list
	ui32 day;
	bool resetBuilded;

	NewTurn(){type = 101;};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroes & res & day & resetBuilded;
	}
}; 
struct SetResource : public CPack<SetResource> //102
{
	SetResource(){type = 102;};

	ui8 player, resid;
	si32 val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & resid & val;
	}
}; 
struct TryMoveHero : public CPack<TryMoveHero> //501
{
	TryMoveHero(){type = 501;};

	ui32 id, movePoints;
	ui8 result;
	int3 start, end;
	std::set<int3> fowRevealed; //revealed tiles

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & result & start & end & movePoints & fowRevealed;
	}
}; 
struct MetaString : public CPack<MetaString> //2001 helper for object scrips
{
	std::vector<std::string> strings;
	std::vector<std::pair<ui8,ui32> > texts; //pairs<text handler type, text number>; types: 1 - generaltexthandler->all; 2 - objh->xtrainfo; 3 - objh->names; 4 - objh->restypes; 5 - arth->artifacts[id].name; 6 - generaltexth->arraytxt; 7 - creh->creatures[os->subID].namePl; 8 - objh->creGens; 9 - objh->mines[ID].first; 10 - objh->mines[ID].second; 11 - objh->advobtxt
	std::vector<si32> message;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & strings & texts & message;
	}

	MetaString& operator<<(const std::pair<ui8,ui32> &txt)
	{
		message.push_back(-((si32)texts.size())-1);
		texts.push_back(txt);
		return *this;
	}
	MetaString& operator<<(const std::string &txt)
	{
		message.push_back(strings.size()+1);
		strings.push_back(txt);
		return *this;
	}
	void clear()
	{
		strings.clear();
		texts.clear();
		message.clear();
	}

	MetaString(){type = 2001;};
}; 
struct Component : public CPack<Component> //2002 helper for object scrips informations
{
	ui16 id, subtype; //ids: 0 - primskill; 1 - secskill; 2 - resource; 3 - creature; 4 - artifact; 5 - experience
	si32 val; // + give; - take
	si16 when; // 0 - now; +x - within x days; -x - per x days

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & subtype & val & when;
	}
	Component(){type = 2002;};
	Component(ui16 Type, ui16 Subtype, si32 Val, si16 When):id(Type),subtype(Subtype),val(Val),when(When){type = 2002;};
};

struct InfoWindow : public CPack<InfoWindow> //103  - displays simple info window
{
	MetaString text;
	std::vector<Component> components;
	ui8 player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text & components & player;
	}
	InfoWindow(){type = 103;};
};

struct SetObjectProperty : public CPack<SetObjectProperty>//1001
{
	ui32 id;
	ui8 what; //1 - owner; 2 - blockvis
	ui32 val;
	SetObjectProperty(){type = 1001;};
	SetObjectProperty(ui32 ID, ui8 What, ui32 Val):id(ID),what(What),val(Val){type = 1001;};
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & what & val;
	}
};

struct SetHoverName : public CPack<SetHoverName>//1002
{
	ui32 id;
	MetaString name;
	SetHoverName(){type = 1002;};
	SetHoverName(ui32 ID, MetaString& Name):id(ID),name(Name){type = 1002;};
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & name;
	}
};