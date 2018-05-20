/*
  Generic ESP8285 Module
  Flash Mode: DOUT
  Flash Frequency: 40 MHz
  CPU Frequency: 80 MHz
  Flash Size: 1M (64k SPIFFS)
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "WM.h"
#include <FS.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266Ping.h>
#include <ESP8266mDNS.h>
#include "css_global.h"
#include "js_global.h"
#include "js_fwupd.h"
#include "html_defaultHtml.h"

const String FIRMWARE_VERSION = "1.1.3";
//#define                       UDPDEBUG

#define LEDPinDual            13
//HVIO:
#define LEDPinHVIO            15
#define Relay1PinHVIO          4
#define Relay2PinHVIO          5
#define Switch1PinHVIO        12
#define Switch2PinHVIO        13

//Dual R2:
#define Relay1PinDualR2       12
#define Relay2PinDualR2        5
#define Switch1PinDualR2       0
#define Switch2PinDualR2       9
#define SwitchPinHeadDualR2   10

byte Switch1 = 0;
byte Switch2 = 0;
byte Relay1 = 0;
byte Relay2 = 0;

#define MillisKeyBounce      150
#define ConfigPortalTimeout  180  //Timeout (Sekunden) des AccessPoint-Modus
#define HTTPTimeOut         3000  //Timeout (Millisekunden) für http requests
#define IPSIZE                16
#define VARIABLESIZE         255
#define UDPPORT             6624
#define PING_ENABLED        false
#define PINGINTERVALSECONDS  300
#define KEYPRESSLONGMILLIS  1500 //Millisekunden für langen Tastendruck bei Sonoff Touch als Sender

const char GITHUB_REPO_URL[] PROGMEM = "https://api.github.com/repos/jp112sdl/SonoffDualHVIOHMLOX/releases/latest";

#ifdef UDPDEBUG
const char * SYSLOGIP = "192.168.1.251";
#define SYSLOGPORT          514
#endif

enum BackendTypes_e {
  BackendType_HomeMatic,
  BackendType_Loxone
};

enum Model_e {
  Model_Dual,
  Model_HVIO,
  Model_DualR2
};

enum RelayStates_e {
  RELAYSTATE_OFF,
  RELAYSTATE_ON,
  RELAYSTATE_TOGGLE
};

enum TransmitStates_e {
  NO_TRANSMITSTATE,
  TRANSMITSTATE
};

struct globalconfig_t {
  char ccuIP[IPSIZE]   = "";
  char DeviceName[VARIABLESIZE] = "";
  byte BackendType = BackendType_HomeMatic;
  byte Model = Model_Dual;
  String Hostname = "DualSwitch";
} GlobalConfig;

struct hmconfig_t {
  String Channel1Name = "";
  String Channel2Name = "";
} HomeMaticConfig;

struct loxoneconfig_t {
  char Username[VARIABLESIZE] = "";
  char Password[VARIABLESIZE] = "";
  char UDPPort[10] = "";
} LoxoneConfig;

struct netconfig_t {
  char ip[IPSIZE]      = "0.0.0.0";
  char netmask[IPSIZE] = "0.0.0.0";
  char gw[IPSIZE]      = "0.0.0.0";
} NetConfig;

enum _SyslogSeverity {
  _slEmergency,
  _slAlert,
  _slCritical,
  _slError,
  _slWarning,
  _slNotice,
  _slInformational,
  _slDebug
};

const String bootConfigModeFilename = "bootcfg.mod";
const String configJsonFile         = "config.json";
bool KeyPress = false;
byte LEDPin = 13;
byte On = 1;
byte Off = 0;
bool Relay1State = Off;
bool Relay2State = Off;
unsigned long LastMillisKeyPress = 0;
unsigned long KeyPressDownMillis = 0;
unsigned long LastWiFiReconnectMillis = 0;
bool OTAStart = false;
bool UDPReady = false;
bool WiFiConnected = false;
bool newFirmwareAvailable = false;
bool startWifiManager = false;
bool wm_shouldSaveConfig        = false;
#define wifiManagerDebugOutput   false

ESP8266WebServer WebServer(80);
ESP8266HTTPUpdateServer httpUpdater;

struct udp_t {
  WiFiUDP UDP;
  char incomingPacket[255];
} UDPClient;

void setup() {
  Serial.begin(19200);
  switch_dual_relay(0);
  Serial.println("\nSonoffDual (R2) / HVIO " + WiFi.macAddress() + " startet...");
  pinMode(LEDPinDual, OUTPUT);
  pinMode(LEDPinHVIO, OUTPUT);
  pinMode(Switch2PinHVIO, INPUT_PULLUP);
  pinMode(SwitchPinHeadDualR2, INPUT_PULLUP);

  Serial.println(F("Config-Modus durch bootConfigMode aktivieren? "));
  if (SPIFFS.begin()) {
    Serial.println(F("-> bootConfigModeFilename mounted file system"));
    if (SPIFFS.exists("/" + bootConfigModeFilename)) {
      startWifiManager = true;
      Serial.println("-> " + bootConfigModeFilename + " existiert, starte Config-Modus");
      SPIFFS.remove("/" + bootConfigModeFilename);
      SPIFFS.end();
    } else {
      Serial.println("-> " + bootConfigModeFilename + " existiert NICHT");
    }
  } else {
    Serial.println(F("-> Nein, SPIFFS mount fail!"));
  }

  if (!startWifiManager) {
    Serial.println(F("Config-Modus mit Taster aktivieren?"));
    Serial.flush();
    for (int i = 0; i < 20; i++) {
      if (digitalRead(SwitchPinHeadDualR2) == LOW || digitalRead(Switch2PinHVIO) == LOW || ButtonPressed()) {
        startWifiManager = true;
        break;
      }
      digitalWrite(LEDPinDual, HIGH);
      digitalWrite(LEDPinHVIO, LOW);
      delay(100);
      digitalWrite(LEDPinDual, LOW);
      digitalWrite(LEDPinHVIO, HIGH);
      delay(100);
    }
    Serial.println("Config-Modus " + String(((startWifiManager) ? "" : "nicht ")) + "aktiviert.");
  }

  if (!loadSystemConfig()) startWifiManager = true;
  //Ab hier ist die Config geladen und alle Variablen sind mit deren Werten belegt!

  if (doWifiConnect()) {
    Serial.println(F("\nWLAN erfolgreich verbunden!"));
    printWifiStatus();
  } else ESP.restart();

  switch (GlobalConfig.Model) {
    case Model_Dual:
      DEBUG("\nModell = Sonoff Dual");
      LEDPin = LEDPinDual;
      break;
    case Model_HVIO:
      DEBUG("\nModell = HVIO");
      LEDPin = LEDPinHVIO;
      pinMode(Relay1PinHVIO, OUTPUT);
      pinMode(Relay2PinHVIO, OUTPUT);
      pinMode(Switch1PinHVIO, INPUT_PULLUP);
      pinMode(Switch2PinHVIO, INPUT_PULLUP);
      Switch1 = Switch1PinHVIO;
      Switch2 = Switch2PinHVIO;
      Relay1 = Relay1PinHVIO;
      Relay2 = Relay2PinHVIO;
      break;
    case Model_DualR2:
      DEBUG("\nModell = DualR2");
      LEDPin = LEDPinDual;
      pinMode(Relay1PinDualR2, OUTPUT);
      pinMode(Relay2PinDualR2, OUTPUT);
      pinMode(Switch1PinDualR2, INPUT_PULLUP);
      pinMode(Switch2PinDualR2, INPUT_PULLUP);
      pinMode(SwitchPinHeadDualR2, INPUT_PULLUP);
      Switch1 = Switch1PinDualR2;
      Switch2 = Switch2PinDualR2;
      Relay1 = Relay1PinDualR2;
      Relay2 = Relay2PinDualR2;
  }
  pinMode(LEDPin, OUTPUT);

  initWebserver();

  if (GlobalConfig.BackendType == BackendType_HomeMatic) {
    reloadCUxDAddress(NO_TRANSMITSTATE);
  }

  startOTAhandling();

  if (!MDNS.begin(GlobalConfig.Hostname.c_str())) {
    DEBUG("Error setting up MDNS responder!");
  }

  DEBUG("Starte UDP-Handler an Port " + String(UDPPORT) + "...");
  UDPClient.UDP.begin(UDPPORT);
  UDPReady = true;
  DEBUG(String(GlobalConfig.DeviceName) + " - Boot abgeschlossen, SSID = " + WiFi.SSID() + ", IP = " + String(IpAddress2String(WiFi.localIP())) + ", RSSI = " + WiFi.RSSI() + ", MAC = " + WiFi.macAddress(), "Setup", _slInformational);
}

void loop() {
  if (LastMillisKeyPress > millis())
    LastMillisKeyPress = millis();
  if (KeyPressDownMillis > millis())
    KeyPressDownMillis = millis();
  if (LastWiFiReconnectMillis > millis())
    LastWiFiReconnectMillis = millis();

  //Reconnect WiFi wenn nicht verbunden (alle 30 Sekunden)
  if (WiFi.status() != WL_CONNECTED) {
    WiFiConnected = false;
    if (millis() - LastWiFiReconnectMillis > 30000) {
      LastWiFiReconnectMillis = millis();
      DEBUG("WiFi Connection lost! Reconnecting...");
      WiFi.reconnect();
    }
  } else {
    if (!WiFiConnected) {
      DEBUG("WiFi reconnected!");
      WiFiConnected = true;
    }
  }

  //auf OTA Anforderung reagieren
  ArduinoOTA.handle();

  if (!OTAStart) {

    //eingehende UDP Kommandos abarbeiten
    handleUDP();

    //eingehende HTTP Anfragen abarbeiten
    WebServer.handleClient();

    //Tasterbedienung Taster abarbeiten
    if (GlobalConfig.Model == Model_HVIO || GlobalConfig.Model == Model_DualR2) {
      if (digitalRead(Switch1) == LOW || digitalRead(Switch2) == LOW) {
        if (!KeyPress) {
          if (digitalRead(Switch1) == LOW) switchRelay(1, RELAYSTATE_TOGGLE, TRANSMITSTATE);
          if (digitalRead(Switch2) == LOW) switchRelay(2, RELAYSTATE_TOGGLE, TRANSMITSTATE);
          KeyPress = true;
        }
      } else {
        KeyPress = false;
      }
    }
  }

  delay(10);
}

void switchRelay(byte RelayNum, byte toState, bool transmitState) {
  DEBUG("Switch Relay Num " + String(RelayNum) + " to " + String(toState) + " with transmitState = " + String(transmitState), "switchRelay()", _slInformational);

  bool oldState1 = Relay1State;
  bool oldState2 = Relay2State;

  switch (RelayNum) {
    case 1:
      if (toState == RELAYSTATE_TOGGLE)
        Relay1State = !Relay1State;
      else Relay1State = toState;
      break;
    case 2:
      if (toState == RELAYSTATE_TOGGLE)
        Relay2State = !Relay2State;
      else Relay2State = toState;
      break;
    default:
      DEBUG("Wrong RelayNum", "switchRelay()", _slError);
      break;
  }

  byte sum = 0;
  switch (GlobalConfig.Model) {
    case Model_Dual:
      sum = Relay1State;
      sum =  sum + ((Relay2State == Off) ? 0 : 2);
      switch_dual_relay(sum);
      break;
    case Model_HVIO:
      digitalWrite(Relay1, Relay1State);
      digitalWrite(Relay2, Relay2State);
      break;
    case Model_DualR2:
      digitalWrite(Relay1, Relay1State);
      digitalWrite(Relay2, Relay2State);
      break;
  }

  if (transmitState) {
    if (GlobalConfig.BackendType == BackendType_HomeMatic) {
      if (oldState1 != Relay1State) setStateCUxD(HomeMaticConfig.Channel1Name + ".SET_STATE", String(Relay1State));
      if (oldState2 != Relay2State) setStateCUxD(HomeMaticConfig.Channel2Name + ".SET_STATE", String(Relay2State));
    }
    if (GlobalConfig.BackendType == BackendType_Loxone) {
      if (oldState1 != Relay1State) sendLoxoneUDP(String(GlobalConfig.DeviceName) + ":1 =" + String(Relay1State));
      if (oldState2 != Relay2State) sendLoxoneUDP(String(GlobalConfig.DeviceName) + ":2 =" + String(Relay2State));
    }
  }
}

void blinkLED(int count) {
  byte oldState = digitalRead(LEDPin);
  delay(100);
  for (int i = 0; i < count; i++) {
    digitalWrite(LEDPin, !oldState);
    delay(100);
    digitalWrite(LEDPin, oldState);
    delay(100);
  }
  delay(200);
}

String IpAddress2String(const IPAddress& ipAddress) {
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3]);
}
