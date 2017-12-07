void switch_dual_relay(byte rel) {
  DEBUG("Schalte Relais " + String(rel));
  Serial.write(0xA0);
  Serial.write(0x04);
  Serial.write(rel);
  Serial.write(0xA1);
  Serial.write('\n');
  Serial.flush();  
}

byte serial_in_byte;                        // Received byte
byte dual_hex_code = 0;                     // Sonoff dual input flag
uint16_t dual_button_code = 0;

bool ButtonPressed() {
  while (Serial.available()) {
    yield();
    serial_in_byte = Serial.read();
    if (dual_hex_code) {
      dual_hex_code--;
      if (dual_hex_code) {
        dual_button_code = (dual_button_code << 8) | serial_in_byte;
        serial_in_byte = 0;

      } else {
        if (serial_in_byte != 0xA1) {
          dual_button_code = 0;
        }
      }
    }
    if (0xA0 == serial_in_byte) {
      serial_in_byte = 0;
      dual_button_code = 0;
      dual_hex_code = 3;
      return true;
    }
  }
  return false;
}
