/*
  Webserver.ino
  created 2025-04-27
  by Christian Rassmann


  https://github.com/ESP32Async/AsyncTCP/releases
  https://github.com/ESP32Async/ESPAsyncWebServer/releases
  https://github.com/me-no-dev/arduino-esp32fs-plugin/releases/

*/

#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static const char *htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html lang="en">
      
<head>
    <title>DL2RN | WSPR-ESP32 Beacon</title>
    <link rel="icon" href="data:,">
    <style>
        body {
          background-color: black;
          color: limegreen;
          font-family: monospace;
          padding: 20px;
        }

        #messages {
          margin-top: 20px;
        }

        #messageInput, #sendButton {
          background-color: #111;
          color: limegreen;
          border: 1px solid limegreen;
          padding: 8px;
          margin-right: 5px;
        }

        #messageInput::placeholder {
          color: #4caf50;
        }

        #sendButton:hover {
          background-color: #222;
          cursor: pointer;
        }

        #messageInput.sent {
          background-color: limegreen;
          color: black;
        }

        #connectionStatus {
          font-size: 18px;
          font-weight: bold;
          margin-bottom: 10px;
          text-align: center;
        }

        .message {
          opacity: 0;
          animation: fadeIn 0.8s forwards;
        }

        .timestamp {
          color: #4caf50;
        }

        .text {
          color: limegreen;
        }

        @keyframes fadeIn {
          from { opacity: 0; }
          to { opacity: 1; }
        }
    </style>
</head>

<body>
   <div id="connectionStatus">Disconnected</div>
   <h3><pre>
                              ______ _      _____ ______ _   _                                    
 ______ ______ ______ ______  |  _  \ |    / __  \| ___ \ \ | |  ______ ______ ______ ______      
|______|______|______|______| | | | | |    `' / /'| |_/ /  \| | |______|______|______|______|     
 ______ ______ ______ ______  | | | | |      / /  |    /| . ` |  ______ ______ ______ ______      
|______|______|______|______| | |/ /| |____./ /___| |\ \| |\  | |______|______|______|______|     
                              |___/ \_____/\_____/\_| \_\_| \_/                                   
 _    _ _________________       _____ ___________ _____  _____  ______                            
| |  | /  ___| ___ \ ___ \     |  ___/  ___| ___ \____ |/ __  \ | ___ \                           
| |  | \ `--.| |_/ / |_/ /_____| |__ \ `--.| |_/ /   / /`' / /' | |_/ / ___  __ _  ___ ___  _ __  
| |/\| |`--. \  __/|    /______|  __| `--. \  __/    \ \  / /   | ___ \/ _ \/ _` |/ __/ _ \| '_ \ 
\  /\  /\__/ / |   | |\ \      | |___/\__/ / |   .___/ /./ /___ | |_/ /  __/ (_| | (_| (_) | | | |
 \/  \/\____/\_|   \_| \_|     \____/\____/\_|   \____/ \_____/ \____/ \___|\__,_|\___\___/|_| |_|
   </pre></h3>
   <h3>Commands: [start, stop, status, trigger4, trigger10, trigger20]</h3>
   <input type="text" id="messageInput" placeholder="Type a message..." />
   <button id="sendButton">Send</button>
   <div id="connectionStatus">
   </div>
   <div id="messages">
   </div>

   <script>
      const socket = new WebSocket(`ws://${window.location.hostname}/ws`);

      socket.addEventListener('open', () => {
        console.log('Connected to server');
        updateConnectionStatus(true);
      });

      socket.addEventListener('close', () => {
        console.log('Disconnected from server');
        updateConnectionStatus(false);
      });

      socket.addEventListener('message', (event) => {
        const messageDiv = document.createElement('div');
        messageDiv.classList.add('message');

        const data = JSON.parse(event.data);

        const timestampSpan = document.createElement('span');
        timestampSpan.classList.add('timestamp');
        timestampSpan.textContent = `[${data.timestamp}] `;

        const textSpan = document.createElement('span');
        textSpan.classList.add('text');
        textSpan.textContent = data.message;

        messageDiv.appendChild(timestampSpan);
        messageDiv.appendChild(textSpan);
        
        document.getElementById('messages').appendChild(messageDiv);
        scrollMessagesToBottom();
      });

      document.getElementById('messageInput').addEventListener('keydown', (event) => {
        if (event.key === 'Enter') {
            event.preventDefault();
            sendMessage();
        }
      });

      document.getElementById('sendButton').addEventListener('click', () => {
        sendMessage();
      });

      function sendMessage() {
        const messageInput = document.getElementById('messageInput');
        const message = messageInput.value;
        if (message.trim() !== '') {
            socket.send(message);
            messageInput.value = '';

            messageInput.classList.add('sent');
            setTimeout(() => {
              messageInput.classList.remove('sent');
            }, 300);
            messageInput.focus();
        }
      }

      function scrollMessagesToBottom() {
        const messagesDiv = document.getElementById('messages');
        const isAtBottom = messagesDiv.scrollHeight - messagesDiv.clientHeight <= messagesDiv.scrollTop + 1;
        if (isAtBottom) {
            messagesDiv.scrollTop = messagesDiv.scrollHeight;
        }
      }

      function updateConnectionStatus(isConnected) {
        const statusDiv = document.getElementById('connectionStatus');
        if (isConnected) {
            statusDiv.textContent = 'Connected';
            statusDiv.style.color = 'limegreen';
        } else {
            statusDiv.textContent = 'Disconnected';
            statusDiv.style.color = 'red';
        }
      }
   </script>
</body>
</html>
)";

void webserver_setup() {

  LittleFS.begin(true);

  {
    File f = LittleFS.open("/index.html", "w");
    assert(f);
    f.print(htmlContent);
    f.close();
  }

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;

    if (type == WS_EVT_CONNECT) {
      Serial.println("ws connect");
      client->setCloseClientOnQueueFull(false);
      for (int i = 0; i < messageCount; i++)
      {
        int index = (messageStart + i) % MAX_MESSAGES;
        String json = messageBuffer[index];
        ws_sendAll(json);
      }
    } else if (type == WS_EVT_DISCONNECT) {
      Serial.println("ws disconnect");

    } else if (type == WS_EVT_ERROR) {
      Serial.println("ws error");

    } else if (type == WS_EVT_PONG) {
      Serial.println("ws pong");

    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      Serial.printf("index: %" PRIu64 ", len: %" PRIu64 ", final: %" PRIu8 ", opcode: %" PRIu8 "\n", info->index, info->len, info->final, info->opcode);
      String msg = "";
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
          msg = (char *)data;
          if (msg == "start") {
            active = true;
            log("Beacon activated ...");
          }
          else if (msg == "stop") {
            active = false;
            log("Beacon deactivated ...");
          }
          else if (msg == "trigger4") {
            trigger_every_x_minutes = 4;
            log("Trigger set every 4 minutes ...");
          }
          else if (msg == "trigger10") {
            trigger_every_x_minutes = 10;
            log("Trigger set every 10 minutes ...");
          } 
          else if (msg == "trigger20") {
            trigger_every_x_minutes = 20;
            log("Trigger set every 20 minutes ...");
          }
          else if (msg == "status") {
            log("SI5351 status: " + String(si5351_get_status()));
            log("Actual time: " + String(printTime()));
            log("Beacon active: " + String(active ? "true" : "false"));
            log("Beacon trigger every " + String(trigger_every_x_minutes) + " minutes");
          } 
          else {
            log(msg);
          }
        }
      }
    }
  });

  // shows how to prevent a third WS client to connect
  server.addHandler(&ws).addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    // ws.count() is the current count of WS clients: this one is trying to upgrade its HTTP connection
    if (ws.count() > 1) {
      // if we have 2 clients or more, prevent the next one to connect
      request->send(503, "text/plain", "Server is busy");
    } else {
      // process next middleware and at the end the handler
      next();
    }
  });

  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
  });

  server.serveStatic("/index.html", LittleFS, "/index.html");

  server.begin();
}

void ws_sendAll(String payload) {
  if (ws.count() > 0) {
    ws.textAll(payload);
  }
}