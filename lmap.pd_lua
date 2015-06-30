local lmap = pd.Class:new():register("lmap")
-- for lua/Pd type conversion
local selectormap = {string = "symbol", number="float", userdata="pointer"}

function lmap:initialize(name, atoms)
	self.inlets = 1
	self.outlets = 4
	self.map = {}
	return true
end

function lmap:in_1_print()
	pd.post("lmap print:")
	for i, k in pairs(self.map)
		do pd.post("key: " .. i .. " value(s): " .. table.concat(k, ", "))
	end
end

function lmap:in_1_dump()
	local count = 0
	for i, k in pairs(self.map)
		do
			self:outlet(3, i, k)
			count = count + 1
	end
	self:outlet(4, "float", {count})
end

function lmap:getatoms(key)
	local v = self.map[key]
	if not v then
		self:outlet(2, "bang", {})
	elseif #v == 1 then
		self:outlet(1, selectormap[type(v[1])], v)
	else self:outlet(1, "list", v) end
end

function lmap:in_1_rem(atoms)
	self.map[atoms[1]] = nil
end

function lmap:in_1_list(atoms)
	local k = table.remove(atoms, 1)
	self.map[k] = atoms
end

function lmap:in_1_symbol(symbol)
	self:getatoms(symbol)
end

function lmap:in_1_float(float)
	self:getatoms(float)
end

function lmap:in_1(sel, atoms)
		if #atoms == 0 then
			self:getatoms(sel)
		else
			self.map[sel] = atoms
		end
end 