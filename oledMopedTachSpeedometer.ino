
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#include "SPIFFS.h"
//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels



//RPM counter variables 
double freq;
int speedKPH;
int speedRPM;
int speedKPHMax =0;
int speedKPHMedium=0;
int stroke =1;
float wheel= 1.25;
int screenMode=0; //mode: 0 - rpm and kph || 1- kph || 2 - rpm || 3 - config stroke and wheel || 4 - average
int configMode=0; // mode: 0 - nothing || 1 - stroke || 2 - wheel size
int lastMode=10;
long samples=0;
bool p1=false;
bool p2=false;
bool p1Last=false;
bool p2Last=false;
long p1Time=0;
long p2Time=0;

TaskHandle_t Task1;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST,800000);

void setup() {
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  stroke=readFile("/stroke.txt").toInt();
  wheel=readFile("/size.txt").toFloat();
 
  Serial.begin(115200);
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  pinMode(13,INPUT);
  pinMode(12,INPUT);
  pinMode(34,INPUT_PULLUP);
  pinMode(35,INPUT_PULLUP);
  
//rpm counter values init
  freq = 0;
  speedKPH=0;
  speedRPM=0;
 
 //initialize OLED
Wire.setClock(800000);
  Wire.begin(OLED_SDA, OLED_SCL);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  menuDoble();  // Draw background
  for(int u=0; u<50;u++){
    displayRPM(u*300);
    displayKPH(u*3);
    }
    delay(1000);
    for(int d=50; d>0;d--){
    displayRPM(d*300); 
    displayKPH(d*3);
    }
delay(2000);
display.clearDisplay(); // Clear display buffer
display.display();
display.setTextSize(4);      // Normal 1:1 pixel scale
display.setTextColor(SSD1306_WHITE); // Draw white text
display.setCursor(0, 0);     // Start at top-left corner
display.write("TEST OK!");
display.display();
delay(2000);
screenMode=0;

 xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */                   
}
 
void menuDoble() {
  display.clearDisplay(); // Clear display buffer
  display.display();
  display.drawLine(88, 0, 88,64, SSD1306_WHITE);
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(10, 0);     // Start at top-left corner
  display.write("K/Hour");
  display.setCursor(92, 0);     // Start at top-left corner
  display.write("RPM"); 
}
void menuKPH (){
  display.clearDisplay(); // Clear display buffer
  display.display();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(40, 0);     // Start at top-left corner
  display.write("K/Hour");
  }

void menuRPM(){
  display.clearDisplay(); // Clear display buffer
  display.display();
  display.drawLine(0, 16,128,16, SSD1306_WHITE);
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(85, 0);     // Start at top-left corner
  display.write("RPM");  
  }  

  void menuAverage(){
  display.clearDisplay(); // Clear display buffer
  display.display();
  display.drawLine(64,0,64,64, SSD1306_WHITE);
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.write("Avrg.");
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setCursor(80, 0);     // Start at top-left corner
  display.write("Max");
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setCursor(0, 53);     // Start at top-left corner
  display.write("K / Hour");
  display.setCursor(70, 53);     // Start at top-left corner
  display.write("K / Hour");
  }  

void menuConfig()
{
    display.clearDisplay(); // Clear display buffer
    display.display();
    display.drawLine(0, 16, 128, 16, SSD1306_WHITE);
    display.drawLine(56, 17, 56, 64, SSD1306_WHITE);
    display.setTextSize(2);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(32, 0);            // Start at top-left corner
    display.write("Config");
    display.setTextSize(1);   // Normal 1:1 pixel scale
    display.setCursor(0, 20); // Start at top-left corner
    display.write("Pulse/Rot");
    display.setCursor(58, 20); // Start at top-left corner
    display.write("Wheel size ");
    display.setCursor(65, 30);
    display.write("(Meters)");
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(3);    // Normal 1:1 pixel scale
    display.setCursor(15, 37); // Start at top-left corner
    char f[] = "99999";
    String(stroke).toCharArray(f, 2);
    display.write(f);
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(2);    // Normal 1:1 pixel scale
    display.setCursor(65, 43); // Start at top-left corner
    String(wheel).toCharArray(f, 5);
    display.write(f);
}

void displayKPH(int kph){
  int x = 10;
  int y = 25;
  int textSize=4;
  if(screenMode ==1){
  x = 15;
  y = 20;
  textSize = 6;
  }
  if(screenMode<=1){
    display.setTextColor(WHITE,BLACK);
    display.setTextSize(textSize);      // Normal 1:1 pixel scale
    display.setCursor(x, y);     // Start at top-left corner
    display.write("   ");
    display.setCursor(x,y);     // Start at top-left corner
    char d[]="999";
    if(kph>=0 && kph<250){
      String(kph).toCharArray(d,4);
    }
    display.write(d);
    display.display();
   }
}
  
void displayRPM(int rpm){
  int x1 =89;
  int y1 =14;
  int x2 =127;
  int y2 =63;
  if(screenMode==2){
    x1=0;
    y1=17;
    x2=128;
    y2=64;
    }
  
  if(screenMode==0){
    display.fillRect(x1,y1,x2,y2,SSD1306_BLACK);
    rpm=map(rpm,0,15000,0,50);
    if(rpm >50){
      rpm=49;
      }
    for(int i=64; i>(64-rpm); i-=2) {
      int x=8;
      if(i<35){
        x=((64-i)/2)-5;
      }
    display.drawLine(110-x, i, 110+x,i, SSD1306_WHITE);
    }
  }
    
    if(screenMode==2){ 
    display.fillRect(x1,y1,x2,y2,SSD1306_BLACK);
    display.setTextColor(WHITE,BLACK);
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.setCursor(15, 0);     // Start at top-left corner
    display.write("     ");
    display.setCursor(15, 0);     // Start at top-left corner
    char f[]="15000";
    String(rpm).toCharArray(f,6);
    display.write(f);
      rpm=map(rpm,0,15000,0,128);
    for(int i=0; i<rpm; i+=2) {
      int sz=10;
      sz=map(i,0,128,5,20);
    display.drawLine(i,40-sz,i,40+sz,SSD1306_WHITE);
   }
  }
  display.display();
}

void displayAverage(){
  if(screenMode==4){
    display.setTextColor(WHITE,BLACK);
    display.setTextSize(3);      // Normal 1:1 pixel scale
    display.setCursor(0,25 );     // Start at top-left corner
    display.write("   ");
    display.setCursor(66,25 );     // Start at top-left corner
    display.write("   ");
    display.setCursor(0,25);     // Start at top-left corner
    char d[]="999";   
    String(speedKPHMedium).toCharArray(d,4);
    display.write(d);
    String(speedKPHMedium).toCharArray(d,4);
    display.setCursor(66,25);     // Start at top-left corner
    display.write(d);
    display.display();
    }
  }

  void displayConfig(){
    if(screenMode==3){
    
      if(configMode==1){
        display.setTextColor(BLACK,WHITE);
        display.setTextSize(4);      // Normal 1:1 pixel scale
        display.setCursor(13, 34);     // Start at top-left corner
        display.write(" ");
        display.setTextSize(3);      // Normal 1:1 pixel scale
        display.setCursor(15, 37);     // Start at top-left corner
        char f[]="99999";
        String(stroke).toCharArray(f,2);
        display.write(f);
        display.setTextColor(WHITE,BLACK);
        display.setTextSize(2);      // Normal 1:1 pixel scale
        display.setCursor(65, 43);     // Start at top-left corner
        display.write("    ");
        display.setCursor(65, 43);     // Start at top-left corner
        String(wheel).toCharArray(f,5);
        display.write(f);
        }
        if(configMode==2){
          display.setTextColor(WHITE,BLACK);
          display.setTextSize(3);      // Normal 1:1 pixel scale
          display.setCursor(15, 37);     // Start at top-left corner
          display.write(" ");
          display.setCursor(15, 37);     // Start at top-left corner
          char f[]="99999";
          String(stroke).toCharArray(f,2);
          display.write(f);
          display.setTextColor(BLACK,WHITE);
          display.setTextSize(2);      // Normal 1:1 pixel scale
          display.setCursor(64, 41);     // Start at top-left corner
          display.write("    ");
          display.setCursor(65, 43);     // Start at top-left corner
          String(wheel).toCharArray(f,5);
          display.write(f);
        }
    }
  }

void writeFile(String fileDir, String input){
  File file = SPIFFS.open(fileDir, "w");
        if(!file){
          // File not found
          Serial.println("Failed to open test file");
          return;
            } else {
            file.println(input);
            file.close();
            }
  }
String readFile(String fileDir){
  String output;
  File file = SPIFFS.open(fileDir, "r");
        if(!file){
          // File not found
          Serial.println("Failed to open data file");          
          return "";
            } else {
                byte fbyte;
                char fchar;
              while(file.available()){
                fbyte= file.read();
                if(fbyte != -1){
                  fchar = char(fbyte);
                output += fchar;
                Serial.print(fchar);
                  }
                }
            file.close();
            int l = output.length();
            output.remove(l-1);
            return output;
            }
  }
    
void Task1code(void * pvParameters){
long deltaT =0;
long microNow=0;
long microLastRPM=0;
long microLastKPH=0;
bool nowValRPM=false;
bool nowValKPH=false;
bool lastBitRPM=false;
bool lastBitKPH=false;
  for(;;){
      nowValRPM=digitalRead(13);
      nowValKPH=digitalRead(12);
      microNow=micros();
        if(microNow-microLastRPM>250000){
        freq=0;
        }
        if(microNow-microLastKPH>250000){
        speedKPH=0;
        }
      if (nowValRPM==true && lastBitRPM!=nowValRPM) {
      deltaT=(microNow-microLastRPM);
      freq=1000000/deltaT;
      microLastRPM=microNow;
      }
      if (nowValKPH==true && lastBitKPH!=nowValKPH) {
      deltaT=(microNow-microLastKPH);
      speedKPH=(wheel/stroke)*(1000000/deltaT);
      if(speedKPH > speedKPHMax){
        speedKPHMax=speedKPH;
      }
      speedKPHMedium = ((speedKPHMedium * samples)+speedKPH)/(samples +1);
      microLastKPH=microNow;
      }
    lastBitRPM=nowValRPM;
    lastBitKPH=nowValKPH;
    samples++;
  }
}
   
void loop()
{
    p1 = digitalRead(34);
    p2 = digitalRead(35);

    if (p1==true && p2==true && screenMode == 3)
    {
        if (p1==true && p2==true && p1 != p1Last && p2 != p2Last)
        {
            p1Time = millis();
            p2Time = millis();
        }
        if ((millis() - p1Time >= 2000) && (millis() - p2Time >= 2000) && p1Time != -1 && p2Time != -1)
        {
            configMode++;
            if (configMode > 2)
            {
            writeFile("/stroke.txt",String(stroke)); 
            writeFile("/size.txt",String(wheel));           
            configMode = 0;
            }
            p1Time = -1;
            p2Time = -1;
        }
    }

    if (p1 == true && p2 == false)
    {
        if (p1==true && p1Last == false)
        {
            p1Time = millis();
        }
        if (millis() - p1Time >= 250 && p1Time != -1)
        {
            if (screenMode == 3 && configMode == 1)
            {
                if (stroke > 1)
                {
                    stroke--;
                }
            }
            if (screenMode == 3 && configMode == 2)
            {
                if (wheel > 0.1)
                {
                    wheel = wheel + -0.01;
                  
                }
            }
            if (screenMode != 3 || (screenMode == 3 && configMode == 0))
            {
                screenMode--;
                if (screenMode <= 0)
                {
                    screenMode = 4;
                }
            }
            p1Time = -1;
        }
    }

    if (p2==true && p1==false)
    {
        if (p2==true && p2Last == false)
        {
            p2Time = millis();
        }
        if (millis() - p2Time >= 250 && p2Time != -1)
        {
            if (screenMode == 3 && configMode == 1)
            {
                if (stroke < 9)
                {
                    stroke++;
                }
            }
            if (screenMode == 3 && configMode == 2)
            {
                if (wheel < 3.01)
                {
                    wheel = wheel + 0.01;
                }
            }
            if (screenMode != 3 || (screenMode == 3 && configMode == 0))
            {
                screenMode++;
                if (screenMode > 4)
                {
                    screenMode = 0;
                }
            }
            p2Time = -1;
        }
    }

    p1Last = p1;
    p2Last = p2;

    if (lastMode != screenMode)
    {
        switch (screenMode)
        {
        case 0:
            menuDoble();
            break;
        case 1:
            menuKPH();
            break;
        case 2:
            menuRPM();
            break;
        case 3:
            menuConfig();
            configMode = 0;
            break;
        case 4:
            menuAverage();
            break;
        default:
            menuDoble();
            break;
        }
        lastMode = screenMode;
    }

    speedRPM = freq;
    displayRPM(speedRPM);
    displayKPH(speedKPH);
    displayAverage();
    displayConfig();
    //Serial.println(freq);
    
}
