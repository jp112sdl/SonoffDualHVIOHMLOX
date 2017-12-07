#define JSONCONFIG_IP                     "ip"
#define JSONCONFIG_NETMASK                "netmask"
#define JSONCONFIG_GW                     "gw"
#define JSONCONFIG_CCUIP                  "ccuip"
#define JSONCONFIG_DEVICENAME             "devicename"
#define JSONCONFIG_LOXUDPPORT             "loxudpport"
#define JSONCONFIG_LOXUSERNAME            "loxusername"
#define JSONCONFIG_LOXPASSWORD            "loxpassword"
#define JSONCONFIG_BACKENDTYPE            "backendtype"
#define JSONCONFIG_MODEL                  "model"

bool loadSystemConfig() {
  DEBUG(F("loadSystemConfig mounting FS..."), "loadSystemConfig()", _slInformational);
  if (SPIFFS.begin()) {
    DEBUG(F("loadSystemConfig mounted file system"), "loadSystemConfig()", _slInformational);
    if (SPIFFS.exists("/" + configJsonFile)) {
      DEBUG(F("loadSystemConfig reading config file"), "loadSystemConfig()", _slInformational);
      File configFile = SPIFFS.open("/" + configJsonFile, "r");
      if (configFile) {
        DEBUG(F("loadSystemConfig opened config file"), "loadSystemConfig()", _slInformational);
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        DEBUG("Content of JSON Config-File: /" + configJsonFile, "loadSystemConfig()", _slInformational);
        if (GlobalConfig.Model == Model_HVIO) {
          json.printTo(Serial);
          Serial.println();
        }
        if (json.success()) {
          DEBUG("\nJSON OK", "loadSystemConfig()", _slInformational);
          ((json[JSONCONFIG_IP]).as<String>()).toCharArray(NetConfig.ip, IPSIZE);
          ((json[JSONCONFIG_NETMASK]).as<String>()).toCharArray(NetConfig.netmask, IPSIZE);
          ((json[JSONCONFIG_GW]).as<String>()).toCharArray(NetConfig.gw, IPSIZE);
          ((json[JSONCONFIG_CCUIP]).as<String>()).toCharArray(GlobalConfig.ccuIP, IPSIZE);
          ((json[JSONCONFIG_DEVICENAME]).as<String>()).toCharArray(GlobalConfig.DeviceName, VARIABLESIZE);

          //((json[JSONCONFIG_LOXUSERNAME]).as<String>()).toCharArray(LoxoneConfig.Username, VARIABLESIZE);
          //((json[JSONCONFIG_LOXPASSWORD]).as<String>()).toCharArray(LoxoneConfig.Password, VARIABLESIZE);
          ((json[JSONCONFIG_LOXUDPPORT]).as<String>()).toCharArray(LoxoneConfig.UDPPort, 10);

          GlobalConfig.BackendType = json[JSONCONFIG_BACKENDTYPE];
          GlobalConfig.Model = json[JSONCONFIG_MODEL];
          GlobalConfig.Hostname = "DualSwitch-" + String(GlobalConfig.DeviceName);
        } else {
          DEBUG(F("\nloadSystemConfig ERROR loading config"), "loadSystemConfig()", _slInformational);
        }
      }
      return true;
    } else {
      DEBUG("/" + configJsonFile + " not found.", "loadSystemConfig()", _slInformational);
      return false;
    }
    SPIFFS.end();
  } else {
    DEBUG(F("loadSystemConfig failed to mount FS"), "loadSystemConfig()", _slCritical);
    return false;
  }
}

bool saveSystemConfig() {
  SPIFFS.begin();
  DEBUG(F("saving config"), "saveSystemConfig()", _slInformational);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json[JSONCONFIG_IP] = NetConfig.ip;
  json[JSONCONFIG_NETMASK] = NetConfig.netmask;
  json[JSONCONFIG_GW] = NetConfig.gw;
  json[JSONCONFIG_CCUIP] = GlobalConfig.ccuIP;
  json[JSONCONFIG_DEVICENAME] = GlobalConfig.DeviceName;
  json[JSONCONFIG_BACKENDTYPE] = GlobalConfig.BackendType;
  //json[JSONCONFIG_LOXUSERNAME] = LoxoneConfig.Username;
  //json[JSONCONFIG_LOXPASSWORD] = LoxoneConfig.Password;
  json[JSONCONFIG_LOXUDPPORT] = LoxoneConfig.UDPPort;
  json[JSONCONFIG_MODEL] = GlobalConfig.Model;

  SPIFFS.remove("/" + configJsonFile);
  File configFile = SPIFFS.open("/" + configJsonFile, "w");
  if (!configFile) {
    DEBUG(F("failed to open config file for writing"), "saveSystemConfig()", _slCritical);
    return false;
  }

  if (GlobalConfig.Model == Model_HVIO) {
    json.printTo(Serial);
    Serial.println();
  }
  json.printTo(configFile);
  configFile.close();
  SPIFFS.end();
  return true;
}

void setBootConfigMode() {
  if (SPIFFS.begin()) {
    DEBUG(F("setBootConfigMode mounted file system"), "setBootConfigMode()", _slInformational);
    if (!SPIFFS.exists("/" + bootConfigModeFilename)) {
      File bootConfigModeFile = SPIFFS.open("/" + bootConfigModeFilename, "w");
      bootConfigModeFile.print("0");
      bootConfigModeFile.close();
      SPIFFS.end();
      DEBUG(F("Boot to ConfigMode requested. Restarting..."), "setBootConfigMode()", _slInformational);
      WebServer.send(200, "text/plain", F("<state>enableBootConfigMode - Rebooting</state>"));
      delay(500);
      ESP.restart();
    } else {
      WebServer.send(200, "text/plain", F("<state>enableBootConfigMode - FAILED!</state>"));
      SPIFFS.end();
    }
  }
}

