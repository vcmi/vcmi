local Base = require("spellEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local function deepCopyBonus(b)
	local copy = {}
	for k, v in pairs(b) do
		copy[k] = v
	end
	return copy
end

function Script:apply(mechanics, server, target)
	if not mechanics:isMassive() then return end

	local battle = mechanics:getBattle()
	if not battle:hasMoat() then return end

	local spell    = mechanics:getSpell()
	local defender = self.defender or {}

	local obstacleType
	if self.dispellable then
		obstacleType = ENUM.ObstacleType.spellCreated
	else
		obstacleType = ENUM.ObstacleType.moat
	end

	for _, patch in ipairs(self.moatHexes or {}) do
	    server:addObstacle(battle, {
			pos              = patch[1],
			obstacleType     = obstacleType,
			spell            = spell,
			casterSpellPower = mechanics:getEffectPower(),
			spellLevel       = mechanics:getEffectLevel(),
			casterSide       = ENUM.BattleSide.defender,
			turnsRemaining   = -1,
			hidden           = self.hidden or false,
			passable         = true,
			nativeVisible    = false,
			trap             = self.trap or false,
			removeOnTrigger  = self.removeOnTrigger or false,
			trigger          = self.triggerAbility or "",
			minimalDamage    = self.moatDamage or 0,
			appearSound      = defender.appearSound or "",
			appearAnimation  = defender.appearAnimation or "",
			animation        = defender.animation or "",
			customSize       = patch,
		})
	end

	local flatHexes = {}
	for _, patch in ipairs(self.moatHexes or {}) do
		for _, hex in ipairs(patch) do
			flatHexes[#flatHexes+1] = hex
		end
	end

	for _, b in pairs(self.bonus or {}) do
		local nb = deepCopyBonus(b)
		nb.duration   = "ONE_BATTLE"
		nb.sourceType = "SPELL_EFFECT"
		nb.sourceID   = spell:getJsonKey()
		nb.limiters   = { type = "UNIT_ON_HEXES", hexes = flatHexes }
		server:addBattleBonus(battle, nb)
	end
end

return Script
