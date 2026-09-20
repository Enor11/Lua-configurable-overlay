-- Called when pressed key
function on_key(key)
  print("[Lua] Key event received: " .. key)
  if key == 9 then
    config.RUNNING = 0
  end
  if key == 38 then
    os.execute("foot &")
  end
  if key == 114 then
    --x = x + 1
    config.COLS = config.COLS + 1
  end
  if key == 113 then
    --x = x - 1
  end
  --if key == 111 then
    print(key .. " | " .. get_symbol(key))
  --end
  if key == 116 then
    --y = y + 1
  end
  if get_symbol(key) == 111 then
    print("up")
  end
end

-- Called each frame
function on_update()
  for y = 1, config.ROWS-2, 1 do
    set_char(y,0,"a")
    set_char(y,config.COLS-1,"a")
  end
  for x = 1, config.COLS-2, 1 do
    set_char(0,x,"a")
    set_char(config.ROWS-1,x,"a")
  end
  set_char(0,0,"d")
  set_char(config.ROWS-1,0,"d")
  set_char(0,config.COLS-1,"d")
  set_char(config.ROWS-1,config.COLS-1,"d")
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
  config.COLOR_BACKGROUND = 0xFFFF8000 --orange 0xFFFF8000, gray 0xFF808080
  config.COLOR_TEXT = 0xFFFFFFFF;
  config.COLOR_SHADOW = 0xFF000000;

  config.SHADOW_OFFSET_X = 1
  config.SHADOW_OFFSET_Y = 1

  config.ROWS = 35
  config.COLS = 25

  config.SCALE = 2
  config.OFFSET_X = 10
  config.OFFSET_Y = 10

  get_apps({"/usr/share/applications/","/"})
  local l = List
    while l do
      print(l.value.name .. " | Exec: " .. l.value.exec)
      l = l.next
    end
end
