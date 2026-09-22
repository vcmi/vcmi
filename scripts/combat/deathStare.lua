local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Applies chance-based kills after matching attacks. Scripted equivalent of the DEATH_STARE bonus.
---
--- Parameters:
---  val       - chance for each creature of the bearer's stack to kill one, in percent
---  situation - when the ability applies: "melee", "ranged", "rangedDistancePenalty",
---              "rangedWallPenalty" or "rangedDistanceAndWallPenalty". A situation this script
---              does not know is left to whatever patches are stacked over it
---  spell     - spell used for immunity, animation and combat log; defaults to death stare

local SPELL = "core:deathStare"

--- Returns the ranged-penalty category of the attack.
local function situationOf(battle, unit, other, payload)
	if not payload.ranged then return "melee" end

	local distance = battle:hasDistancePenalty(unit, other)
	local wall = battle:hasWallPenalty(unit, other)

	if distance and wall then return "rangedDistanceAndWallPenalty" end
	if distance then return "rangedDistancePenalty" end
	if wall then return "rangedWallPenalty" end

	return "ranged"
end

--- Returns binomial kills capped by the expected eligible share of the bearer stack.
function Script:rolledKills(server, unit)
	local chance = self.val or 0

	if chance <= 0 then return 0 end

	local count = unit:getCount()
	local killed = server:rngBinomial(count, math.min(chance, 100) / 100)
	local cap = math.ceil(count * chance / 100)

	return math.min(killed, cap)
end

--- Returns kills for a matching attack category, or nil. Override to add categories.
function Script:killsIn(server, battle, unit, other, payload)
	if (self.situation or "melee") ~= situationOf(battle, unit, other, payload) then return nil end

	return self:rolledKills(server, unit)
end

function Script:onAfterAttack(server, battle, unit, other, payload)
	-- A dead bearer cannot apply the effect after retaliation or reflected damage.
	if not unit:isAlive() then return end
	if not other or not other:isAlive() then return end

	local killed = self:killsIn(server, battle, unit, other, payload)

	if not killed or killed <= 0 then return end

	-- Spell mechanics apply immunity and client animation.
	local spell = LIBRARY:getSpellByName(self.spell or SPELL)

	server:castSpell(battle, unit, spell, { other }, killed)
end

return Script
