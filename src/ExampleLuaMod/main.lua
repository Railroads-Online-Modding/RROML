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
