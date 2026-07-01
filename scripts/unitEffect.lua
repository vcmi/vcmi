local Script = {}
Script.__index = Script
Script.type = "unitEffect"

--- Actually casts the spell and applies all changes caused by it.
--- Use `server` parameter to apply changes on specified target(s).
--- target contains units pre-filtered by filterTarget,
--- so every dest.unit is guaranteed alive and receptive.
function Script:apply(mechanics, server, target)
	return
end

--- Returns true if at least one valid target exists on the battlefield.
function Script:applicableGeneral(mechanics, problem)
	local found = mechanics:getBattle():getUnitsIf(function(unit)
		if not self:isValidTarget(mechanics, unit) then return false end
		if not self:isReceptive(mechanics, unit) then return false end
		if mechanics:isSmart() and not mechanics:ownerMatches(unit) then return false end
		return true
	end)
	if #found == 0 then
		problem:addStandard(mechanics, ENUM.SpellCastProblem.noAppropriateTarget)
		return false
	end
	return true
end

--- Returns true if at least one unit in target is valid (ignoring receptiveness per H3 rules).
function Script:applicableTarget(mechanics, problem, target)
	for _, dest in ipairs(target) do
		local unit = dest.unit
		if unit and self:isValidTarget(mechanics, unit) then
			if not mechanics:isSmart() or mechanics:ownerMatches(unit) then
				return true
			end
		end
	end
	return false
end

--- Returns true if `unit` can be affected by this spell effect.
function Script:isValidTarget(mechanics, unit)
	return unit:isValidTarget(false)
end

--- Returns true if `unit` is receptive to this spell effect (not immune).
function Script:isReceptive(mechanics, unit)
	if unit:isInvincible() and mechanics:isNegative() then
		return false
	end
	if self.ignoreImmunity then
		return not unit:hasAbsoluteImmunity(mechanics:getSpell())
	else
		return mechanics:isReceptive(unit)
	end
end

--- Returns health change preview data for hover tooltip.
function Script:getHealthChange(mechanics, spellTarget)
	return { hpDelta = 0, unitsDelta = 0 }
end

--- Adjusts the spell's required target types. Return the (possibly modified) types array.
function Script:adjustTargetTypes(mechanics, types)
	return types
end

--- Adjusts affected hexes for visual highlighting. Inserts hex positions from spellTarget.
function Script:adjustAffectedHexes(mechanics, hexes, spellTarget)
	for _, dest in ipairs(spellTarget) do
		if dest.hex and dest.hex:isValid() then
			hexes:insert(dest.hex)
		end
	end
end

--- Filters the transformed target, keeping only valid and receptive units.
function Script:filterTarget(mechanics, target)
	local result = {}
	for _, dest in ipairs(target) do
		local unit = dest.unit
		if unit and self:isValidTarget(mechanics, unit) and self:isReceptive(mechanics, unit) then
			table.insert(result, dest)
		end
	end
	return result
end

--- Builds the full list of affected units from the spell target.
--- Dispatches to transformByChain or transformByRange based on chainLength parameter.
function Script:transformTarget(mechanics, aimPoint, spellTarget)
	local chainLength = self.chainLength or 0
	if chainLength > 1 then
		return self:transformByChain(mechanics, aimPoint, spellTarget, chainLength)
	else
		return self:transformByRange(mechanics, aimPoint, spellTarget)
	end
end

--- Resolves the effective spell target from hex/unit destinations (non-chain spells).
--- Mirrors C++ UnitEffect::transformTargetByRange.
function Script:transformByRange(mechanics, aimPoint, spellTarget)
	local battle = mechanics:getBattle()

	local function mainFilter(unit)
		if not self:isValidTarget(mechanics, unit) then return false end
		if not self:isReceptive(mechanics, unit) then return false end
		if mechanics:isSmart() and not mechanics:ownerMatches(unit) then return false end
		return true
	end

	-- ensure we have a non-empty target to work from
	local effectiveTarget = spellTarget
	if #aimPoint > 0 and #spellTarget == 0 then
		effectiveTarget = { aimPoint[1] }
	end

	local targetsSet = {}
	local targetsOrder = {}

	local function addUnit(unit)
		if not targetsSet[unit] then
			targetsSet[unit] = true
			table.insert(targetsOrder, unit)
		end
	end

	if mechanics:isMassive() then
		local units = battle:getUnitsIf(mainFilter)
		for _, unit in ipairs(units) do
			addUnit(unit)
		end
	else
		for _, dest in ipairs(effectiveTarget) do
			if dest.unit then
				if mainFilter(dest.unit) then
					addUnit(dest.unit)
				end
			elseif dest.hex and dest.hex:isValid() then
				local onTile = battle:getUnitsIf(function(unit)
					return unit:coversPos(dest.hex) and mainFilter(unit)
				end)
				local chosen = nil
				for _, unit in ipairs(onTile) do
					if unit:isAlive() then
						chosen = unit
						break
					end
				end
				if chosen == nil and #onTile > 0 then
					chosen = onTile[1]
				end
				if chosen then
					addUnit(chosen)
				end
			end
		end
	end

	if mechanics:alwaysHitFirstTarget() and #aimPoint > 0 and aimPoint[1].unit then
		addUnit(aimPoint[1].unit)
	end

	local result = {}
	for _, unit in ipairs(targetsOrder) do
		table.insert(result, { unit = unit, hex = unit:getPosition() })
	end
	return result
end

--- Resolves chain-lightning style targets (chain of nearest units).
--- Mirrors C++ UnitEffect::transformTargetByChain.
function Script:transformByChain(mechanics, aimPoint, spellTarget, chainLength)
	local byRange = self:transformByRange(mechanics, aimPoint, spellTarget)
	if #byRange == 0 then return {} end

	local mainDest = byRange[1]
	if not mainDest.hex or not mainDest.hex:isValid() then return {} end

	local battle = mechanics:getBattle()
	local alwaysHitFirst = mechanics:alwaysHitFirstTarget()
	local effectTarget = {}
	local processedIds = {}

	local function isEligible(unit)
		return self:isReceptive(mechanics, unit) and self:isValidTarget(mechanics, unit)
	end

	local function buildPossibleHexes()
		local units = battle:getUnitsIf(function(u)
			return isEligible(u) and not processedIds[u]
		end)
		if #units == 0 then return nil end
		local hexes = units[1]:getHexes()
		for i = 2, #units do
			local uHexes = units[i]:getHexes()
			for j = 1, uHexes:size() do
				hexes:insert(uHexes:at(j))
			end
		end
		return hexes
	end

	local destHex = mainDest.hex
	local targetIndex = 0

	while targetIndex < chainLength do
		local unit = battle:getUnitByPos(destHex, true)
		if not unit then break end

		local wouldResist = mechanics:wouldResist(unit)

		if alwaysHitFirst and targetIndex == 0 then
			table.insert(effectTarget, { unit = unit, hex = unit:getPosition() })
		end

		if wouldResist and targetIndex == 0 then
			if not alwaysHitFirst then
				table.insert(effectTarget, { unit = unit, hex = unit:getPosition() })
			end
			break
		elseif isEligible(unit) and not wouldResist then
			if not (alwaysHitFirst and targetIndex == 0) then
				table.insert(effectTarget, { unit = unit, hex = unit:getPosition() })
			end
		elseif isEligible(unit) and wouldResist then
			-- unit skipped, no resistance animation (H3 logic); don't advance targetIndex
			processedIds[unit] = true
			local hexes = buildPossibleHexes()
			if not hexes then break end
			destHex = destHex:getClosestTile(unit:unitSide(), hexes)
			goto continue
		else
			table.insert(effectTarget, {})
		end

		processedIds[unit] = true

		local hexes = buildPossibleHexes()
		if not hexes then break end
		destHex = destHex:getClosestTile(unit:unitSide(), hexes)

		targetIndex = targetIndex + 1
		::continue::
	end

	return effectTarget
end

return Script
