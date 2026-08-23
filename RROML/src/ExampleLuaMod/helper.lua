-- helper.lua - optional module required by main.lua via require("helper")
-- Demonstrates Lua require / module pattern with RROML's FileSystem loader

local helper = {}
helper.greeting = "Hello from helper.lua!"
helper.version = "1.0.0"

function helper.sayHello(name)
    return "Hello, " .. tostring(name) .. " from helper!"
end

return helper
