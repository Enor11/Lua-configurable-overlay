-- Called when pressed key
typed_text = ""
function on_key(key)
	print("[Lua] Key event received: " .. key)
	if key == 9 then
		config.RUNNING = 0
	end
	--if key == 38 then
		--os.execute("foot &")
	--end
	if key == 24 then
		config.OFFSET_X = config.OFFSET_X - 10
		config.OFFSET_Y = config.OFFSET_Y - 10
	end
	if key == 26 then
		config.OFFSET_X = config.OFFSET_X + 10
		config.OFFSET_Y = config.OFFSET_Y + 10
	end

	if key == 38 then
		config.WIDTH = config.WIDTH - 20
		print("Resized window width to: " .. config.WIDTH)
	end
	if key == 40 then
		config.WIDTH = config.WIDTH + 20
		print("Resized window width to: " .. config.WIDTH)
	end
	if key == 25 then
		config.HEIGHT = config.HEIGHT - 20
		print("Resized window HEIGHT to: " .. config.HEIGHT)
	end
	if key == 39 then
		config.HEIGHT = config.HEIGHT + 20
		print("Resized window HEIGHT to: " .. config.HEIGHT)
	end

	local sym = get_symbol(key)
	if sym then
		if key == 22 then
			typed_text = string.sub(typed_text, 1, -2)
		else
			typed_text = typed_text .. sym
		end
	end
end

-- Called each frame
function on_update()
	-- Draw shapes to match the current window dimensions
	fill_rect(0, 0, config.WIDTH, config.HEIGHT, config.COLOR_SHAPE)
	fill_rect(4, 4, config.WIDTH - 8, config.HEIGHT - 8, config.COLOR_BACKGROUND)
	draw_text(typed_text, 20, 20, 2, config.COLOR_SHAPE)
	draw_line(50, 50, 10, 250, 10, config.COLOR_SHAPE)
end

function get_apps(dirs)
	List = nil
	for i,dir in ipairs(dirs) do
		for file in io.popen("ls -- " .. dir):lines() do
			if string.match(' '..file..' ', ".desktop") ~= nil then
				print("file: " .. file)
				local name, exec
				for line in io.popen("grep -E 'Exec=|Name=' " .. dir .. file):lines() do
					if not name and line:find("^Name=") then
						name = line:sub(6)
					elseif not exec and line:find("^Exec=") then
						exec = line:sub(6)
					end
					if name and exec then break end
				end
				if name and exec then
					print("\tname: " .. name)
					print("\texec: " .. exec)
					List = {next = List, value = {name = name, exec = exec}}
				end
			end
		end
	end
end

-- Runs once at startup
function init()
	config.COLOR_BACKGROUND = 0xFF1E1E2E
	config.COLOR_SHAPE = 0xFFCDD6F4
	config.COLOR_SHADOW = 0xFF000000

	config.SHADOW_OFFSET_X = 2
	config.SHADOW_OFFSET_Y = 2

	config.WIDTH = 900
	config.HEIGHT = 600

	config.OFFSET_X = 10
	config.OFFSET_Y = 10

	config.FPS = 20

	get_apps({"/usr/share/applications/","/"})
	local l = List
	while l do
		print(l.value.name .. " | Exec: " .. l.value.exec)
		l = l.next
	end
end
