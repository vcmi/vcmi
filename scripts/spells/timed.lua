local Base = require("spells/unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:deepCopyBonus(b)
	local copy = {}
	for k, v in pairs(b) do
		copy[k] = v
	end
	return copy
end

function Script:convertBonuses(mechanics)
	local duration = mechanics:getEffectDuration()
	local spellKey = mechanics:getSpell():getJsonKey()
	local converted = {}

	for name, b in pairs(self.bonus or {}) do
		local nb = self:deepCopyBonus(b)

		if not nb.turns or nb.turns == 0 then
			nb.turns = duration
		end

		nb.sourceType = "SPELL_EFFECT"
		nb.sourceID = spellKey

		converted[name] = nb
	end

	return converted
end

--- Shifts every buffered bonus value by a per-target-tier amount (weakness/slayer-style).
function Script:applyPeculiarEnchant(mechanics, hero, buffer, tier, spellKey)
	local peculiar = hero:getBonuses({type = "SPECIAL_PECULIAR_ENCHANT", subtype = spellKey})
	if peculiar:size() == 0 then return end

	local bonus = peculiar:getBonus(1)
	local levels = bonus:getParametersAsVector()
	local power = 0
	if #levels > 0 then
		-- explicit per-tier values from addInfo array (sign included), clamping (tier - 1) into bounds
		local idx = math.max(1, math.min(#levels, tier))
		power = levels[idx]
	else
		if tier <= 2 then power = 3
		elseif tier <= 4 then power = 2
		elseif tier <= 6 then power = 1
		end
		if mechanics:isNegative() then power = -power end
	end
	if power ~= 0 then
		for _, nb in pairs(buffer) do
			nb.val = (nb.val or 0) + power
		end
	end
end

--- Adds a flat amount to every buffered bonus value (Aenain-style).
function Script:applyAddValueEnchant(mechanics, hero, buffer, tier, spellKey)
	local addVal = hero:getBonuses({type = "SPECIAL_ADD_VALUE_ENCHANT", subtype = spellKey})
	if addVal:size() == 0 then return end

	local addAmount = addVal:getBonus(1):getParametersAsNumber()
	for _, nb in pairs(buffer) do
		nb.val = (nb.val or 0) + addAmount
	end
end

--- Overwrites every buffered bonus value with a fixed amount (Daremyth-style).
function Script:applyFixedValueEnchant(mechanics, hero, buffer, tier, spellKey)
	local fixedVal = hero:getBonuses({type = "SPECIAL_FIXED_VALUE_ENCHANT", subtype = spellKey})
	if fixedVal:size() == 0 then return end

	local fixedAmount = fixedVal:getBonus(1):getParametersAsNumber()
	for _, nb in pairs(buffer) do
		nb.val = fixedAmount
	end
end

--- Scales every buffered bonus value by a per-target-tier percentage (Solmyr-style
--- SPECIAL_SPELL_SCALING, matching CGHeroInstance::getSpellBonus but for buff/debuff vals).
function Script:applySpellScaling(mechanics, hero, buffer, tier, spellKey)
	local scaling = hero:getBonuses({type = "SPECIAL_SPELL_SCALING", subtype = spellKey})
	if scaling:size() == 0 then return end

	local percent = scaling:getBonus(1):getVal() * math.floor(hero:getLevel() / tier)
	if percent == 0 then return end
	for _, nb in pairs(buffer) do
		nb.val = math.floor((nb.val or 0) * (100 + percent) / 100)
	end
end

function Script:applyHeroSpecialty(mechanics, buffer, unit)
	local hero = mechanics:getHeroCaster()
	if not hero then return end

	local spellKey = mechanics:getSpell():getJsonKey()
	local tier = math.max(unit:creatureLevel(), 1)

	self:applySpellScaling(mechanics, hero, buffer, tier, spellKey)
	self:applyPeculiarEnchant(mechanics, hero, buffer, tier, spellKey)
	self:applyAddValueEnchant(mechanics, hero, buffer, tier, spellKey)
	self:applyFixedValueEnchant(mechanics, hero, buffer, tier, spellKey)
end

function Script:describeEffect(server, battle, unit, bonuses)
	-- Age spell: STACK_HEALTH bonus with negative val gets a custom message
	for _, nb in pairs(bonuses) do
		if nb.type == "STACK_HEALTH" and (nb.val or 0) < 0 then
			local oldHealth = unit:getMaxHealth()
			local lost = oldHealth - math.floor((oldHealth * (100 + nb.val)) / 100)
			local count = unit:getCount()
			local ageTextID = count == 1 and self.battleLogSingular or self.battleLogPlural
			server:appendLog(battle, {
				append         = { ageTextID },
				replaceStrings = { unit:getCreature():getNameTextID(count) },
				replaceNumbers = { lost }
			})
			return
		end
	end

	if not self.battleLogPlural or self.battleLogPlural == "" then return end

	local textID = (self.battleLogSingular and self.battleLogSingular ~= "" and unit:getCount() == 1) and self.battleLogSingular or self.battleLogPlural
	local nameTextID = unit:getCreature():getNameTextID(unit:getCount())
	server:appendLog(battle, {
		append         = { textID },
		replaceStrings = { nameTextID }
	})
end

function Script:apply(mechanics, server, target)
	local battle   = mechanics:getBattle()
	local describe = server:describeChanges()
	local converted = self:convertBonuses(mechanics)

	for _, dest in ipairs(target) do
		local unit = dest.unit
		if not unit or not unit:isAlive() then goto continue end

		local buffer = {}
		for name, nb in pairs(converted) do
			buffer[name] = self:deepCopyBonus(nb)
		end

		self:applyHeroSpecialty(mechanics, buffer, unit)

		if describe then
			self:describeEffect(server, battle, unit, buffer)
		end

		for _, nb in pairs(buffer) do
			server:addUnitBonus(battle, unit, nb, self.cumulative or false)
		end

		::continue::
	end
end

return Script
