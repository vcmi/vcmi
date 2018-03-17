local ReceiverBase = require("core:erm.ReceiverBase")
local TM = ReceiverBase:new()
local bit = bit
local band, bor, bxor = bit.band, bit.bor, bit.bxor
local ALL_PLAYERS = 0xFF

local DATA = DATA
DATA.ERM.timers = DATA.ERM.timers or {}

local timers = DATA.ERM.timers

local function newTimer(timerId)
	return
	{
		id = timerId,
		dayFirst = 1,
		dayLast = 1,
		interval = 0,
		players = 0
	}
end

local function getTimer(timerId)
	timerId = tostring(timerId)
	timers[timerId] = timers[timerId] or newTimer(timerId)
	return timers[timerId]
end

function TM:new(ERM, timerId)
	assert(timerId ~= nil, "!!TM requires timer identifier")
	return ReceiverBase.new(self,
	{
		timerId = timerId,
		timer = getTimer(timerId),
		ERM = ERM
	})
end

function TM:D(x, playerMask)
	--disable by mask
	self.timer.players = band(self.timer.players, bnot(playerMask), ALL_PLAYERS)
end

function TM:E(x, playerMask)
	--enable by mask
	self.timer.players = bor(self.timer.players, playerMask)
end

function TM:S(x, dayFirst, dayLast, interval, playerMask)
	local t = self.timer
	t.dayFirst = dayFirst
	t.dayLast = dayLast
	t.interval = interval
	t.players = playerMask
end

return TM
