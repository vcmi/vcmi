
-- make effect work only on expert level
applicable = function ()
	return effectLevel >= 3
end

-- make effect work only on left side of battle field
applicableTarget = function(targets)
	if not #targets then
		return false
	end

	for _, dest in ipairs(targets) do
		local hex = dest[1]
		local x = hex % 17

		if x>9 then
			return false
		end
	end

	return true
end



