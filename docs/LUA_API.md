# RROML Lua API (v1.2.0)

Lua mods are powered by **MoonSharp** (Lua 5.2, ~99% compatible). Each mod gets its own isolated `Script` instance with a soft-default sandbox.

## Quick Start

1. Create a folder: `Mods/MyLuaMod/`
2. Add `mod.json`:
```json
{
  "Id": "MyLuaMod",
  "Name": "My Lua Mod",
  "Version": "1.0.0",
  "EntryLua": "main.lua",
  "Dependencies": []
}
```
3. Add `main.lua`:

```lua
function OnLoad(rroml)
    rroml.log.info("Hello from Lua! Game root: " .. rroml.paths.gameRoot)
end
```

You can also place a single loose file `Mods/MyLoose.lua` – it will be auto-detected as a Lua mod without `mod.json`.

Supported entry resolution order for folders **without** `mod.json`:
1. `main.lua`
2. `init.lua`
3. `mod.lua`
4. First `*.lua` found in folder top-level

## Manifest Fields

`ModManifest` now supports:

- `EntryDll` – C# DLL entry (existing)
- `EntryLua` – Lua file path **relative to mod folder** (new in v1.2.0)
- `Id`, `Name`, `Version`, `Dependencies` – same as C# mods

You can declare **both** `EntryDll` and `EntryLua` in the same folder – both will be loaded as separate candidates (same `Id` warning will apply, so prefer distinct `Id`s or separate folders).

## Lifecycle

After the script is `DoString`'d, RROML tries to invoke `OnLoad` in this order:

1. Global `OnLoad` (`function OnLoad(rroml)`)
2. Global `onLoad` (lowercase fallback)
3. Returned table `OnLoad` – if your file ends with `return { OnLoad = function(...) end }`

Formally:

```lua
-- style A: global function
function OnLoad(ctx)
    ctx.log.info("global OnLoad")
end

-- style B: returned table (also valid)
local mod = {}
function mod.OnLoad(ctx)
    ctx.log.info("table OnLoad")
end
return mod
```

If no `OnLoad` is found the file is still considered **loaded successfully** – it ran top-level code.

`OnLoad` receives the `rroml` table as its single argument (also available as global `rroml` / `RROML`).

## API Reference

### `rroml` (and alias `RROML`)

| Field | Type | Description |
|---|---|---|
| `log` | table | Logging helpers |
| `paths` | table | Filesystem roots |
| `mod` | table | Metadata for current mod |
| `getConfigPath(fileName)` | function | Returns `RROML/Configs/<ModId>/fileName` (creates folder) |
| `getUserGameConfigPath(fileName)` | function | Returns `%LocalAppData%/arr/Saved/Config/Windows[/NoEditor]` or fallback |
| `version` | string | Loader version, e.g. `"1.2.0"` |

#### `rroml.log`

```lua
rroml.log.info("message")
rroml.log.warn("message")
rroml.log.error("message")
-- also: print("msg") aliases to log.info
```

All go to `RROML/Logs/rroml.log`.

#### `rroml.paths`

```lua
rroml.paths.gameRoot   -- game root containing arr.exe
rroml.paths.loaderPath -- <gameRoot>/RROML
rroml.paths.modsPath   -- <gameRoot>/Mods
rroml.paths.modRoot    -- this mod's folder (or Mods for loose files)
rroml.paths.configRoot -- same as getConfigPath("") trimmed
```

#### `rroml.mod`

```lua
rroml.mod.id      -- from mod.json or file name
rroml.mod.name    -- from mod.json or id
rroml.mod.version -- from mod.json or "1.0.0"
rroml.mod.root    -- absolute mod folder
rroml.mod.entry   -- absolute lua file path
```

### Standard Lua Libraries

Available (via `CoreModules.Preset_Default`): `basic`, `string`, `table`, `math`, `coroutine`, `os`, `io`, etc.

Dangerous functions (`os.execute` etc.) are **not** hard-sandboxed but will log warnings if misused – use with care.

### `require`

Uses `FileSystemScriptLoader` with `ModulePaths`:

```
<modRoot>/?.lua
<modRoot>/?/init.lua
?.lua
?/init.lua
```

So `require("helper")` loads `<modRoot>/helper.lua`, and `require("utils.math")` loads `<modRoot>/utils/math.lua`.

Example `helper.lua`:

```lua
local M = {}
function M.add(a,b) return a+b end
return M
```

Usage:

```lua
local helper = require("helper")
print(helper.add(2,3))
```

### Config Example

```lua
function OnLoad(rroml)
    local path = rroml.getConfigPath("settings.json")
    local f = io.open(path, "r")
    if not f then
        local out = io.open(path, "w")
        out:write('{"enabled":true}')
        out:close()
    else
        f:close()
    end
end
```

### C# Interop Notes

Lua mods run **on the same CLR** as C# mods (`.NET Framework 4.x` via `mscoree.dll` host). They share `DisabledMods`, dependency ordering, and file logging. You can call any exposed `rroml` function; exposing additional C# objects is not supported in v1.2.0 (future: UEBridge hooks).

## Example Mod

See `RROML/src/ExampleLuaMod/` – includes `main.lua`, `helper.lua`, and `mod.json`. Installed by `tools/install.bat` to `Mods/ExampleLuaMod/`.

## Troubleshooting

- **Lua error in log**: check `RROML/Logs/rroml.log` for `Lua error in mod XYZ: <stack>`
- **File not found**: ensure `EntryLua` is relative to mod folder, not absolute.
- **Encoding**: Lua files must be UTF-8 (BOM optional) or ASCII. MoonSharp handles UTF-8 unlike KopiLua.
- **MoonSharp DLL missing**: `tools/build-managed.bat` copies `MoonSharp.Interpreter.dll` to `build/managed/` and `RROML/`. If you build manually, reference `RROML/lib/MoonSharp.Interpreter.dll`.

## Versioning

- RROML v1.2.0 – initial Lua support.
- MoonSharp 2.0.0.0 (MIT, `lib/MoonSharp.Interpreter.dll` vendored).
