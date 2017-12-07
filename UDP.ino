void handleUDP() {
  int packetSize = UDPClient.UDP.parsePacket();
  if (packetSize) {
    DEBUG("Received " + String(packetSize) + " bytes from " + UDPClient.UDP.remoteIP().toString() + ", port " + String(UDPClient.UDP.remotePort()));
    int len = UDPClient.UDP.read(UDPClient.incomingPacket, 255);
    if (len > 0)
      UDPClient.incomingPacket[len] = 0;

    DEBUG("UDP packet contents: " + String(UDPClient.incomingPacket));

    UDPClient.UDP.beginPacket(UDPClient.UDP.remoteIP(), UDPClient.UDP.remotePort());
    String replyMsg = createReplyString();
    UDPClient.UDP.write(replyMsg.c_str());
    UDPClient.UDP.endPacket();

    String message = String(UDPClient.incomingPacket);
    message.trim();
    message.toUpperCase();
    if (message == "BOOTCONFIGMODE")
      setBootConfigMode;
    if (message == "REBOOT" || message == "RESTART")
      ESP.restart();
    if (message.length() == 2 && message.toInt() >= 10) {
      int RelayNum = message.substring(0, 1).toInt();
      int RelayState = message.substring(1, 2).toInt();
      switchRelay(RelayNum, RelayState, NO_TRANSMITSTATE);
    }
    /*if (message == "10")
      switchRelay(1, RELAYSTATE_OFF, NO_TRANSMITSTATE);
      if (message == "11")
      switchRelay(1, RELAYSTATE_ON, NO_TRANSMITSTATE);
      if (message == "12")
      switchRelay(1, RELAYSTATE_TOGGLE, NO_TRANSMITSTATE);
      if (message == "20")
      switchRelay(2, RELAYSTATE_OFF, NO_TRANSMITSTATE);
      if (message == "21")
      switchRelay(2, RELAYSTATE_ON, NO_TRANSMITSTATE);
      if (message == "22")
      switchRelay(2, RELAYSTATE_TOGGLE, NO_TRANSMITSTATE);*/
  }
}
