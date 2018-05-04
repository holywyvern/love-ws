# love-ws

A löve lua library to create websocket clients and servers.

## Dependencies

- Boost
- OpenSSL
- CMake
- Lua

## Building

```
mkdir build
cd build
cmake ..
```

### Notes for windows users

Remeber than cmake's find package sometimes requires you to config your 
environment variables.

I personally had to add the following:

- BOOST_INCLUDEDIR
- BOOST_LIBRARYDIR
- BOOST_ROOT
- OPENSSL_INCLUDE_DIR
- OPENSSL_LIBRARIES
- OPENSSL_ROOT_DIR

You will also need a visual studio version from 2013, 2015 or 2017.
I tested on both 2015 and 2017.

If you build this way, you may end up having only a 32 bits version of the dll.
I'll have to investigate on how to get a x64 version easily.

#### Building for Windows 64 bits

You may need a bit of tools to compile, I'm testing it with Visual Studio 2015, but it should
work on other Visual Studio versions.

So first go ahead and install Visual Studio 2015 express.

The CMake command I use for generating 64 bit builds is the following:

```
cmake -G "Visual Studio 14 2015 Win64" .
```

##### For building lua 5.1 for 64 bits

- Got the lua sources (https://www.lua.org/source/)
- Added this on the Lua build (https://gist.github.com/squeek502/76fb065848897138a95d11f9aa0eedd4)
- Make with Cmake

##### For building openssl for 64 bits

- I got the openssl sources (https://www.openssl.org/source/)
- Installed ActivePerl
- Installed NASM (https://www.nasm.us/pub/nasm/releasebuilds/2.13.03/win64/)

Followed instructions on INSTALL file:

With the visual studio developer command line tools open:

- perl Configure { VC-WIN32 | VC-WIN64A | VC-WIN64I | VC-CE }
- nmake

##### Building all together

- TODO

### Notes on non windows users

I didn't try to build this library on other platforms right now.
But it's using CMake and I'm only using the standard library, so it may work easily.

## Usage

You just require this dll like any other module:

```lua
local ws = require("ws")
```

Then you must create a server or a client:

### Client

```lua
local client = ws.newClient("url_or_ip:port/ns")
```

ws.newClient only takes 1 argument: the connection url.
The url may contain a host or ip address, a port and a namespace.

For example:

```lua
local client = ws.newClient("localhost:8080/game")
```


Creates a client than will connect to localhost (the local machine) at port 8080 on the "game"
namespace.

If you need to connect using TLS for security reasons,
you can also create a secure version of the client socket.

```lua
local client = ws.newTlsClient("localhost:8080/game", true)
```

The second argument (false by default) is if you should validate
the certificate of the server.

On development you may want to use self signed certificates,
if that's the case, put it to false, However, you should 
always put it as true in production.

Then you should start the server:

```lua
client:connect()
```

After that, the client will listen on a separate thread, you may do it in a loop:

```lua
while true do 
    local ev = client:checkQueue()
    if ev then
        --- an event happened
    end
end

```
An event is a table with 2 parameters:

- **type**: One of "message", "open", "close", "error", "connecting"
- **message**: A string, with the message attached. On error, this is the error message.

Type indicates wich action is taking place:

- **open**: The client is connected to the server
- **close**: The client closed a connection to the server
- **error**: An error happened
- **connecting**: The client is trying to connect to the server
- **message**: The server sent a message to the client

You can also send a message to the server, using `client:sendMessage("my message")`

### Server

```lua
local server = ws.newServer(port)
```

The server works almost like the client, but unlike the client it can listen into different channels:
You can open a channel by doing the following:

```lua
local channel = server:getChannel("^/test/?$")
```

server:getChannel(pattern) takes a pattern as a parameter, this parameter is a regular expression.
So if you want to have a channel listening to multiple sources, you can do that.

After selecting your channels, you need to start the server:

```lua
server:start()
```

Then, like the clients, you listen to messages on the queue:

```lua
while true do 
    local ev = channel:checkQueue()
    if ev then
        --- an event happened
    end
end
```

An event, is a table with 3 fields:

- **connection**: A string, indicating an id used to identify the connection who send the message.
- **type**: One of "open", "close", "message", "connecting", "error"
- **message**: A string, with the message attached. On error, this is the error message.

Type indicates wich action is taking place:

- **open**: The client just opened the connection with the server
- **close**: The client closed the connection with the server
- **error**: An error happened on the client
- **connecting**: The client is trying to connect to the server
- **message**: The client send a message to the server

You can send a message to a client from the server, using `channel:send(connectionId, message)`
or you can broadcast to all users, using `channel:broadcast(message)`

## License

Apache 2.0

Thanks to Ole Christian Eidheim for the awesome simple-websocket-library.

## Features and TODO list

- [x] plain websocket server
- [x] plain websocket client
- [X] secure websocket client
- [X] provide simple examples
- [ ] TLS Server (Not sure)
- [ ] Cleanup of code
- [ ] Optimize methods
- [ ] Better documentation