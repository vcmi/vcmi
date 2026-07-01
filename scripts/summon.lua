local Base = require("spellEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:summonedEffectValue(mechanics)
	local effectPower = mechanics:getEffectPower()
	local rawEffectPower = mechanics:calculateRawEffectValue(0, effectPower)
	local finalEffectPower = mechanics:applySpecificSpellBonus(rawEffectPower)

	return finalEffectPower
end

function Script:summonedCreatureHealth(mechanics)
	local valueWithBonus = self:summonedEffectValue(mechanics)
	local creature = LIBRARY:getCreatureByName(self.id)
	if self.summonByHealth then
		return valueWithBonus
	else
		return valueWithBonus * creature:getMaxHealth()
	end
end

function Script:summonedCreatureAmount(mechanics)
	local valueWithBonus = self:summonedEffectValue(mechanics)
	local creature = LIBRARY:getCreatureByName(self.id)
	if self.summonByHealth then
		return math.floor(valueWithBonus / creature:getMaxHealth())
	else
		return valueWithBonus
	end
end

--- Returns true if spell can be casted in general
--- if no valid targets exist, script needs to call `problem:add`
--- to explain the reason to the player
function Script:applicableGeneral(mechanics, problem)
	local creature = LIBRARY:getCreatureByName(self.id)

	if self:summonedCreatureAmount(mechanics) == 0 then
		problem:addGeneric(mechanics)
		return false
	end

	-- check if there are summoned creatures of other type
	if self.exclusive then
		local elementals = mechanics:getBattle():getUnitsIf(function(unit)
			return (unit:getOwner() == mechanics:getCasterColor())
				and (unit:isSummoned())
				and (not unit:isClone())
				and (unit:getCreature():getJsonKey() ~= creature:getJsonKey())
		end)
		local elemental = elementals[1]

		if elemental ~= nil then
			local hero = mechanics:getHeroCaster()
			local himHer = "core.genrltxt.539"
			if hero ~= nil and hero:isFemale() then
				himHer = "core.genrltxt.540"
			end

			if hero ~= nil then
				problem:addCustom({
					append         = { "core.genrltxt.538" },
					replaceStrings = {
						hero:getNameTextID(),
						elemental:getCreature():getNameTextID(0),
						himHer
					}
				})
			else
				problem:addCustom({
					append = { "core.genrltxt.538" }
				})
			end
			return false
		end
	end
	return true
end

--- Actually casts the spells and applies all changes caused by spell
--- use `server` parameter to apply changes on specified target(s)
function Script:apply(mechanics, server, target)
	local creature = LIBRARY:getCreatureByName(self.id)
	local battle   = mechanics:getBattle()

	for _, dest in ipairs(target) do
		if dest.unit ~= nil then
			server:healUnit(
				battle,
				dest.unit,
				self:summonedCreatureHealth(mechanics),
				ENUM.HealLevel.overheal,
				self.permanent and ENUM.HealPower.permanent or ENUM.HealPower.oneBattle
			)
		else
			print("SpellEffectSummon. Hex: ", dest.hex)
			assert(dest.hex ~= nil)
			server:addUnit(
				battle,
				{
					count = self:summonedCreatureAmount(mechanics),
					type = creature,
					side = mechanics:getCasterSide(),
					position = dest.hex,
					summoned = not self.permanent
				}
			)
		end
	end
end

--- converts specified range into actual list of affected targets
--- for example, area damage spells should locate all units on affected hexes
--- and return list of affected units
function Script:transformTarget(mechanics, aimPoint, spellTarget)
	local creature = LIBRARY:getCreatureByName(self.id)
	local sameSummoned = mechanics:getBattle():getUnitsIf(function(unit)
		return (unit:getOwner() == mechanics:getCasterColor())
			and (unit:isSummoned())
			and (not unit:isClone())
			and (unit:getCreature():getJsonKey() == creature:getJsonKey())
			and (unit:isAlive())
	end)[1]

	if sameSummoned == nil or not self.summonSameUnit then
		local hex = mechanics:getBattle():getAvailableHex(creature, mechanics:getCasterSide())
		if not hex:isValid() then
			return {} -- no free space. FIXME: should be in isApplicable
		else
			return {
				{
					hex = hex,
					unit = nil
				}
			}
		end
	else
		return {
			{
				hex = sameSummoned:getPosition(),
				unit = sameSummoned
			}
		}
	end
end

return Script
