local ws = require("ws")

local client = ws.newClient("localhost:8080/test")

client:connect()

local connected = true

while connected do 
    local ev = client:checkQueue()
    if ev then 
        print(ev.type .. ": " .. ev.message)
        if ev.type == "error" or ev.type == "close" then 
            connected = false
        end
        if ev.type == "open" then 
            client:sendMessage("hello")
        end
    end
end

print("Connection closed")