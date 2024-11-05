#include <MIDI.h>
#include <WiFi.h>

// Source https://randomnerdtutorials.com/esp32-web-server-arduino-ide/

const char* ap_ssid     = "EXP-MIDI-CONTROLLER";
const char* ap_password = "always42";
IPAddress ap_local_ip(192, 168, 1, 1);
IPAddress ap_gateway_ip(192, 168, 1, 254);
IPAddress ap_network_mask(255, 255, 255, 0);
long wifi_time_counter = 0;
bool wifi_is_on = false;
WiFiServer server(80);
String header;

// Source https://www.arduinoslovakia.eu/blog/2018/6/arduino-a-midi-out?lang=en

#define PIN_FOOTSWITCH 22 // D22 HIGH or LOW
#define PIN_POTENTIOMETER 34 // D34, ADC1, 0 to 3v3, 0 to 4096. See more at https://lastminuteengineers.com/esp32-basics-adc/

MIDI_CREATE_DEFAULT_INSTANCE();

const int numLeituras = 50; // Número de leituras para a média
int leituras[numLeituras];   // Array para armazenar as leituras
int soma = 0;
int indice = 0;
int media = 0;
int ultimoValor = 0;
int mediaCorrigida = 0;

int footswitch = 0;
int lastFootswitch = 0;

void setup() {
  // Serial.begin(115200);

  pinMode(PIN_FOOTSWITCH, INPUT_PULLUP);
  pinMode(PIN_POTENTIOMETER, INPUT_PULLUP);

  // WiFi.softAPConfig(ap_local_ip, ap_gateway_ip, ap_network_mask);
  // WiFi.softAPsetHostname("exp-midi-controller.net");
  // WiFi.softAP(ap_ssid, ap_password);
  // server.begin();

  MIDI.begin(MIDI_CHANNEL_OMNI);

  for (int i = 0; i < numLeituras; i++) {
    leituras[i] = 0;
  }

  footswitch, lastFootswitch = digitalRead(PIN_FOOTSWITCH);
}

void loop() {
  // Pot read
  soma = soma - leituras[indice];
  leituras[indice] = analogRead(PIN_POTENTIOMETER);
  soma = soma + leituras[indice];
  indice = (indice + 1) % numLeituras;
  media = soma / numLeituras;
  mediaCorrigida = map(media, 0, 4095, 0, 127);

  if (mediaCorrigida != ultimoValor) {
    MIDI.sendControlChange(11, mediaCorrigida, 1);

    ultimoValor = mediaCorrigida;
  }
  else {
    // nothing
  }


  //Switch read
  footswitch = digitalRead(PIN_FOOTSWITCH);
  if (lastFootswitch != footswitch) {
    if (footswitch == 1) {
      MIDI.sendControlChange(48, 127, 1);
    }
    else {
      MIDI.sendControlChange(48, 0, 1);
    }
    lastFootswitch = footswitch;
  }


  // WiFi stuff
  if (WiFi.softAPgetStationNum() >= 1) {
    wifi_time_counter = millis();
  }

  if (wifi_is_on && (millis() - wifi_time_counter >= 60000)) { // More than 1 minute without any clients, turnoff wifi
    WiFi.softAPdisconnect(true);
    wifi_is_on = false;
    // Serial.print("wifi is off!");
    delay(100);
  }

  if (wifi_is_on) {
    WiFiClient client = server.available();

    if (client) {    
      String currentLine = ""; 
      while (client.connected()) {  
        if (client.available()) {
          char c = client.read();
          // Serial.write(c); 
          header += c;

          if (c == '\n') { 
            if (currentLine.length() == 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // CSS to style the on/off buttons 
              // Feel free to change the background-color and font-size attributes to fit your preferences
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #555555;}</style></head>");
              
              // Web Page Heading
              client.println("<body><h1>ESP32 Web Server</h1>");

              client.println("</body></html>");
              break;


            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }
      header = "";
      client.stop();
    }
  }
  
  delay(1);

}
