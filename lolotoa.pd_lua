local lolotoa = pd.Class:new():register("lolotoa")

-- for lua/Pd type conversion
local selectormap = {string = "symbol", number="float", userdata="pointer"}

function lolotoa:initialize(name, atoms)
	self.inlets = 1
	self.outlets = 2
	self.list = {next = nil}
	return true
end

function lolotoa:in_1_add(atoms)
	local l = self.list
	local k = table.remove(atoms, 1)
	local sel = table.remove(atoms, 1)
	while l.next do
		if l.next.index > k then break end
		l = l.next
	end
	if l.index == k then
		if not l.val[sel] then l.count = l.count + 1 end
		l.val[sel] = atoms
	else l.next = {next = l.next, index = k, val = {}, count = 1}
		l.next.val[sel] = atoms
	end
end

function lolotoa:in_1_rem(atoms)
	local l = self.list
	local prev = l
	local k = atoms[1]
	while l.next do
		prev = l
		l = l.next
		if l.index and l.index > k then break end
	end
	if l.index == k then
		prev.next = l.next
	else pd.post("lolotoa: item not found")
	end
end

function lolotoa:in_1_dump()
	local count
	local outable
	local l = self.list
	while l.next do
		l = l.next
		self:outlet(2, "float", {l.count})
		for i, v in pairs(l.val) do
			self:outlet(1, i, v)
		end
	end
end

function lolotoa:in_1_remmes(atoms)
	local l = self.list
	local k = atoms[1]
	local sel = atoms[2]
	while l.next do
		l = l.next
		if l.index and l.index > k then break end
	end
	if l.index == k then
		if l.val[sel] then l.count = l.count - 1 end
		l.val[sel] = nil
	else pd.post("lolotoa: item not found")
	end
end