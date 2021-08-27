local T =
{
	ARTIFACT = 1,
	ARTIFACT_INSTANCE = 2,
	CREATURE = 3,
	CREATURE_INSTANCE = 4,
	FACTION = 5,
	HERO_CLASS = 6,
	HERO_TYPE = 7,
	HERO_INSTANCE = 8,
	MAP_OBJECT_GROUP = 9,
	MAP_OBJECT_TYPE = 10,
	MAP_OBJECT_INSTANCE = 11,
	SKILL = 12,
	SPELL =	13
}

local M = setmetatable({},
{
	__newindex = function() error("Metatype table is immutable") end,
	__index = T
})

return M
