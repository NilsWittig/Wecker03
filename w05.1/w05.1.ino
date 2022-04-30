#include <SPI.h>
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"


DFRobotDFPlayerMini dfplayer;
SoftwareSerial SoftSerial(4, 5); // RX, TX
ESP8266WebServer server(80);

bool player = false;

long alarms[5]; //the alarm array
int alarmOff_button = 12; //D6 stops ringing and cancels sleepmode of Alarm
int sleep_button = 13; //D7 stops ringing and sets Alarm in sleepmode
int alarm_led = 15; //a LED that is on while an Alarm is ringing or in sleepmode and toggles the amp [1 = OFF, 0 = ON]
volatile bool sleep = false; // in  sleep mode 
volatile bool ringing = false; // currently ringing
volatile int db_aoff = 0; // debounce
volatile int db_sleep = 0;// debounce
volatile int aoff = false;// debounce
volatile int slee = false;// debounce
volatile bool show = false; // show time(false) or show ip(true)
volatile int show_ = 0; // part of ip wich is shown
int ipParts[4]; // array holding the values between the dots in the ip
int ringTone = 0; // song number
int CYCLES = 3; // ring cycles
int cycle = 0; // current ring cycle
int nextCycleTime = 0; // time in minutes when the next cycle begins
int last = 0; // number of milliseconds after last wakeup check


//########################  Variables for 7 Segment Display ##########################
//  --2--
//  0---3
//  --1--
//  6---4
//  --5--      7-6-5-4-3-2-1-0
int ZERO[]  = {0,1,1,1,1,1,0,1};
int ONE[]   = {0,0,0,1,1,0,0,0};
int TWO[]   = {0,1,1,0,1,1,1,0};
int THREE[] = {0,0,1,1,1,1,1,0};
int FOUR[]  = {0,0,0,1,1,0,1,1};
int FIVE[]  = {0,0,1,1,0,1,1,1};
int SIX[]   = {0,1,1,1,0,1,1,1};
int SEVEN[] = {0,0,0,1,1,1,0,0};
int EIGHT[] = {0,1,1,1,1,1,1,1};
int NINE[]  = {0,0,1,1,1,1,1,1};
int numbers[10][8] = {
  {0,1,1,1,1,1,0,1},
  {0,0,0,1,1,0,0,0},
  {0,1,1,0,1,1,1,0},
  {0,0,1,1,1,1,1,0},
  {0,0,0,1,1,0,1,1},
  {0,0,1,1,0,1,1,1},
  {0,1,1,1,0,1,1,1},
  {0,0,0,1,1,1,0,0},
  {0,1,1,1,1,1,1,1},
  {0,0,1,1,1,1,1,1}
};

int ser = 14; //D5
int srck = 2; //D4
int rck = 0; //D3
int latch = 0; //D3

//void handleIT();
//################################## Setup ##############################################################
void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");
  for(int i = 0; i < 5; i++){// initialize alarms with zeros
    alarms[i] = 0;
  }
  //################################ Init WifiManager ###################################################
  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP", "www");//first parameter is name of access point, second is the password
  //############################  init Alarm memory and related buttons  #################################
  
  pinMode(alarmOff_button, INPUT);
  pinMode(sleep_button, INPUT);
  pinMode(alarm_led, OUTPUT);
  digitalWrite(alarm_led, HIGH);
  attachInterrupt(digitalPinToInterrupt(alarmOff_button), button_alarmOff, RISING);
  attachInterrupt(digitalPinToInterrupt(sleep_button), button_sleep, RISING);
   //############################  Init 7 Segment Display  ##############################################
  pinMode(ser, OUTPUT);
  pinMode(srck, OUTPUT);
  pinMode(rck, OUTPUT);
  digitalWrite(ser, LOW);
  digitalWrite(srck, LOW);
  digitalWrite(rck, LOW);
  digitalWrite(latch, LOW);
  display_4(ONE,THREE,THREE,SEVEN);//Display init value (1337)
  digitalWrite(latch, HIGH);
  
  //###########################  Initialize the network time protocol ######################################
  NTP.begin("pool.ntp.org", 1, true);
  NTP.setInterval(63);
  //############################  Start Webserver #########################################################
  if (MDNS.begin("esp8266")) {Serial.println("MDNS responder started");}
  server.onNotFound(handleIT);// only one handle
  server.begin();
  //########################### Start DfPlayer ############################################################
  
  SoftSerial.begin(9600);
  delay(100);
  Serial.println("SoftSerial established");
  Serial.println("NOW CRASH!!!!!!!!!");
  dfplayer.begin(SoftSerial);
  delay(100);
  Serial.println("dfplayer started");
  dfplayer.volume(15);
  

  Serial.print("ip: ");
  Serial.println(WiFi.localIP());
  Serial.println("finished initialization");

  for(int i = 0; i < 4; i++){ // split ip in handy pieces
    ipParts[i] = WiFi.localIP()[i];
  }
  
  
}

void loop() {
  server.handleClient();

  if( (last + 200) < millis() ){

    //############## debounce ############
    if(db_aoff > 0){db_aoff--;}
    if(db_sleep > 0){db_sleep--;}
    //##### show last part of the ip if the user so desires ##############
    
    if(show){
      int first_num = int( ipParts[show_] / 100 );
      int second_num = int( ( ipParts[show_] - (first_num * 100) ) / 10 );
      int third_num = int( ( ipParts[show_] - (first_num * 100) ) % 10 );
      digitalWrite(latch, LOW);
      display_4(numbers[0],numbers[first_num],numbers[second_num],numbers[third_num]);
      digitalWrite(latch, HIGH);
      
      //int relip = WiFi.localIP()[3];
      //int lsb = relip % 10;
      //int msb = relip / 10;
      //digitalWrite(latch, LOW);
      //display_4(numbers[0],numbers[0],numbers[msb],numbers[lsb]);
      //digitalWrite(latch, HIGH);
      
    }else{update_time();}
    
    //####################### wake? ##############################
    if( !ringing && !sleep ){wake(hour(),minute());}
    if( !ringing && sleep ){sleepWake(hour(),minute());}

    last = millis();
  }

  yield();

}
