#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "Comms.h"
#include "ADC.h"
#include "HardwareConfig.h"
#include "RX5808.h"
#include "HTTPserver.h"
#include "Bluetooth.h"

//#define BluetoothEnabled //uncomment this to use bluetooth (experimental, ble + wifi appears to cause issues)


WiFiClient client;
WiFiServer server(23);

WiFiUDP UDPserver;

#define MAX_SRV_CLIENTS 5
WiFiClient serverClients[MAX_SRV_CLIENTS];

void readADCs();

extern uint16_t ADCcaptime;

extern int ADC1value;
extern int ADC2value;
extern int ADC3value;
extern int ADC4value;

const char* ssid = "Chorus";
const char* password = "Chorus123";

int RSSIthresholds[4] = {3500, 3500, 3500, 3500};

volatile uint32_t LapTimes[NumRecievers][100];
volatile int LapTimePtr[NumRecievers] = {0, 0, 0, 0}; //Keep track of what lap we are up too
bool LapModeREL = true;  // lap move is realtive, ie lap is millis() difference from previous lap

uint32_t MinLapTime = 5000;  //this is in millis

void setup() {

  Serial.begin(115200);
  Serial.println("Booting....");
  InitHTTPserver();
  delay(1000);

  server.begin();
  server.setNoDelay(true);

  InitHardwarePins();
  ConfigureADC();

  InitSPI();
  //
  UDPserver.begin(9000);

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  setModuleChannelBand(0); // inits module with defaults
  delay(100);
  setModuleChannelBand(1);
  delay(100);
  setModuleChannelBand(2);
  delay(100);
  setModuleChannelBand(3);

#ifdef BluetoothEnabled
  SerialBT.begin("Chorus Laptimer SPP");
#endif
  //delay(5000);
  InitADCtimer();

}

void loop() {

  HandleServerUDP();
  SendCurrRSSIloop();
  HTTPserver.handleClient();
  dnsServer.processNextRequest();
#ifdef BluetoothEnabled
  HandleBluetooth();
#endif

}

void IRAM_ATTR CheckRSSIthresholdExceeded() {
  uint32_t CurrTime = millis();

  for (uint8_t i = 0; i < NumRecievers; i++) {
    if ( ADCvalues[i] > RSSIthresholds[i]) {

      if (CurrTime > (MinLapTime + LapTimes[i][LapTimePtr[i]])) {

        LapTimePtr[i] = LapTimePtr[i] + 1;
        LapTimes[i][LapTimePtr[i]] = CurrTime;

        sendLap(LapTimePtr[i], i);
      }
    }
  }
}




