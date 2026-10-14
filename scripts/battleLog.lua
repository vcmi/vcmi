local BattleLog = {}

--- Battle log messages that several combat and spell scripts share.

--- Name of the creatures of `victim`, in the form matching `count`. A nil victim stands for
--- creatures in general, which is what the log says when the victim can no longer be named.
local function creatureName(victim, count)
	if victim then
		return victim:getCreature():getNameTextID(count)
	end

	-- 42 and 43 are "creature" and "creatures"
	return count == 1 and "core.genrltxt.42" or "core.genrltxt.43"
end

--- "<attacker> drains life from <victim>", optionally followed by how many creatures it raised.
--- Life drain and soul steal have always shared this wording, since both fed the same counter.
--- `drainerCount` is how big the draining stack was before it healed, since that is the stack the
--- message names - it must be read before the heal, which may well have grown it.
function BattleLog.lifeDrained(server, battle, unit, victim, healed, resurrected, drainerCount)
	-- 361 and 362 are the singular and plural forms of the same message
	local lines = { drainerCount == 1 and "core.genrltxt.361" or "core.genrltxt.362" }
	local numbers = { healed }

	if resurrected == 1 then
		table.insert(lines, "core.genrltxt.363")
	elseif resurrected > 1 then
		table.insert(lines, "core.genrltxt.364")
		table.insert(numbers, resurrected)
	end

	-- both stacks are named in the form matching what is left of them
	local victimCount = victim and victim:getCount() or 0

	server:appendLog(battle, {
		append         = lines,
		replaceStrings = { unit:getCreature():getNameTextID(drainerCount), creatureName(victim, victimCount) },
		replaceNumbers = numbers
	})
end

--- "<n> <creatures> perish". Says nothing when the effect killed nobody.
function BattleLog.creaturesPerish(server, battle, victim, killed)
	if killed <= 0 then return end

	-- 378 and 379 are the singular and plural forms of the same message
	server:appendLog(battle, {
		append         = { killed == 1 and "core.genrltxt.378" or "core.genrltxt.379" },
		replaceStrings = { creatureName(victim, killed) },
		replaceNumbers = { killed }
	})
end

--- "<spell> does <n> damage", followed by the casualties it caused.
function BattleLog.spellDamage(server, battle, spell, victim, damage, killed)
	server:appendLog(battle, {
		append         = { "core.genrltxt.376" },
		replaceStrings = { spell:getNameTextID() },
		replaceNumbers = { damage }
	})

	BattleLog.creaturesPerish(server, battle, victim, killed)
end

return BattleLog
