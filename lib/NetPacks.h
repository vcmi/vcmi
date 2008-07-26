#include "../global.h"

struct NewTurn
{
	struct Hero
	{
		ui32 id, move, mana; //id is a general serial id
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h -= id -= move -= mana;
		}
	};
	struct Resources
	{
		ui8 player;
		si32 resources[RESOURCE_QUANTITY];
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h -= resources;
		}
	};

	std::set<Hero> heroes;
	std::set<Resources> res;



	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h += heroes += res;
	}
}; 