#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const int PIN_LED = 2;
static const int PIN_VENT = 14;

static String lastProcessedMessage = "";

static String urlEncodeUtf8(const String &value) {
  String out;
  out.reserve(value.length() * 3);
  for (size_t i = 0; i < value.length(); ++i) {
    uint8_t ch = static_cast<uint8_t>(value[i]);
    if (ch == ' ') {
      out += '+';
    } else if (ch > 32 && ch < 128 && ch != '?' && ch != '=') {
      out += static_cast<char>(ch);
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%02X", ch);
      out += '%';
      out += buf;
    }
  }
  return out;
}

static bool httpsGet(const String &url, String &payload, int &code) {
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient http;
  if (!http.begin(client, url)) {
    code = -1;
    return false;
  }
  http.setTimeout(15000);
  http.addHeader("Accept", "application/json");
  
  code = http.GET();
  if (code > 0) {
    payload = http.getString();
  } else {
    payload = "";
  }
  http.end();
  return code > 0;
}

void controlLED(const String &message, const String &sender) {
  String msg = message;
  msg.toLowerCase();
  msg.trim();
  
  Serial.print("От: ");
  Serial.print(sender);
  Serial.print(" | Команда: ");
  Serial.println(msg);
  
  if (msg.indexOf("включи свет") >= 0 || msg.indexOf("включи светодиод") >= 0 || 
      msg.indexOf("включить свет") >= 0 || msg.indexOf("свет вкл") >= 0) {
    digitalWrite(PIN_LED, HIGH);
    Serial.println("✓ Светодиод включен (GPIO 2 - встроенный)");
  } 
  else if (msg.indexOf("выключи свет") >= 0 || msg.indexOf("выключи светодиод") >= 0 ||
           msg.indexOf("выключить свет") >= 0 || msg.indexOf("свет выкл") >= 0) {
    digitalWrite(PIN_LED, LOW);
    Serial.println("✓ Светодиод выключен (GPIO 2 - встроенный)");
  }
  else if (msg.indexOf("включи вентилятор") >= 0 || msg.indexOf("вентилятор вкл") >= 0) {
    digitalWrite(PIN_VENT, HIGH);
    Serial.println("✓ Вентилятор включен (GPIO 14)");
  }
  else if (msg.indexOf("выключи вентилятор") >= 0 || msg.indexOf("вентилятор выкл") >= 0) {
    digitalWrite(PIN_VENT, LOW);
    Serial.println("✓ Вентилятор выключен (GPIO 14)");
  }
  else {
    Serial.println("⚠ Команда не распознана. Доступные: включи/выключи свет, включи/выключи вентилятор");
  }
}

void runN8nDemo() {
  const char *ssid = "Samsung_IoT";
  const char *password = "IOT5iot5";
  
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Подключение к Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n✓ Wi-Fi подключен!");
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());
  }
  
  String url2 = String("https://n8n.levandrovskiy.ru/get_recent_messages?chat_name=v6&limit=1&sender=user");
  String payload2;
  int code2 = 0;
  
  if (!httpsGet(url2, payload2, code2)) {
    Serial.printf("get_recent_messages: HTTP ошибка: %d\n", code2);
    return;
  }
  
  Serial.printf("get_recent_messages: статус: %d\n", code2);
  
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload2);
  if (err) {
    Serial.print("JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }
  
  String content = "";
  String sender = "unknown";
  
  if (doc["content"].is<String>()) {
    content = doc["content"].as<String>();
    if (doc["sender"].is<String>()) {
      sender = doc["sender"].as<String>();
    }
  }
  else if (doc.is<JsonArray>()) {
    JsonArray arr = doc.as<JsonArray>();
    if (!arr.isNull() && arr.size() > 0) {
      JsonVariant first = arr[0];
      if (!first.isNull()) {
        if (first["content"].is<String>()) {
          content = first["content"].as<String>();
        }
        if (first["sender"].is<String>()) {
          sender = first["sender"].as<String>();
        }
      }
    }
  }
  
  if (content.length() > 0) {
    String messageID = content + "_" + sender;
    if (messageID != lastProcessedMessage) {
      Serial.println("\n--- Новое сообщение ---");
      Serial.print("От: ");
      Serial.println(sender);
      Serial.print("Текст: ");
      Serial.println(content);
      
      controlLED(content, sender);
      
      lastProcessedMessage = messageID;
      Serial.println("--- Обработано ---\n");
    } else {
      Serial.println("(сообщение уже обработано)");
    }
  } else {
    Serial.println("Поле 'content' не найдено в ответе");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_VENT, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_VENT, LOW);
}

void loop() {
  runN8nDemo();
  delay(3000);
}