local Base = require("combat/combatScript")
local BattleLog = require("battleLog")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Burns whoever strikes its bearer in melee for a share of the damage that strike could have
--- dealt. Scripted equivalent of the FIRE_SHIELD bonus.
---
--- Parameters:
---  val - share of the reflected damage, in percent

local ANIMATION = "C05SPF0"
local SOUND = "FIRESHIE"
local SPELL = "core:fireShield"
-- spell schools report their json key unscoped, unlike creatures and spells
local FIRE_SCHOOL = "fire"

--- Whether fire damage can reach the attacker at all. Immunity is checked here rather than left to
--- the damage pipeline, because a fully immune attacker should not even produce a battle log entry.
function Script:isImmune(attacker)
	local function immuneBy(type, predicate)
		return attacker:getBonuses({type = type}):filter(function(bonus)
			return bonus:getSubtype() == FIRE_SCHOOL and (predicate == nil or predicate(bonus))
		end):size() > 0
	end

	return immuneBy("SPELL_SCHOOL_IMMUNITY")
		or immuneBy("NEGATIVE_EFFECTS_IMMUNITY")
		or immuneBy("SPELL_DAMAGE_REDUCTION", function(bonus) return bonus:getVal() >= 100 end)
end

--- The entry of the payload describing the hit this unit took.
function Script:ownEntry(unit, payload)
	for _, target in ipairs(payload.targets or {}) do
		if target.unit and target.unit:unitID() == unit:unitID() then
			return target
		end
	end

	return nil
end

function Script:onAfterAttacked(server, battle, unit, other, payload)
	if payload.ranged then return end
	if not other or not other:isAlive() then return end

	-- a clone is an illusion: it deals and reflects nothing
	if unit:isClone() then return end

	-- an area attack reaches units the attacker never closed with, and those do not burn it
	if not battle:isMeleeAttackPossible(other, unit) then return end
	if self:isImmune(other) then return end

	local entry = self:ownEntry(unit, payload)

	if entry == nil then return end

	-- a stack reflects at most what it could have absorbed, so a small stack burns less
	local reflectable = math.min(entry.healthBeforeAttack, entry.damageBeforeDefense)
	local spell = LIBRARY:getSpellByName(SPELL)
	local damage = spell:adjustDamage(battle, unit, other, math.floor(reflectable * (self.val or 0) / 100))

	if damage <= 0 then return end

	-- deferred so that the flames and the flinch of the burned attacker start on the same frame
	server:showBattleAnimation(battle, { { unit = unit } }, ANIMATION, SOUND, 1.0, true)

	local dealt, killed = server:damageUnit(battle, other, damage)

	BattleLog.spellDamage(server, battle, spell, other, dealt, killed)
end

return Script
