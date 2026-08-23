-- ExampleLuaMod - demonstrates RROML v1.2.0 Lua modding (MoonSharp Lua 5.2)
-- Entry: main.lua
-- API: global `rroml` (alias `RROML`) provided by loader
-- See docs/LUA_API.md for full reference

-- Optional metadata globals (manifest is preferred)
-- Id will be taken from mod.json, but you can also expose them here if mod.json absent
local MOD_NAME = "Example Lua Mod"
local MOD_VERSION = "1.0.0"

-- Helper module example: require("helper") will resolve to helper.lua in same folder
-- Uncomment to test inter-file requires:
-- local helper = require("helper")
-- if helper then
--     rroml.log.info("helper loaded: " .. tostring(helper.greeting))
-- end

function OnLoad(context)
    -- `context` is the same rroml table (passed for convenience)
    -- You can use either `rroml` global or `context` parameter

    rroml.log.info("[" .. MOD_NAME .. " v" .. MOD_VERSION .. "] OnLoad called")

    -- Paths exposed by RROML
    rroml.log.info("  Game root : " .. rroml.paths.gameRoot)
    rroml.log.info("  Loader path: " .. rroml.paths.loaderPath)
    rroml.log.info("  Mods path  : " .. rroml.paths.modsPath)
    rroml.log.info("  Mod root   : " .. rroml.paths.modRoot)
    rroml.log.info("  Config root: " .. rroml.paths.configRoot)
    rroml.log.info("  Mod id     : " .. rroml.mod.id)
    rroml.log.info("  Mod version: " .. rroml.mod.version)

    -- Config helpers
    local myConfig = rroml.getConfigPath("settings.json")
    local userConfig = rroml.getUserGameConfigPath("Engine.ini")
    rroml.log.info("  My config path  : " .. myConfig)
    rroml.log.info("  User game config: " .. userConfig)

    -- Example: write a simple config if not exists
    -- MoonSharp allows io.*, but using Lua standard library is allowed (sandbox is soft)
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

    -- Standard Lua features work (tables, loops, functions, coroutines, etc.)
    local sum = 0
    for i = 1, 5 do sum = sum + i end
    rroml.log.info("  Lua loop test sum 1..5 = " .. sum)

    -- Example string formatting
    local greeting = string.format("Hello from %s! RROML %s is running Lua %s", rroml.mod.id, rroml.version, _VERSION)
    rroml.log.info("  " .. greeting)
    print(greeting) -- print() is aliased to rroml.log.info

    rroml.log.info("[" .. MOD_NAME .. "] OnLoad finished")
end

-- Alternative return-table style (also supported). If you prefer that style, comment out
-- the global OnLoad above and uncomment below:
-- local mod = {}
-- mod.Id = "ExampleLuaMod"
-- mod.Name = MOD_NAME
-- mod.Version = MOD_VERSION
-- function mod.OnLoad(ctx)
--     ctx.log.info("Loaded via returned table OnLoad")
-- end
-- return mod
