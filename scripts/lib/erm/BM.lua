require("battle.Unit")

local battle = BATTLE

local ReceiverBase = require("core:erm.ReceiverBase")

local BM = ReceiverBase:new()

function BM:new(x, unitId)
	local o = ReceiverBase.new(self)

	assert(unitId ~= nil, "!!BM requires unit identifier")

	o.unitId = unitId

	return o
end

function BM:A(x, p1)
	if type(p1) == "nil" then
		local unit = battle:getUnitById(self.unitId)

		return unit:getAttack(false)
	else
		error("!!BM:A set is not implemented")
	end
end

function BM:B(x, ...)
	error("!!BM:B is not implemented")
end

function BM:C(x, ...)
	error("!!BM:C is not implemented")
end

function BM:D(x, ...)
	error("!!BM:D is not implemented")
end

function BM:E(x, ...)
	error("!!BM:E is not implemented")
end

function BM:F(x, ...)
	error("!!BM:F is not implemented")
end

function BM:G(x, ...)
	error("!!BM:G is not implemented")
end

function BM:H(x, ...)
	error("!!BM:H is not implemented")
end

function BM:J(x, ...)
	error("!!BM:J is not implemented")
end

function BM:K(x, ...)
	error("!!BM:K is not implemented")
end

function BM:L(x, ...)
	error("!!BM:L is not implemented")
end

function BM:M(x, ...)
	error("!!BM:M is not implemented")
end

function BM:N(x, ...)
	error("!!BM:N is not implemented")
end

function BM:O(x, ...)
	error("!!BM:O is not implemented")
end

function BM:P(x, ...)
	error("!!BM:P is not implemented")
end

function BM:Q(x, ...)
	error("!!BM:Q is not implemented")
end

function BM:R(x, ...)
	error("!!BM:R is not implemented")
end

function BM:S(x, ...)
	error("!!BM:S is not implemented")
end

function BM:T(x, ...)
	error("!!BM:T is not implemented")
end

function BM:U(x, ...)
	error("!!BM:U is not implemented")
end

function BM:V(x, ...)
	error("!!BM:V is not implemented")
end

return BM
