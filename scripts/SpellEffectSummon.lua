local BattleUnitsChanged = require("netpacks.BattleUnitsChanged")
local MetaString = require("texts.MetaString")

-- expected globals:
-- parameters:
-- - creature
-- - permanent
-- - exclusive
-- - summonByHealth
-- - summonSameUnit

local function summonedEffectValue(mechanics)
	return mechanics:applySpecificSpellBonus(mechanics:calculateRawEffectValue(0, mechanics:getEffectPower()))
end

local function summonedCreatureHealth(mechanics)
	local valueWithBonus = summonedEffectValue(mechanics)

	if parameters.summonByHealth then
		return valueWithBonus
	else
		return valueWithBonus * parameters.creature:getMaxHealth()
	end
end

local function summonedCreatureAmount(mechanics)
	local valueWithBonus = summonedEffectValue(mechanics)

	if parameters.summonByHealth then
		return math.floor(valueWithBonus / parameters.creature:getMaxHealth())
	else
		return valueWithBonus
	end
end

applicable = function(problem, mechanics)
	if parameters.creature == "nil" then
		return false -- mechanics:adaptGenericProblem(problem)
	end

	if summonedCreatureAmount(mechanics) == 0 then
		return false -- mechanics:adaptGenericProblem(problem)
	end

	-- check if there are summoned creatures of other type
	if parameters.exclusive then
		local otherSummoned = mechanics:battle():battleGetUnitsIf(function(unit)
			return (unit:getOwner() == mechanics:getCasterColor())
				and (unit:isSummoned())
				and (not unit:isClone())
				and (unit:getCreature() ~= self.creature)
		end)

		if #otherSummoned > 0 then
			local elemental = otherSummoned[1]

			local text = MetaString.new()
			text:appendTextID("core.genrltxt.538")

			local hero = mechanics:caster():getHeroCaster()
			if caster then
				text:replaceRawString(caster:getNameTranslated())
				text:replaceNamePlural(elemental:creatureId())

				if caster.gender == EHeroGender.FEMALE then
					text:replaceTextID("core.genrltxt.540")
				else
					text:replaceTextID("core.genrltxt.539")
				end
			end

			problem:add(text, Problem.NORMAL)
			return false
		end
	end

	return true
end

apply = function(server, mechanics, target)
	local pack = BattleUnitsChanged.new()
	pack.battleID = mechanics:battle():getBattle():getBattleID()

	for _, dest in ipairs(target) do
		if dest.unitValue then
			local summoned = dest.unitValue
			local state = summoned:acquire()
			local healthValue = summonedCreatureHealth(mechanics, summoned)
			
			state:heal(
				healthValue, 
				EHealLevel.OVERHEAL, 
				(self.permanent and EHealPower.PERMANENT or EHealPower.ONE_BATTLE)
			)
			
			table.insert(pack.changedStacks, {
				unitId = summoned:unitId(),
				operation = UnitChanges.EOperation.RESET_STATE
			})
			
			state:save(pack.changedStacks[#pack.changedStacks].data)
		else
			local amount = summonedCreatureAmount(mechanics)

			if amount < 1 then
				server:complain("Summoning didn't summon any!")
			else
				local id = mechanics:battle():battleNextUnitId();

				pack:add(id,
				{
					count = amount,
					type = parameters.creature,
					side = mechanics:casterSide(),
					position = dest.hexValue,
					summoned = not self.permanent
				})
			end
		end
	end

	if #pack.changedStacks > 0 then
		server:apply(pack)
	end
end

apply = function(mechanics, aimPoint, spellTarget)
	local sameSummoned = mechanics:battle():battleGetUnitsIf(function(unit)
		return (unit:getOwner() == mechanics:getCasterColor())
			and (unit:isSummoned())
			and (not unit:isClone())
			and (unit:getCreature() == parameters.creature)
			and (unit:isAlive())
	end)

	local effectTarget = {}

	if #sameSummoned == 0 or not self.summonSameUnit then
		local hex = mechanics:battle():getAvailableHex(parameters.creature, mechanics:casterSide())
		if not hex:isValid() then
			return {} -- no free space. FIXME: should be in isApplicable
		else
			return {
				{ hex }
			}
		end
	else
		return {
			{ sameSummoned[1]:getPosition(), sameSummoned[1] }
		}
	end
end


