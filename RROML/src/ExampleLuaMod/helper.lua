local helper = {}
helper.greeting = "Hello from helper.lua!"
helper.version = "1.0.0"

function helper.sayHello(name)
    return "Hello, " .. tostring(name) .. " from helper!"
end

return helper
