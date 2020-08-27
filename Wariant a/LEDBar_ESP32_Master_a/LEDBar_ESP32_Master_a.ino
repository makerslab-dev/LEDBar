
// //Wczytywanie biblioteki od WiFi, ESP_NOW i NeoPixel odpowiedzialnej za wyświetlanie ledow
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>

// SSID i hasło rozgłaszanej sieci WiFi
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

//PIN do którego podłączony jest pasek ledów
#define PIN  4 

//Ilość ledów na pasku
#define NUMPIXELS 86
//Maksymalna ilośc slaveów ilu wyszukuje master podczas rozgłaszania kolorów. Maksymalnie można podać 20.
#define NUMSLAVES 5
esp_now_peer_info_t slaves[NUMSLAVES] = {};
int SlaveCnt = 0;
#define CHANNEL 3
#define PRINTSCANRESULTS 0

//Stworzenie obiektu, którego potem wykorzystujemy do sterowania kolorami.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


//Dodatkowe zmienne

#define DELAYVAL 500 // Czas w milesekundach do opóźniania wyswietlania
const int ledPin =  2; //Wbudowana dioda do ESP32, która swieci sie gdy serwer jest uruchomiony

//Ustawienie portu serwera na 80
WiFiServer server(80);

//Zmienna przechowujaca dane z requesta HTTP
String header;

//Dane, ktore przesylamy przez ESP32 do wszystkich slaveów
typedef struct kolory_struct {
  int R;
  int G;
  int B;
  int Brightness;
  int Tryb;
  int Wait;
} kolory_struct;

kolory_struct kolory; //Nazwa naszego structa

//Inicjacja ESPNow
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// Szukanie slaveów
void ScanForSlave() {
  int8_t scanResults = WiFi.scanNetworks();
  //reset slaves
  memset(slaves, 0, sizeof(slaves));
  SlaveCnt = 0;
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

      if (PRINTSCANRESULTS) {
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
      }
     // delay(10);
      // Check if the current device starts with `Slave`
      if (SSID.indexOf("Slave") == 0) {
        // SSID of interest
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];

        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            slaves[SlaveCnt].peer_addr[ii] = (uint8_t) mac[ii];
          }
        }
        slaves[SlaveCnt].channel = CHANNEL; // pick a channel
        slaves[SlaveCnt].encrypt = 0; // no encryption
        SlaveCnt++;
      }
    }
  }
if (SlaveCnt > 0) {
    Serial.print(SlaveCnt); Serial.println(" Slave(s) found, processing..");
  } else {
    Serial.println("No Slave Found, trying again.");
  }

  // Czyszczenie ramu
  WiFi.scanDelete();
}

// Sprawdzenie czy slave jest juz sparowany z masterem
// Jesli nie, slave jest parowany do mastera
void manageSlave() {
  if (SlaveCnt > 0) {
    for (int i = 0; i < SlaveCnt; i++) {
      Serial.print("Processing: ");
      for (int ii = 0; ii < 6; ++ii ) {
        Serial.print((uint8_t) slaves[i].peer_addr[ii], HEX);
        if (ii != 5) Serial.print(":");
      }
      Serial.print(" Status: ");
      // check if the peer exists
      bool exists = esp_now_is_peer_exist(slaves[i].peer_addr);
      if (exists) {
        // Slave already paired.
        Serial.println("Already Paired");
      } else {
        // Slave not paired, attempt pair
        esp_err_t addStatus = esp_now_add_peer(&slaves[i]);
        if (addStatus == ESP_OK) {
          // Pair success
          Serial.println("Pair success");
        } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
          // How did we get so far!!
          Serial.println("ESPNOW Not Init");
        } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
          Serial.println("Add Peer - Invalid Argument");
        } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
          Serial.println("Peer list full");
        } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
          Serial.println("Out of memory");
        } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
          Serial.println("Peer Exists");
        } else {
          Serial.println("Not sure what happened");
        }
      //  delay(100);
      }
    }
  } else {
    // No slave found to process
    Serial.println("No Slave found to process");
  }
}


//Wyslanie structa do wszystkich sparowanych slaveow
void sendData() {
  for (int i = 0; i < SlaveCnt; i++) {
    const uint8_t *peer_addr = slaves[i].peer_addr;
    if (i == 0) { // print only for first slave
      Serial.print("Sending: ");
      Serial.println(kolory.R);
    }
    esp_err_t result = esp_now_send(peer_addr, (uint8_t *) &kolory, sizeof(kolory_struct));
    Serial.print("Send Status: ");
    if (result == ESP_OK) {
      Serial.println("Success");
    } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      Serial.println("ESPNOW not Init.");
    } else if (result == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Invalid Argument");
    } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
      Serial.println("Internal Error");
    } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("ESP_ERR_ESPNOW_NO_MEM");
    } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
      Serial.println("Peer not found.");
    } else {
      Serial.println("Not sure what happened");
    }

  }


}


//Po przeslaniu danych
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}




//Zmienne odpowiedzialne za przechowywanie informacji, które wysyłamy do serwera
String redString = "0";
String greenString = "0";
String blueString = "0";
String brightString = "0";
String trybString = "0";
String waitString = "0";
//Kazda pozycja to kolejna czesc headera. Zmienne potrzebne do podzielenia headera
int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;
int pos5 = 0;
int pos6 = 0;
int pos7 = 0;


unsigned long currentTime = millis(); //Zmienna przechowujaca czas w milisekundach od uruchomienia płytki
unsigned long previousTime = 0; //Zmienna przechowujaca poprzedni czas
const long timeoutTime = 2000; //Timeout ustalony na 2 sekundy



void setup() {

  //Zainicjowanie portu szeregowego, paska ledów i leda na plytce
  Serial.begin(115200); 
  pinMode(ledPin, OUTPUT); //LED na płytce
  pixels.begin();

  WiFi.mode(WIFI_STA); //Odpalenie WiFi w trybie Station
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress()); //Wyswietlenie mac adresu mastera

  Serial.print("Setting AP (Access Point)…");
  // Mozna pozbyc sie parametru odpowiedzialnego za haslo, by siec byla niezabezpieczona
  WiFi.softAP(ssid, password);
  //Wyswietla IP, na którym rozglaszana jest strona
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  //Uruchomienie serwera
  server.begin();

  // Inicjacja ESPNow. ESPNow musi zostać zainicjowany po WiFi.
  InitESPNow();
  esp_now_register_send_cb(OnDataSent);

}


void loop(){




 
 
 WiFiClient client = server.available();   // Listen for incoming clients
 digitalWrite(ledPin, HIGH);     
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {            // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

//Kod odpowiedzialny za wyswietlanie strony
                   
            // Display the HTML web page
   client.println("<!DOCTYPE html><html> ");
  client.println("          <head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> ");
 client.println("           <link rel=\"icon\" href=\"data:,\"> ");
 client.println("           </head><body><div class=\"container\"><div class=\"row\"><h1>Wybierz kolor, jasnosc, tryb wyswietlania oraz opoznienie</h1></div> ");
 client.println("       <p>Red: <span id=\"czerwony\"></span></p>");
 client.println("       <input type=\"range\" min=\"0\" max=\"255\" value=\"0\" class=\"slider\" id=\"rValue\">       ");
 client.println("       <p>Green: <span id=\"zielony\"></span></p>");
 client.println("       <input type=\"range\" min=\"0\" max=\"255\" value=\"0\" class=\"slider\" id=\"gValue\">   ");   
 client.println("        <p>Blue: <span id=\"niebieski\"></span></p>");
 client.println("       <input type=\"range\" min=\"0\" max=\"255\" value=\"0\" class=\"slider\" id=\"bValue\">");       
 client.println("        <p>Jasnosc: <span id=\"jasnosc\"></span></p>");
 client.println("      <input type=\"range\" min=\"0\" max=\"255\" value=\"0\" class=\"slider\" id=\"Brightness\">  ");    
 client.println("<br><br>");
 client.println(" <p>Tryb: <span id=\"tryb\"></span></p>");
 client.println("      <input type=\"range\" min=\"0\" max=\"10\" value=\"0\" class=\"slider\" id=\"tValue\">");
 client.println("<br><br>");
 client.println("  <label for=\"Opoznienie\">Opoznienie(ms): </label> <input type=\"text\" id=\"waitValue\" ><br><br>");
  client.println("    <a  style=\"font-size:30px\" href=\"#\" id=\"change_color\" role=\"button\" onclick=\"update()\"  >Wyslij</a> ");
 client.println("      </div> ");
  client.println("          <script>       ");
 client.println("      var Brslider = document.getElementById(\"Brightness\");");
 client.println("     var Broutput = document.getElementById(\"jasnosc\");");
 client.println("      var JasnoscValue = 0;");
 client.println("     Broutput.innerHTML = Brslider.value;      ");
 client.println("     var Rslider = document.getElementById(\"rValue\");");
 client.println("     var Routput = document.getElementById(\"czerwony\");");
 client.println("      var RedValue = 0;");
 client.println("     Routput.innerHTML = Rslider.value;      ");
 client.println("     var Blueslider = document.getElementById(\"bValue\");");
 client.println("     var Blueoutput = document.getElementById(\"niebieski\");");
   client.println("    var BlueValue = 0;");
   client.println("   Blueoutput.innerHTML = Blueslider.value;    ");
 client.println("     var Greenslider = document.getElementById(\"gValue\");");
 client.println("     var Greenoutput = document.getElementById(\"zielony\");");
 client.println("      var GreenValue = 0;");
 client.println("     Greenoutput.innerHTML = Greenslider.value;  ");
 client.println("     var TrybSlider = document.getElementById(\"tValue\");");
 client.println("     var Tryboutput = document.getElementById(\"tryb\");");
 client.println("      var TrybValue = 0;");
 client.println("     Tryboutput.innerHTML = TrybSlider.value;              ");
 client.println("      Brslider.oninput = function() {");
  client.println(" Broutput.innerHTML = this.value;");
 client.println("  JasnoscValue = this.value;");
 client.println("}");
 client.println("    Rslider.oninput = function() {");
 client.println("  Routput.innerHTML = this.value;");
 client.println("  RedValue = this.value;");
 client.println("}");
 client.println("    Blueslider.oninput = function() {");
  client.println(" Blueoutput.innerHTML = this.value;");
 client.println("  BlueValue = this.value;");
 client.println("}");
 client.println("   Greenslider.oninput = function() {");
  client.println(" Greenoutput.innerHTML = this.value;");
  client.println(" GreenValue = this.value;");
 client.println("}");
 client.println("Tryboutput.innerHTML = \"Swiatlo stale (Wlasny kolor)\";");
 client.println(" TrybSlider.oninput = function() {");
 client.println("   TrybValue = this.value;");
 client.println(" switch(TrybValue) {");
  client.println(" case '0':");
  client.println("   Tryboutput.innerHTML = \"Swiatlo stale (Wlasny kolor)\";");
  client.println("   break;");
  client.println(" case '1':");
 client.println("    Tryboutput.innerHTML = \"Kolejka (Wlasny kolor, opoznienie)\";");
 client.println("    break;");
 client.println("case '2':");
  client.println("   Tryboutput.innerHTML = \"TheaterChase (Wlasny kolor, opoznienie)\";");
  client.println("   break;");
 client.println(" case '3':");
  client.println("   Tryboutput.innerHTML = \"Tecza (Opoznienie)\";");
  client.println("   break;");
 client.println(" case '4':");
  client.println("   Tryboutput.innerHTML = \"TheaterChase z tecza (Opoznienie)\";");
  client.println("   break;");
 client.println(" case '5':");
  client.println("   Tryboutput.innerHTML = \"Wygaszanie (Wlasny kolor, opoznienie)\";");
  client.println("   break;");
 client.println(" case '6':");
  client.println("   Tryboutput.innerHTML = \"Cylon Bounce (Wlasny kolor, opoznienie)\";");
  client.println("   break;");
 client.println(" case '7':");
 client.println("    Tryboutput.innerHTML = \"Sparkle (Wlasny kolor, opoznienie)\";");
  client.println("   break;");
 client.println(" case '8':");
 client.println("    Tryboutput.innerHTML = \"Running Lights (Wlasny kolor, opoznienie)\";");
  client.println("   break;");
 client.println(" case '9':");
   client.println("  Tryboutput.innerHTML = \"Color Wipe (Wlasny kolor, opoznienie)\";");
  client.println("   break;");
 client.println(" case '10':");
   client.println("  Tryboutput.innerHTML = \"Meteor Rain (Wlasny kolor, opoznienie)\";");
   client.println("  break;");
 client.println("}}");
   client.println("   function update(){ ");
  client.println("              var redValue = document.getElementById(\"rValue\").value;");
 client.println("        var greenValue = document.getElementById(\"gValue\").value;");
   client.println("      var blueValue = document.getElementById(\"bValue\").value;");
   client.println("      var trValue = document.getElementById(\"tValue\").value;");
   client.println("      var wValue = document.getElementById(\"waitValue\").value; ");
    client.println("              document.getElementById(\"change_color\").href=\"?r\" + redValue + \"g\" + greenValue + \"b\" + blueValue + \"l\" + JasnoscValue + \"t\" + trValue + \"w\" + wValue + \"&\"; } </script></body></html> ");
            // The HTTP response ends with another blank line
            client.println();

    //Dzielenie headera
            if(header.indexOf("GET /?r") >= 0) {
              pos1 = header.indexOf('r');
              pos2 = header.indexOf('g');
              pos3 = header.indexOf('b');
              pos4 = header.indexOf('l');
              pos5 = header.indexOf('t');
              pos6 = header.indexOf('w');
              pos7 = header.indexOf('&');
              
    //Pobieranie danych z headera
              redString = header.substring(pos1+1, pos2);
              greenString = header.substring(pos2+1, pos3);
              blueString = header.substring(pos3+1, pos4);
              brightString = header.substring(pos4+1,pos5);
              trybString = header.substring(pos5+1,pos6);
              waitString = header.substring(pos6+1,pos7);
              
//Konwersja stringów z headera do intów
  kolory.R = redString.toInt();
  kolory.G = greenString.toInt();
  kolory.B = blueString.toInt();
  kolory.Brightness = brightString.toInt();
  kolory.Tryb = trybString.toInt();
  kolory.Wait = waitString.toInt();

//Wyswietlenie wszystkich danych na porcie szeregowym
              Serial.println(kolory.R);
              Serial.println(kolory.G);
              Serial.println(kolory.B);
              Serial.println(kolory.Brightness);
              Serial.println(kolory.Tryb);
              Serial.println(kolory.Wait);
             
   //Szukamy slaveów w okolicy
  ScanForSlave();

  if (SlaveCnt >= 0){ //Aktualnie niepotrzebny if ale moze zostać wykorzystany w przyszłosci. Funcka przejdzie nawet wtedy gdy nie znajdzie żadnego slave w okolicy
  
  //Sparowanie slave jesli wczesniej nie zostal sparowany
    manageSlave();
//Wysylanie danych. Kolory pokazuja sie dopiero po przeslaniu
    sendData();
    
  } else {
    // Else do niewykorzystanego ifa zostawionego na przyszlosc. W przypadku gdy nie znajdzie slaveow.
  }
            }
            // Wyrwanie sie petli while
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }


} //Koniec loopa
