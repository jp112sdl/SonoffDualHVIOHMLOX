bool setStateCUxD(String id, String value) {
  if (id.indexOf(".null.") == -1 && String(GlobalConfig.ccuIP) != "0.0.0.0") {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(HTTPTimeOut);
      id.replace(" ", "%20");
      String url = "http://" + String(GlobalConfig.ccuIP) + ":8181/cuxd.exe?ret=dom.GetObject(%22" + id + "%22).State(" + value + ")";
      DEBUG("setStateCUxD url: " + url, "setStateCUxD()", _slInformational);
      http.begin(url);
      int httpCode = http.GET();
      String payload = "";

      if (httpCode > 0) {
        DEBUG("HTTP " + id + " success", "setStateCUxD()", _slInformational);
        payload = http.getString();
      }
      if (httpCode != 200) {
        blinkLED(3);
        DEBUG("HTTP " + id + " failed with HTTP Error Code " + String(httpCode), "setStateCUxD()", _slError);
      }
      http.end();

      payload = payload.substring(payload.indexOf("<ret>"));
      payload = payload.substring(5, payload.indexOf("</ret>"));

      DEBUG("result: " + payload, "setStateCUxD()", (payload != "null") ? _slInformational : _slError);

      return (payload != "null");

    } else {
      DEBUG("setStateCUxD: WiFi.status() != WL_CONNECTED, trying to reconnect with doWifiConnect()", "setStateCUxD()", _slError);
      /*if (!doWifiConnect()) {
        DEBUG("setStateCUxD: doWifiConnect() failed.", "setStateCUxD()", _slError);
        //ESP.restart();
        }*/
    }
  } else return true;
}

String getStateCUxD(String id, String type) {
  if (id != "" && id.indexOf(".null.") == -1 && String(GlobalConfig.ccuIP) != "0.0.0.0") {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(HTTPTimeOut);
      id.replace(" ", "%20");
      String url = "http://" + String(GlobalConfig.ccuIP) + ":8181/cuxd.exe?ret=dom.GetObject(%22" + id + "%22)." + type + "()";
      DEBUG("getStateFromCUxD url: " + url, "getStateCUxD()", _slInformational);
      http.begin(url);
      int httpCode = http.GET();
      String payload = "error";
      if (httpCode > 0) {
        payload = http.getString();
      }
      if (httpCode != 200) {
        blinkLED(3);
        DEBUG("HTTP " + id + " fail", "getStateCUxD()", _slError);
      }
      http.end();

      payload = payload.substring(payload.indexOf("<ret>"));
      payload = payload.substring(5, payload.indexOf("</ret>"));
      DEBUG("result: " + payload, "getStateCUxD()", _slInformational);

      return payload;
    } else {
      DEBUG("getStateCUxD: WiFi.status() != WL_CONNECTED, trying to reconnect with doWifiConnect()", "getStateCUxD()", _slError);
      /*if (!doWifiConnect()) {
        DEBUG("getStateCUxD: doWifiConnect() failed.", "getStateCUxD()", _slError);
        //ESP.restart();
        }*/
    }
  } else return "null";
}

String reloadCUxDAddress(bool transmitState) {
  HomeMaticConfig.Channel1Name =  "CUxD." + getStateCUxD(String(GlobalConfig.DeviceName) + ":1", "Address");
  HomeMaticConfig.Channel2Name =  "CUxD." + getStateCUxD(String(GlobalConfig.DeviceName) + ":2", "Address");

  DEBUG("HomeMaticConfig.Channel1Name = " + HomeMaticConfig.Channel1Name);
  DEBUG("HomeMaticConfig.Channel2Name = " + HomeMaticConfig.Channel2Name);

  if (transmitState == TRANSMITSTATE)
    setStateCUxD(HomeMaticConfig.Channel1Name + ".SET_STATE", String(Relay1State));

  return "CUxD Address 1 = " + HomeMaticConfig.Channel1Name + "; CUxD Address 2 = " + HomeMaticConfig.Channel2Name;
}

