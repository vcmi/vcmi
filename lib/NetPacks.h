#include "../global.h"
struct IPack
{
	virtual ui16 getType()=0;
};
template <typename T> struct CPack
	:public IPack
{
	ui16 type; 
	ui16 getType(){return type;}
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