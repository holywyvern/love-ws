const https = require('https');
const fs = require('fs');
const path = require("path");

const WebSocket = require('ws');

// Remember to generate your keys and put it inside a keys folder.
// For this example, they can be self silgned.
// As an example, you may use this using OpenSSL:
// openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout private.pem -out public.pem
const server = https.createServer({
  cert: fs.readFileSync(path.join(__dirname, 'keys', 'public.pem')),
  key: fs.readFileSync(path.join(__dirname, 'keys', 'private.pem'))
});

const wss = new WebSocket.Server({ server });

wss.on('connection', function connection (ws) {
  console.log("User connected");
  ws.on('message', function message (msg) {
    console.log(`Message from user: ${msg}.`);
  });
});

server.listen(8080);