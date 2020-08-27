

#include <ESP8266WiFi.h>
#include <espnow.h>

#include <Adafruit_NeoPixel.h>


#define PIN  12  //PIN 12 czyli GPIO12 odpowiada D6 na ESP8266

#define NUMPIXELS 86 

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500 

#define CHANNEL 1

//Inicjacja ESPNow
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == 0) {
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

// Konfiguracja rozgłaszanej sieci. Kazda plytka ma unikalny adres mac dlatego kazde SSID zawsze bedzie takie same i unikalne
void configDeviceAP() {
  String Prefix = "Slave:";
  String Mac = WiFi.macAddress();
  String SSID = Prefix + Mac;
  String Password = "123456789";
  bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}


//Dokladnie to samo co po stronie mastera 
typedef struct test_struct {
  int R;
  int G;
  int B;
  int Brightness;
  int Tryb;
  int Wait;
} test_struct;


test_struct test;




//Funkcja odpalajaca sie za kazdym razem gdy slave otrzyma informacje poprzez ESPNow od mastera. Tutaj wyswietla na porcie szeregowym otrzymane wartosci.
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&test, incomingData, sizeof(test));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("R: ");
  Serial.println(test.R);
  Serial.print("G: ");
  Serial.println(test.G);
  Serial.print("B: ");
  Serial.println(test.B);
  Serial.print("Brightness: ");
  Serial.println(test.Brightness);
  Serial.print("Tryb: ");
  Serial.println(test.Tryb);
   Serial.print("Wait: ");
  Serial.println(test.Wait);

if(test.Tryb==0 && test.Wait==0){
    SwiatloStale(0);
}

   
  }

void SwiatloStale(int wait){
  
 if(test.Brightness != 0){
    pixels.setBrightness(test.Brightness);
}
else{
  pixels.setBrightness(50);
}
pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...

    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:
    pixels.setPixelColor(i, pixels.Color(test.R, test.G, test.B));

    pixels.show();   // Send the updated pixel colors to the hardware.

if(test.Tryb==1){
  delay(wait); // Pause before next pass through loop
}

   
  }
   Serial.println("Skończyłem wyświetlać");
}

//Funkcje
//Wszystkie wytlumaczone sa w kodzie do mastera z wyświetlaniem


void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){

  for(int i = 0; i < NUMPIXELS-EyeSize-2; i++) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue);
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    delay(SpeedDelay);
  }

  delay(ReturnDelay);

  for(int i = NUMPIXELS-EyeSize-2; i > 0; i--) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue);
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    delay(SpeedDelay);
  }
 
  delay(ReturnDelay);
}

void Sparkle(byte red, byte green, byte blue, int SpeedDelay) {
  int Pixel = random(NUMPIXELS);
  setPixel(Pixel,red,green,blue);
  showStrip();
  delay(SpeedDelay);
  setPixel(Pixel,0,0,0);
}

void RunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position=0;
 
  for(int i=0; i<NUMPIXELS*2; i++)
  {
      Position++; // = 0; //Position + Rate;
      for(int i=0; i<NUMPIXELS; i++) {
        setPixel(i,((sin(i+Position) * 127 + 128)/255)*red,
                   ((sin(i+Position) * 127 + 128)/255)*green,
                   ((sin(i+Position) * 127 + 128)/255)*blue);
      }
     
      showStrip();
      delay(WaveDelay);
  }
}
void colorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  for(uint16_t i=0; i<NUMPIXELS; i++) {
      setPixel(i, red, green, blue);
      showStrip();
      delay(SpeedDelay);
  }
}

byte * Wheel(byte WheelPos) {
  static byte c[3];
 
  if(WheelPos < 85) {
   c[0]=WheelPos * 3;
   c[1]=255 - WheelPos * 3;
   c[2]=0;
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   c[0]=255 - WheelPos * 3;
   c[1]=0;
   c[2]=WheelPos * 3;
  } else {
   WheelPos -= 170;
   c[0]=0;
   c[1]=WheelPos * 3;
   c[2]=255 - WheelPos * 3;
  }

  return c;
}



void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {  
  setAll(0,0,0);
 
  for(int i = 0; i < NUMPIXELS+NUMPIXELS; i++) {
   
   

    for(int j=0; j<NUMPIXELS; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
   
    for(int j = 0; j < meteorSize; j++) {
      if( ( i-j <NUMPIXELS) && (i-j>=0) ) {
        setPixel(i-j, red, green, blue);
      }
    }
   
    showStrip();
    delay(SpeedDelay);
  }
}

void fadeToBlack(int ledNo, byte fadeValue) {
 #ifdef ADAFRUIT_NEOPIXEL_H
    // NeoPixel
    uint32_t oldColor;
    uint8_t r, g, b;
    int value;
   
    oldColor = pixels.getPixelColor(ledNo);
    r = (oldColor & 0x00ff0000UL) >> 16;
    g = (oldColor & 0x0000ff00UL) >> 8;
    b = (oldColor & 0x000000ffUL);

    r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
    g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
    b=(b<=10)? 0 : (int) b-(b*fadeValue/256);
   
    pixels.setPixelColor(ledNo, r,g,b);
 #endif
}


void showStrip() {
   pixels.show();
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
   pixels.setPixelColor(Pixel, pixels.Color(red, green, blue));
}

void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUMPIXELS; i++ ) {
    setPixel(i, red, green, blue);
  }
  showStrip();
}

void FadeInOut(){
  float r, g, b;
     
  for(int k = 0; k < 256; k=k+1) {
    r = (k/256.0)*test.R;
    g = (k/256.0)*test.G;
    b = (k/256.0)*test.B;
    setAll(r,g,b);
    showStrip();
  }
     
  for(int k = 255; k >= 0; k=k-2) {
    r = (k/256.0)*test.R;
    g = (k/256.0)*test.G;
    b = (k/256.0)*test.B;
    setAll(r,g,b);
    showStrip();
  }
}


void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<20; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      pixels.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<pixels.numPixels(); c += 3) {
        pixels.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      pixels.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}


void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<pixels.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (pixels.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / pixels.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
    }
    pixels.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}


void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      pixels.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<pixels.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / pixels.numPixels();
        uint32_t color = pixels.gamma32(pixels.ColorHSV(hue)); // hue -> RGB
        pixels.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      pixels.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}


 
void setup() {

  pixels.begin(); // Inicjalizacja listwy
  Serial.begin(115200); //Inicjalizacja monitora portu szeregowego
  WiFi.mode(WIFI_AP); //Ustawienie urzadzenia w trybie Access Point, by master mógł zobaczyc jego adres mac
  configDeviceAP();
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress()); //Wyswietlenie mac adresu tego urzadzenia
  // Inicjalizacja ESPNow
  InitESPNow();
  esp_now_register_recv_cb(OnDataRecv); //Po otrzymaniu czegos poprzez ESPNow wyzwalana jest funkcja OnDataRecv

  
}
 
void loop() {

 switch(test.Tryb) {


 case 1  : {
                SwiatloStale(test.Wait);
                break;
              }

 case 2  : {
                theaterChase(pixels.Color(test.R, test.G ,test.B), test.Wait);
                break;
              }

 case 3  : {
                rainbow(test.Wait);             // Flowing rainbow cycle along the whole strip
                break;
              }
              
 case 4  : {
                theaterChaseRainbow(test.Wait); // Rainbow-enhanced theaterChase variant
                break;
              }
  
 case 5  : {
                  FadeInOut(); 
                break;
              }

 case 6  : {
                
                CylonBounce(test.R, test.G, test.B, 16, test.Wait, 50);
                break;
              }

 case 7  : {
                
                Sparkle(test.R, test.G, test.B, test.Wait);
                break;
              }

 case 8 : {
                
                RunningLights(test.R,test.G,test.B, test.Wait);  // red
                break;
              }

 case 9 : {
                colorWipe(test.R,test.G,test.B, test.Wait);
                colorWipe(0x00,0x00,0x00, test.Wait);
                break;
              }

              
 case 10 : {
                // meteorRain - Color (red, green, blue), meteor size, trail decay, random trail decay (true/false), speed delay
                meteorRain(test.R,test.G,test.B,10, 64, true, test.Wait);
                break;
              }        



}


}
