local ws = require("ws")

local server = ws.newServer(8080)

local channel = server:getChannel("^/test/?$")

server:start()

while true do 
    local ev = channel:checkQueue()
    if ev then 
        print("connection '" .. ev.connection .. "' " .. ev.type .. " " .. ev.message)
        if ev.type == "message" then 
            channel:send(ev.connection, ev.message);
        end
    end
end