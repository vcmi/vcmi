local Policy = {}

-- This is the executable specification of NK2's compiled priority formula.
-- It intentionally contains no tuning: copy it before changing strategy.
-- It reads only the request so captured inputs are sufficient for differential
-- tests, although experimental policies may also use the standard VCMI globals.
-- A positive score keeps a candidate; zero rejects it. Higher scores win
-- within the first priority tier that produces any accepted candidates.

local BUILDINGS = "buildings"
local INSTAKILL = "instakill"
local INSTADEFEND = "instaDefend"
local KILL = "kill"
local ESCAPE = "escape"
local EXPLORE_AND_GATHER = "exploreAndGather"
local DEFEND = "defend"

local RESOURCE_NAMES = { "wood", "mercury", "ore", "sulfur", "crystal", "gems", "gold" }

-- Lua calculates with doubles, while NK2 stores priorities as C++ floats.
-- These helpers round at the same expression boundaries as the C++ scorer;
-- retaining them is necessary for exact shadow-mode and test parity.
local function add(left, right)
	return float32(float32(left) + float32(right))
end

local function subtract(left, right)
	return float32(float32(left) - float32(right))
end

local function multiply(left, right)
	return float32(float32(left) * float32(right))
end

local function divide(left, right)
	return float32(float32(left) / float32(right))
end

local function almostZero(value)
	return math.abs(value) <= 0.00001
end

local function evaluateMovement(score, movementCost)
	-- movementCost is measured in hero-turns. Sub-turn routes are rewarded for
	-- using less movement; routes of one turn or more receive a steeper penalty.
	if movementCost > 0 then
		if movementCost < 1 then
			score = divide(score, float32(movementCost ^ float32(0.6)))
		else
			local power = float32(movementCost ^ float32(1.3))
			score = float32(score / (0.75 + power))
		end
	end
	return score
end

local function evaluateArmyLossRatio(score, armyLossRatio, heroRole)
	-- Expected losses reduce every score. Any lossy task is additionally a poor
	-- use of a scout, whose job is exploration rather than carrying the army.
	if armyLossRatio > 0 then
		score = subtract(score, multiply(score, armyLossRatio))
		if heroRole ~= "main" then
			score = divide(score, 5.0)
		end
	end
	return score
end

local function evaluateSkillReward(score, skillReward, armyInvolvement, armyLossRatio)
	local survivingArmy = subtract(1.0, armyLossRatio)
	local reward = multiply(skillReward, armyInvolvement)
	reward = multiply(reward, survivingArmy)
	reward = multiply(reward, 0.05)
	return add(score, reward)
end

local function evaluateConquestValue(score, conquestValue, armyInvolvement)
	-- Conquest value replaces accumulated reward with the strategic value of
	-- committing this army; ordinary non-conquest tasks keep their prior score.
	if conquestValue > 0 then
		return multiply(armyInvolvement, conquestValue)
	end
	return score
end

local function maximumArmyLoss(state, evaluation)
	-- Losing the last town permits desperate fights. Otherwise willingness to
	-- take losses scales with how much of the available army this hero carries.
	if state.daysWithoutCastle then
		return float32(1.0)
	end
	local scaled = multiply(state.maximumArmyLossTarget, evaluation.powerRatio)
	if scaled > 0 then
		return scaled
	end
	return float32(1.0)
end

local function maximumArmyLossForTask(state, evaluation)
	local base = maximumArmyLoss(state, evaluation)
	-- Important enemy towns justify progressively more risk, capped at 75% of
	-- the committed army. Enemy heroes do not receive this relaxation.
	local enemyTownConquest = evaluation.isEnemy
		and not evaluation.isHero
		and evaluation.conquestValue > 2.0
	if not enemyTownConquest or evaluation.conquestValue <= 2.0 then
		return base
	end
	local conquestPressure = multiply(subtract(evaluation.conquestValue, 2.0), 0.05)
	return math.min(add(base, conquestPressure), float32(0.75))
end

local function buildingTurnsToAfford(state, evaluation)
	-- The slowest missing resource determines the wait. A resource with no
	-- income makes an unaffordable building unreachable through waiting alone.
	local turns = 0
	for _, resource in ipairs(state.resourceNames or RESOURCE_NAMES) do
		local cost = evaluation.buildingCost.resources[resource]
		local needed = math.max(0, cost - state.resources[resource])
		if needed > 0 then
			local income = state.dailyIncome[resource]
			if income == 0 then
				return nil
			end
			turns = math.max(turns, math.ceil(needed / income))
		end
	end
	return turns
end

local function canAffordBuilding(state, evaluation)
	for _, resource in ipairs(state.resourceNames or RESOURCE_NAMES) do
		if state.resources[resource] < evaluation.buildingCost.resources[resource] then
			return false
		end
	end
	return true
end

local function missesNonGoldResource(state, evaluation)
	for _, resource in ipairs(state.resourceNames or RESOURCE_NAMES) do
		local missing = state.resources[resource] < evaluation.buildingCost.resources[resource]
		if resource ~= "gold" and missing then
			return true
		end
	end
	return false
end

local function evaluateCandidate(request, candidate)
	local input = candidate.rankingInput
	local tier = input.priorityTier
	local state = input.state
	local evaluation = input.evaluation
	-- Some building tasks arrive with an explicit priority from their caller;
	-- the compiled evaluator preserves it instead of recalculating the task.
	if tier == BUILDINGS and candidate.initialPriority > 0 then
		return float32(candidate.initialPriority)
	end

	local score = float32(0)
	local maximumLoss = maximumArmyLossForTask(state, evaluation)
	local maximumEnemyDangerRatio = evaluation.powerRatio > 0 and evaluation.powerRatio or 1.0
	local arriveNextWeek = state.dayOfWeek + evaluation.turn > state.daysInWeek

	-- Priority tiers are attempted in this order: immediate conquest, immediate
	-- defense, conquest, escape, exploration/gathering, then fallback defense.
	-- BUILDINGS is used separately when choosing economic and recruitment tasks.
	if tier == INSTAKILL then
		-- Only safe, same-turn conquests reachable in less than one hero-turn.
		if evaluation.turn > 0 or evaluation.isExchange or evaluation.isDefend then
			return 0
		end
		if evaluation.movementCost >= 1 then
			return 0
		end
		if subtract(maximumLoss, evaluation.armyLossRatio) < 0 then
			return 0
		end
		score = evaluateConquestValue(score, evaluation.conquestValue, evaluation.armyInvolvement)
		score = evaluateArmyLossRatio(score, evaluation.armyLossRatio, evaluation.heroRole)
		if almostZero(score)
			or (evaluation.enemyHeroDangerRatio > maximumEnemyDangerRatio and not state.daysWithoutCastle) then
			return 0
		end
		score = multiply(score, evaluation.closestWayRatio)
		return evaluateMovement(score, evaluation.movementCost)
	end

	if tier == INSTADEFEND then
		-- Prefer the response closest to 75% of the incoming threat: enough to be
		-- useful without committing every available army to the same defense.
		if not evaluation.isDefend then
			return 0
		end
		if subtract(maximumLoss, evaluation.armyLossRatio) < 0 then
			return 0
		end
		if evaluation.isEnemy and evaluation.turn > 0 then
			return 0
		end
		local canPrepareForNextTurnThreat = evaluation.turn == 0 and evaluation.threatTurns == 1
		if evaluation.threatTurns <= evaluation.turn or canPrepareForNextTurnThreat then
			local optimalStrength = multiply(evaluation.threat, 0.75)
			local deviation = math.abs(subtract(evaluation.armyInvolvement, optimalStrength))
			local deviationPercentage = divide(deviation, evaluation.threat)
			score = divide(1.0, add(1.0, deviationPercentage))
			score = multiply(score, evaluation.closestWayRatio)
			score = evaluateMovement(score, evaluation.movementCost)
		end
		return score
	end

	if tier == KILL then
		-- This tier handles non-immediate conquest, but deliberately avoids
		-- chasing heroes across turns or starting enemy tasks in the next week.
		if evaluation.isDefend then
			return 0
		end
		if evaluation.turn > 0 and evaluation.isHero then
			return 0
		end
		if arriveNextWeek and evaluation.isEnemy then
			return 0
		end
		score = evaluateConquestValue(score, evaluation.conquestValue, evaluation.armyInvolvement)
		if almostZero(score)
			or (evaluation.enemyHeroDangerRatio > maximumEnemyDangerRatio
				and (evaluation.turn > 0 or evaluation.isExchange)
				and not state.daysWithoutCastle) then
			return 0
		end
		if subtract(maximumLoss, evaluation.armyLossRatio) < 0 then
			return 0
		end
		score = evaluateArmyLossRatio(score, evaluation.armyLossRatio, evaluation.heroRole)
		score = multiply(score, evaluation.closestWayRatio)
		return evaluateMovement(score, evaluation.movementCost)
	end

	if tier == EXPLORE_AND_GATHER or tier == ESCAPE then
		-- These tiers are exclusively for non-conquest, non-defense, non-building
		-- work. Escape can seed the score with the reduction in immediate threat.
		if evaluation.conquestValue > 0 or evaluation.isDefend
			or evaluation.buildingCost.marketValue > 0 then
			return 0
		end
		if subtract(maximumLoss, evaluation.armyLossRatio) < 0 then
			return 0
		end
		if tier == EXPLORE_AND_GATHER and evaluation.enemyHeroDangerRatio > maximumEnemyDangerRatio then
			return 0
		end
		if tier == ESCAPE and input.escape.hasHero then
			local escape = input.escape
			if escape.currentDangerTurn < 1 and escape.currentDanger > escape.heroTotalStrength then
				local delta = subtract(escape.currentThreat, escape.destinationThreat)
				if delta > 0 then
					score = add(score, delta)
				end
			end
		end

		local requiresBattle = evaluation.armyLossRatio > 0 or evaluation.danger > 0
		score = add(score, multiply(evaluation.strategicalValue, 1000.0))
		if evaluation.explorePriority > 0 then
			-- Exploration priority is an ordinal where 1 is best. It replaces the
			-- generic strategic score so known frontier quality drives this task.
			score = divide(600.0, evaluation.explorePriority)
			if evaluation.heroRole == "main" and requiresBattle then
				score = multiply(score, 2.0)
			end
		end

		if evaluation.goldReward > 0 then
			-- Small rewards are favored, while rewards above 500 are discounted to
			-- keep large treasure estimates from overwhelming strategic work.
			local goldValue
			if evaluation.goldReward > 500 then
				goldValue = divide(evaluation.goldReward, 2.0)
			else
				goldValue = multiply(evaluation.goldReward, 2.0)
			end
			score = add(score, goldValue)
			if evaluation.heroRole == "main" then
				if requiresBattle then
					score = multiply(score, 2.0)
				else
					score = float32(score * 0.33)
				end
			end
		end

		if evaluation.skillReward > 0 then
			-- Main heroes receive permanent progression; scouts are strongly
			-- discouraged to avoid destabilizing NK2's main/scout role assignment.
			if evaluation.heroRole == "main" then
				score = add(1000.0, evaluateSkillReward(
					score,
					evaluation.skillReward,
					evaluation.armyInvolvement,
					evaluation.armyLossRatio))
				if not requiresBattle then
					score = multiply(score, 3.0)
				end
			else
				score = math.max(float32(1.0), divide(score, 1000.0))
			end
		end

		if evaluation.heroRole == "main" then
			score = add(score, evaluation.armyReward)
		else
			score = add(score, divide(evaluation.armyReward, 10.0))
		end
		score = add(score, evaluation.armyGrowth)
		if evaluation.goldCost > 0 then
			score = subtract(score, divide(evaluation.goldCost, 4.0))
		end
		score = evaluateArmyLossRatio(score, evaluation.armyLossRatio, evaluation.heroRole)
		score = multiply(score, evaluation.closestWayRatio)
		return evaluateMovement(score, evaluation.movementCost)
	end

	if tier == DEFEND then
		-- Last-resort defense and army upgrades are ranked only by the army they
		-- involve, after danger, route-sharing, and movement penalties.
		if evaluation.enemyHeroDangerRatio > maximumEnemyDangerRatio then
			return 0
		end
		if evaluation.isDefend or evaluation.isArmyUpgrade then
			score = float32(evaluation.armyInvolvement)
		end
		score = multiply(score, evaluation.closestWayRatio)
		return evaluateMovement(score, evaluation.movementCost)
	end

	if tier == BUILDINGS then
		-- Building and recruitment choices combine their benefits, then reject
		-- plans that conflict with reserved resources or cannot become affordable.
		if subtract(maximumLoss, evaluation.armyLossRatio) < 0 then
			return 0
		end
		if state.lockedResourceMarketValue > 0 then
			return 0
		end
		score = add(score, multiply(evaluation.conquestValue, 1000.0))
		score = add(score, multiply(evaluation.strategicalValue, 1000.0))
		score = add(score, evaluation.goldReward)
		score = evaluateSkillReward(
			score,
			evaluation.skillReward,
			evaluation.armyInvolvement,
			evaluation.armyLossRatio)
		score = add(score, evaluation.armyReward)
		score = add(score, evaluation.armyGrowth)

		if evaluation.buildingCost.marketValue > 0 then
			local remainingWood = state.resources.wood - evaluation.buildingCost.resources.wood
			if not evaluation.isTradeBuilding and remainingWood < 5
				and state.dailyIncome.wood < 1 and state.hasTownWithoutMarketplace then
				return 0
			end
			score = add(score, 1000.0)
			if state.goldPressureOverMax then
				score = divide(score, evaluation.buildingCost.marketValue)
			end
			if not canAffordBuilding(state, evaluation) then
				local turnsTo = buildingTurnsToAfford(state, evaluation)
				if turnsTo == nil then
					return 0
				end
				if missesNonGoldResource(state, evaluation) then
					score = divide(score, turnsTo)
				end
			end
		elseif evaluation.enemyHeroDangerRatio > 1 and not evaluation.isDefend
			and almostZero(evaluation.conquestValue) then
			return 0
		end
		return score
	end

	error("Unsupported priority tier " .. tostring(tier))
end

function Policy.rank(request)
	local scores = {}
	for _, candidate in ipairs(request.candidates) do
		local score = evaluateCandidate(request, candidate)
		-- Match the compiled evaluator's final guard against an invalid route
		-- ratio or another calculation producing NaN.
		if score ~= score then
			score = 0
		end
		table.insert(scores, { id = candidate.id, score = float32(score) })
	end
	return { scores = scores }
end

return Policy
