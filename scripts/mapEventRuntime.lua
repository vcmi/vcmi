-- Runtime layer stacked over a map's event-handler table (available here as the global Base).
-- Decorates the raw AdventureServer proxy with the blocking actions (showQuestion /
-- showRewardsMessage / startCombat) and drives handlers as coroutines so those actions can pause
-- the script until the player replies. Returns a {run, resume} driver; other lookups (e.g. init)
-- fall through to the handler table via __index.

local threads = {}
local nextId = 0

local function wrapServer(raw, player, host)
	local server = {}
	function server:showQuestion(opts)
		local components = {}
		if opts.images then
			for _, image in ipairs(opts.images) do
				components[#components + 1] = {type = image[1], subType = image[2], value = image[3]}
			end
		end
		-- A HotA SHOW_QUESTION is always a yes/no prompt (mode 1), regardless of its image/exit code.
		raw:spawnDialog(player, opts.text, 1, components)
		local answer = coroutine.yield()
		if answer == 1 then
			if opts.onYes then opts.onYes() end
		elseif answer == 0 then
			if opts.onNo then opts.onNo() end
		else
			if opts.onCancel then opts.onCancel() end
		end
	end
	function server:showRewardsMessage(messagePlayer, text, reward)
		raw:spawnDialog(messagePlayer, text, 0, {})
		coroutine.yield()
		if reward then reward() end
	end
	function server:startCombat(hero, slots)
		-- Reuses the visited event/pandora (an armed instance) as the opposing army: its garrison is
		-- replaced with the event's creatures and a battle starts. The coroutine pauses until it ends.
		raw:spawnCombat(host, hero, slots)
		coroutine.yield()
	end
	-- Forward every other call to the real server proxy, rebinding self to the raw userdata.
	return setmetatable(server, {__index = function(_, key)
		local field = raw[key]
		if type(field) == "function" then
			return function(_, ...) return field(raw, ...) end
		end
		return field
	end})
end

local function step(id, answer)
	local co = threads[id]
	if not co then return 0 end
	local ok, err = coroutine.resume(co, answer)
	if not ok then
		threads[id] = nil
		error(err)
	end
	if coroutine.status(co) == "dead" then
		threads[id] = nil
		return 0
	end
	return id
end

-- __index = Base lets non-driver lookups (init and any other handler the engine calls directly)
-- resolve to the map's own table, so this layer only adds run/resume.
return setmetatable({
	run = function(handlerName, player, host, game, raw, extra1, extra2)
		local server = wrapServer(raw, player, host)
		nextId = nextId + 1
		local id = nextId
		threads[id] = coroutine.create(function()
			Base[handlerName](Base, game, server, extra1, extra2)
		end)
		return step(id, nil)
	end,
	resume = function(id, answer)
		return step(id, answer)
	end,
}, {__index = Base})
