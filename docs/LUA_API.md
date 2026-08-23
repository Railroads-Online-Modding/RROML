# RROML Lua API — v1.2.0

Lua mods run on **MoonSharp 2.0.0** (Lua 5.2, ~99% compatible). Each mod gets an isolated `Script` (`CoreModules.Preset_Default`).

## 1. Layout

Folder mod (recommended):

```
Mods/MyLuaMod/
  mod.json
  main.lua
  helper.lua  (optional, loaded via require)
```

`mod.json`:

```json
{
  "Id": "MyLuaMod",
  "Name": "My Lua Mod",
  "Version": "1.0.0",
  "EntryLua": "main.lua",
  "Dependencies": []
}
```

Loose file:

```
Mods/MyLoose.lua
```

Auto-detected, no `mod.json` required.

Folder without `mod.json` is guessed in order: `main.lua`, `init.lua`, `mod.lua`, then first `*.lua` at top level. Generic names (`main`/`init`/`mod`) resolve the mod id from the folder name to avoid collisions.

Manifest fields `RROML/src/RROML.Core/ModManifest.cs:5`:

- `EntryLua` — path relative to mod folder
- `EntryDll` — C# entry (still supported)
- `Id`, `Name`, `Version`, `Dependencies` — shared with C# mods

A folder may declare both `EntryLua` and `EntryDll`; both are loaded as separate candidates (`RROML/src/RROML.Core/ModLoader.cs:94`).

## 2. Lifecycle

File is executed via `Script.DoString(code, null, filename)` (`RROML/src/RROML.Core/Lua/LuaModRunner.cs:32`). Then `OnLoad` is dispatched:

1. global `OnLoad`
2. global `onLoad` (lowercase fallback)
3. returned table `OnLoad` / `onLoad` if file ends with `return { OnLoad = function(...) end }`

If none found, the top-level execution still counts as loaded.

`OnLoad` receives the `rroml` table:

```lua
function OnLoad(rroml)
    rroml.log.info("loaded " .. rroml.mod.id)
end

-- alternative
local mod = {}
function mod.OnLoad(ctx) ctx.log.info("table style") end
return mod
```

Reference: `RROML/src/RROML.Core/Lua/LuaModRunner.cs:127`.

## 3. `rroml` / `RROML` Global

Injected before execution (`RROML/src/RROML.Core/Lua/LuaModRunner.cs:81`):

```lua
rroml.log.info("msg")
rroml.log.warn("msg")
rroml.log.error("msg")
print("msg") -- alias to rroml.log.info

rroml.paths.gameRoot    -- string, folder containing arr.exe
rroml.paths.loaderPath  -- <gameRoot>/RROML
rroml.paths.modsPath    -- <gameRoot>/Mods
rroml.paths.modRoot     -- this mod's folder
rroml.paths.configRoot  -- GetConfigPath("") trimmed

rroml.mod.id            -- from mod.json or file/folder name
rroml.mod.name
rroml.mod.version       -- default "1.0.0"
rroml.mod.root          -- absolute mod folder
rroml.mod.entry         -- absolute entry file

rroml.getConfigPath("settings.json")         -- -> RROML/Configs/<Id>/settings.json
rroml.getUserGameConfigPath("Engine.ini")    -- -> %LocalAppData%/arr/Saved/Config/Windows[NoEditor] or fallback
rroml.version           -- "1.2.0" (RROML/src/RROML.Core/RROMLVersion.cs:3)
RROML -- alias of rroml
```

All logging goes to `RROML/Logs/rroml.log`.

## 4. Standard Library

`Preset_Default` includes `basic`, `string`, `table`, `math`, `coroutine`, `os`, `io`, `debug`, etc. `print` is overridden to log. `require` is enabled via `FileSystemScriptLoader`.

## 5. `require`

`ModulePaths` (`RROML/src/RROML.Core/Lua/LuaModRunner.cs:63`):

```
<modRoot>/?.lua
<modRoot>/?/init.lua
<modRoot>/?/.lua
?.lua
?/init.lua
```

Example (`RROML/src/ExampleLuaMod/helper.lua:1`):

```lua
-- helper.lua
local helper = {}
helper.greeting = "Hello from helper.lua!"
helper.version = "1.0.0"
function helper.sayHello(name) return "Hello, " .. tostring(name) .. " from helper!" end
return helper
```

```lua
-- main.lua
local helper = require("helper")
rroml.log.info(helper.sayHello("world"))
```

## 6. Working Example

`RROML/src/ExampleLuaMod/main.lua:1` (comments stripped):

```lua
local MOD_NAME = "Example Lua Mod"
local MOD_VERSION = "1.0.0"

function OnLoad(context)
    rroml.log.info("[" .. MOD_NAME .. " v" .. MOD_VERSION .. "] OnLoad called")
    rroml.log.info("  Game root : " .. rroml.paths.gameRoot)
    rroml.log.info("  Loader path: " .. rroml.paths.loaderPath)
    rroml.log.info("  Mods path  : " .. rroml.paths.modsPath)
    rroml.log.info("  Mod root   : " .. rroml.paths.modRoot)
    rroml.log.info("  Config root: " .. rroml.paths.configRoot)
    rroml.log.info("  Mod id     : " .. rroml.mod.id)
    rroml.log.info("  Mod version: " .. rroml.mod.version)

    local myConfig = rroml.getConfigPath("settings.json")
    local userConfig = rroml.getUserGameConfigPath("Engine.ini")
    rroml.log.info("  My config path  : " .. myConfig)
    rroml.log.info("  User game config: " .. userConfig)

    do
        local f = io.open(myConfig, "r")
        if f == nil then
            rroml.log.info("  No existing config, creating default at " .. myConfig)
            local out = io.open(myConfig, "w")
            if out ~= nil then
                out:write('{\n  "enabled": true,\n  "message": "Hello from Lua!"\n}\n')
                out:close()
                rroml.log.info("  Default config written")
            else
                rroml.log.warn("  Could not write config file")
            end
        else
            f:close()
            rroml.log.info("  Config already exists, skipping creation")
        end
    end

    local sum = 0
    for i = 1, 5 do sum = sum + i end
    rroml.log.info("  Lua loop test sum 1..5 = " .. sum)

    local greeting = string.format("Hello from %s! RROML %s is running Lua %s", rroml.mod.id, rroml.version, _VERSION)
    rroml.log.info("  " .. greeting)
    print(greeting)

    rroml.log.info("[" .. MOD_NAME .. "] OnLoad finished")
end
```

C# counterpart: `RROML/src/ExampleMod/ExampleMod.cs:1`.

## 7. Loader Behavior

- Discovery: `RROML/src/RROML.Core/ModLoader.cs:52` scans `Mods/*.dll`, `Mods/*.lua`, then each `Mods/*/`.
- Ordering: topological sort by `Dependencies`, warns on cycles/duplicates (`OrderCandidates:191`).
- Disabled: entries in `RROML/RROML.config.json` `DisabledMods` skip by candidate id, filename, or `rroml.mod.id` (`LoadOne:255`, `LoadLuaOne:331`).
- Errors: `InterpreterException` logged as `Lua error in mod <id>: <decorated>` and counts as not loaded; does not crash other mods.
- Install: `RROML/tools/install.bat:34` copies `MoonSharp.Interpreter.dll` to `<game>/RROML/`; `RROML/tools/build-managed.bat:30` builds `MoonSharp` + `Lua/*.cs`.

## 8. Troubleshooting

- Check `RROML/Logs/rroml.log` for `Loading Lua mod <id> from <path>` and error stacks.
- Ensure `EntryLua` is relative, file exists, UTF-8 encoding.
- Missing `MoonSharp.Interpreter.dll` at `RROML/` causes `RROML.Core` load failure; rebuild copies it to `RROML/build/managed/`.
- Dependency not found logs `declares missing dependency` and still attempts load after available deps.

## 9. Version

- RROML `1.2.0`, MoonSharp `2.0.0.0`, Lua `5.2` (`RROML/src/RROML.Core/RROMLVersion.cs:3`).
- Engine file: `RROML/lib/MoonSharp.Interpreter.dll` (vendored, MIT).
