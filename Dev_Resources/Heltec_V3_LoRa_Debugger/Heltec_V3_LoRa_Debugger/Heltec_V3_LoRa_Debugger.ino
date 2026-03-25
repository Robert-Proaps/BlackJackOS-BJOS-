#include <RadioLib.h>
#include <heltec_unofficial.h>  // Already declares: SX1262 radio = new Module(SS, DIO1, RST_LoRa, BUSY_LoRa)

// NOTE: Do NOT redeclare 'radio' — heltec_unofficial.h does it for you

void setup() {
  Serial.begin(115200);
  heltec_setup();  // Initializes board peripherals (OLED, power pin, etc.)

  Serial.println("Initializing LoRa...");

  // begin(frequency, bandwidth, spreading factor, coding rate, sync word, output power, preamble length)
  int state = radio.begin(
    910.525,   // Frequency in MHz — change to 868.0 for EU
    62.5,   // Bandwidth in kHz
    7,       // Spreading factor (SF7–SF12)
    5,       // Coding rate (5 = 4/5)
    RADIOLIB_SX126X_SYNC_WORD_PUBLIC,
    17,      // TX power in dBm
    8        // Preamble length
  );

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("LoRa init failed, code: ");
    Serial.println(state);
    while (true);
  }

  // Write 0x00 to match packets regardless of sync word
  radio.setSyncWord(0x2B);

  Serial.println("LoRa ready — listening for packets...");
}

void loop() {
  heltec_loop();  // Required to keep board power management happy

  String received = "";
  int state = radio.receive(received);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("=== Packet Received ===");
    Serial.print("Payload:  ");
    Serial.println(received);
    Serial.print("RSSI:     ");
    Serial.print(radio.getRSSI());
    Serial.println(" dBm");
    Serial.print("SNR:      ");
    Serial.print(radio.getSNR());
    Serial.println(" dB");
    Serial.print("Freq err: ");
    Serial.print(radio.getFrequencyError());
    Serial.println(" Hz");
    Serial.println("=======================");

  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("CRC error — corrupted packet:");
    Serial.println(received);

  }
  // All other states (timeout, etc.) silently ignored
}