local lolotoa = pd.Class:new():register("lolotoa")

function lolotoa:initialize(name, atoms)
	self.inlets = 1
	self.outlets = 4
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

function lolotoa:in_1_create(atoms)
	local l = self.list
	local k = atoms[1]
	local sel = table.remove(atoms, 1)
	while l.next do
		if l.next.index > k then break end
		l = l.next
	end
	if l.index == k then
		return
	else l.next = {next = l.next, index = k, val = {}, count = 0}
	end
end

function lolotoa:in_1_rem(atoms)
	local l = self.list
	local prev = l
	local k = atoms[1]
	while l.next do
		prev = l
		l = l.next
		if l.index and l.index >= k then break end
	end
	if l.index == k then
		prev.next = l.next
	else pd.post("lolotoa: item " .. k .. " not found")
	end
end

function lolotoa:in_1_dump()
	local outable
	local count = 1
	local l = self.list
	while l.next do
		l = l.next
		self:outlet(4, "float", {count})
		count = count + 1
		self:outlet(3, "float", {l.index})
		self:outlet(2, "float", {l.count})
		for i, v in pairs(l.val) do
			self:outlet(1, i, v)
		end
	end
end

function lolotoa:in_1_nth(num)
	local outable
	local l = self.list
	num = num[1]
	for i=1, num do
		if not l.next then break end
		l = l.next
	end
	self:outlet(3, "float", {l.index})
	self:outlet(2, "float", {l.count})
	for i, v in pairs(l.val) do
		self:outlet(1, i, v)
	end
end

function lolotoa:in_1_index(num)
	local l = self.list
	local count = 0
	num = num[1]
	while l.next do
		if l.next.index > num then break end
		l = l.next
		count = count + 1
	end
	if not l.index == num then
		pd.post("lolotoa: index " .. num .. " not found")
		return
	end
	self:outlet(4, "float", {count})
	self:outlet(2, "float", {l.count})
	for i, v in pairs(l.val) do
		self:outlet(1, i, v)
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