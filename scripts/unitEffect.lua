local Script = {}
Script.__index = Script
Script.type = "unitEffect"

--- Actually casts the spell and applies all changes caused by it.
--- Use `server` parameter to apply changes on specified target(s).
--- target contains units pre-filtered by C++ (applicableUnit / filterTarget),
--- so every dest.unit is guaranteed alive and receptive.
function Script:apply(mechanics, server, target)
	return
end

--- Returns true if `unit` can be affected by this spell effect.
--- Called for every candidate unit during target selection.
function Script:isValidTarget(mechanics, unit)
	return unit:isValidTarget(false)
end

--- Returns true if `unit` is receptive to this spell effect (not immune).
--- Called for every candidate unit during target selection and filtering.
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
	return { hpDelta = 0, unitsDelta = 0, unitType = -1 }
end

return Script
