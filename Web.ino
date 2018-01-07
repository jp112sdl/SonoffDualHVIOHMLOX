const char HTTP_TITLE_LABEL[] PROGMEM = "<div class='l lt'><label>{v}</label><hr /></div>";
const char HTTP_CURRENT_STATE_LABEL[] PROGMEM = "<div class='l ls'> <label id='_ls2'>{ls1}</label> <label id='_ls2'>{ls2}</label> </div>";
const char HTTP_FW_LABEL[] PROGMEM = "<div class='l c k'><label>Firmware: {fw}</label></div>";
const char HTTP_HOME_BUTTON[] PROGMEM = "<div><input class='lnkbtn' type='button' value='Zur&uuml;ck' onclick=\"window.location.href='/'\" /></div>";
const char HTTP_SAVE_BUTTON[] PROGMEM = "<div><button name='btnSave' value='1' type='submit'>Speichern</button></div>";
const char HTTP_CONF[] PROGMEM = "<div><label>{st}:</label></div><div><input type='text' id='ccuip' name='ccuip' pattern='((^|\\.)((25[0-5])|(2[0-4]\\d)|(1\\d\\d)|([1-9]?\\d))){4}$' maxlength=16 placeholder='{st}' value='{ccuip}'></div><div><label>Ger&auml;tename:</label></div><div><input type='text' id='devicename' name='devicename' pattern='[A-Za-z0-9_ -]+' placeholder='Ger&auml;tename' value='{dn}'></div>";
const char HTTP_CONF_LOX[] PROGMEM = "<div><label>UDP Port:</label></div><div><input type='text' id='lox_udpport' pattern='[0-9]{1,5}' maxlength='5' name='lox_udpport' placeholder='UDP Port' value='{udp}'></div>";
const char HTTP_STATUSLABEL[] PROGMEM = "<div class='l c'>{sl}</div>";

void initWebserver() {
  DEBUG("initWebServer()...");
  WebServer.on("/set", webSetRelay);
  WebServer.on("/10", []() {
    sendDefaultWebCmdReply();
    switchRelay(1, RELAYSTATE_OFF, TRANSMITSTATE);
  });
  WebServer.on("/11", []() {
    sendDefaultWebCmdReply();
    switchRelay(1, RELAYSTATE_ON, TRANSMITSTATE);
  });
  WebServer.on("/12", []() {
    sendDefaultWebCmdReply();
    switchRelay(1, RELAYSTATE_TOGGLE, TRANSMITSTATE);
  });
  WebServer.on("/20", []() {
    sendDefaultWebCmdReply();
    switchRelay(2, RELAYSTATE_OFF, TRANSMITSTATE);
  });
  WebServer.on("/21", []() {
    sendDefaultWebCmdReply();
    switchRelay(2, RELAYSTATE_ON, TRANSMITSTATE);
  });
  WebServer.on("/22", []() {
    sendDefaultWebCmdReply();
    switchRelay(2, RELAYSTATE_TOGGLE, TRANSMITSTATE);
  });
  WebServer.on("/getState", replyRelayState);
  WebServer.on("/bootConfigMode", setBootConfigMode);
  WebServer.on("/reboot", []() {
    WebServer.send(200, "text/plain", "rebooting");
    ESP.restart();
  });
  WebServer.on("/restart", []() {
    WebServer.send(200, "text/plain", "rebooting");
    ESP.restart();
  });
  WebServer.on("/reloadCUxD", []() {
    String ret = reloadCUxDAddress(TRANSMITSTATE);
    WebServer.send(200, "text/plain", ret);
  });
  WebServer.on("/version", versionHtml);
  WebServer.on("/firmware", versionHtml);
  WebServer.on("/config", configHtml);
  httpUpdater.setup(&WebServer);
  WebServer.onNotFound(defaultHtml);
  WebServer.begin();
  DEBUG("initWebServer()... DONE");
}

void webSetRelay() {
  bool _transmitstate = TRANSMITSTATE;
  byte _relaynum = 0;
  byte _relaystate = RELAYSTATE_OFF;
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "ts") {
        _transmitstate = WebServer.arg(i).toInt();
      }
      if (WebServer.argName(i) == "ch") {
        _relaynum = WebServer.arg(i).toInt();
      }
      if (WebServer.argName(i) == "state") {
        _relaystate = WebServer.arg(i).toInt();
      }
    }
  } else {
    DEBUG(F("webSwitchRelay()"));
  }
  switchRelay(_relaynum, _relaystate, _transmitstate);
  sendDefaultWebCmdReply();
}

void replyRelayState() {
  sendDefaultWebCmdReply();
}

void defaultHtml() {
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "btnR1") {
        //
      }
    }
  }

  String page = FPSTR(HTTP_HEAD);
  page += FPSTR(HTTP_ALL_STYLE);
  if (GlobalConfig.BackendType == BackendType_HomeMatic)
    page += FPSTR(HTTP_HM_STYLE);
  if (GlobalConfig.BackendType == BackendType_Loxone)
    page += FPSTR(HTTP_LOX_STYLE);
  page += FPSTR(HTTP_HEAD_END);

  page += FPSTR(HTTP_DEFAULTHTML);

  page.replace("{v}", GlobalConfig.DeviceName);
  page.replace("{ls1}", ((Relay1State == On) ? "AN" : "AUS"));
  page.replace("{ls2}", ((Relay2State == On) ? "AN" : "AUS"));

  String fwurl = FPSTR(GITHUB_REPO_URL);
  String fwjsurl = FPSTR(GITHUB_REPO_URL);
  fwurl.replace("api.", "");
  fwurl.replace("repos/", "");
  page.replace("{fwurl}", fwurl);

  page += F("</div><script>");
  page += FPSTR(HTTP_CUSTOMSCRIPT);
  page += FPSTR(HTTP_CUSTOMUPDATESCRIPT);
  page.replace("{fwjsurl}", fwjsurl);
  page.replace("{fw}", FIRMWARE_VERSION);

  page += F("</script></div></body></html>");
  WebServer.sendHeader("Content-Length", String(page.length()));
  WebServer.send(200, "text/html", page);
}

void configHtml() {
  bool sc = false;
  bool saveSuccess = false;
  bool showHMDevError = false;
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "btnSave")
        sc = (WebServer.arg(i).toInt() == 1);
      if (WebServer.argName(i) == "ccuip")
        strcpy(GlobalConfig.ccuIP, WebServer.arg(i).c_str());

      if (WebServer.argName(i) == "devicename")
        strcpy(GlobalConfig.DeviceName, WebServer.arg(i).c_str());
      if (WebServer.argName(i) == "lox_udpport")
        strcpy(LoxoneConfig.UDPPort, WebServer.arg(i).c_str());
    }
    if (sc) {
      saveSuccess = saveSystemConfig();
      if (GlobalConfig.BackendType == BackendType_HomeMatic) {
        String ch1 =  getStateCUxD(String(GlobalConfig.DeviceName) + ":1", "Address");
        String ch2 =  getStateCUxD(String(GlobalConfig.DeviceName) + ":2", "Address");
        if (ch1 != "null" && ch2 != "null") {
          showHMDevError = false;
          HomeMaticConfig.Channel1Name =  "CUxD." + ch1;
          HomeMaticConfig.Channel2Name =  "CUxD." + ch2;
        } else {
          showHMDevError = true;
        }
      }
    }
  }
  String page = FPSTR(HTTP_HEAD);

  //page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_ALL_STYLE);
  if (GlobalConfig.BackendType == BackendType_HomeMatic)
    page += FPSTR(HTTP_HM_STYLE);
  if (GlobalConfig.BackendType == BackendType_Loxone)
    page += FPSTR(HTTP_LOX_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += F("<div class='fbg'>");
  page += F("<form method='post' action='config'>");
  page += FPSTR(HTTP_TITLE_LABEL);
  page += FPSTR(HTTP_CONF);

  if (GlobalConfig.BackendType == BackendType_HomeMatic) {
    page.replace("{st}", "CCU2 IP");
  }
  if (GlobalConfig.BackendType == BackendType_Loxone) {
    page += FPSTR(HTTP_CONF_LOX);
    page.replace("{st}", "MiniServer IP");
    page.replace("{udp}", LoxoneConfig.UDPPort);
    page.replace("{remanenz}", "Remanenzeingang");
  }

  page.replace("{dn}", GlobalConfig.DeviceName);
  page.replace("{ccuip}", GlobalConfig.ccuIP);

  page += FPSTR(HTTP_STATUSLABEL);

  if (sc && !showHMDevError) {
    if (saveSuccess) {
      page.replace("{sl}", F("<label class='green'>Speichern erfolgreich.</label>"));
    } else {
      page.replace("{sl}", F("<label class='red'>Speichern fehlgeschlagen.</label>"));
    }
  }

  if (showHMDevError)
    page.replace("{sl}", "<label class='red'>Ger&auml;tenamen in CUxD pr&uuml;fen!<br>Es m&uuml;ssen 2 Ger&auml;te existieren:<br>" + String(GlobalConfig.DeviceName) + ":1<br>" + String(GlobalConfig.DeviceName) + ":2</label>");

  if (!sc && !showHMDevError)
    page.replace("{sl}", "");

  page += FPSTR(HTTP_SAVE_BUTTON);
  page += FPSTR(HTTP_HOME_BUTTON);
  page += FPSTR(HTTP_FW_LABEL);
  page.replace("{fw}", FIRMWARE_VERSION);

  page += F("</form></div>");
  page += F("</div></body></html>");
  page.replace("{v}", GlobalConfig.DeviceName);

  WebServer.send(200, "text/html", page);
}

void sendDefaultWebCmdReply() {
  String reply = createReplyString();
  DEBUG("Sending Web-Reply: " + reply);
  WebServer.send(200, "application/json", reply);
}

String createReplyString() {
  return "{\"relay1\": " + String(Relay1State) + ", \"relay2\": " + String(Relay2State) + "}";
}

void versionHtml() {
  WebServer.send(200, "text/plain", "<fw>" + FIRMWARE_VERSION + "</fw>");
}

