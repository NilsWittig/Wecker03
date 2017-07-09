#include <SPI.h>
#include <NtpClientLib.h>
#include <ArduinoOTA.h>
#include <SimpleList.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>

#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
SoftwareSerial SoftSerial(4, 5); // RX, TX
DFRobotDFPlayerMini dfplayer;

//MDNSResponder mdns;
ESP8266WebServer server(80);
String WECKER = "";
const char* www_username = "nils";
const char* www_password = "w03";

int last = 0;

volatile bool aoff = false;
volatile bool slee = false;

volatile bool show = false;

volatile int db_aoff = 0;
volatile int db_sleep = 0;

int dbg = 0;

int SONGCOUNT = 18;
String SONGINFO = "";

String SONGS[] = {
  "The Package",
  "Moon",
  "Agostina",
  "Piano Lessons",
  "Dreams",
  "Fight",
  "Grand Canyon",
  "Night",
  "Deep Peace",
  "The Blizzard",
  "Horizons",
  "Terminal",
  "Earthrise",
  "Away",
  "Greetings",
  "Fadeaway",
  "A Daisy Through Concrete",
  "A Pillow Of Winds"
};

//######################### Alarm handling variables  #############################
SimpleList<long> alarms; //a list of Alarms, an Alarm is a long number calculated as such: (hour * 10^6) + (minute * 10^4) + (song * 10) + active
volatile bool ringing = false; //is an Alarm currently ringing?
volatile bool sleep = false; //is an Alarm currently in sleepmode?
volatile int repeats = 3; //times to repeat 3 -> 5 times ringing with initial ring
volatile int curr_repeat = -1; //times to repeat current alarm
int next_repeat = -1; //next time to ring from sleep
long curr_checksum = -1; //checksum of current alarm
int curr_song = 1; //song of current alarm
bool alarmsactive = false; // is there any activ alarm?
String active_alarms = "None"; //string form of all active alarms to display on the OLED display
int alarmOff_button = 12; //D6 stops ringing and cancels sleepmode of Alarm
int sleep_button = 13; //D7 stops ringing and sets Alarm in sleepmode
int alarm_led = 15; //a LED that is on while an Alarm is ringing or in sleepmode and toggles the amp
bool alarm_led_on = false; //state to blink the alarm_led
int slow_led = 0; //
int alarm_off = 0; //a counter to switch off the last ring of an alarm thats not been manually deactivated

int ser = 14; //D5
int srck = 2; //D4
int rck = 0; //D3
int latch = 0; //D3

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

void setup() {

  Serial.begin(115200);

  Serial.println("");
  Serial.print("ready: ");
  Serial.println(dbg++); //####################  0
  
  // put your setup code here, to run once:
WiFiManager wifiManager;
//first parameter is name of access point, second is the password
wifiManager.autoConnect("ESP", "www");

  Serial.print("ready: ");
  Serial.println(dbg++); //####################  1

//############################  init Alarm memory and related buttons  #################################
  pinMode(alarmOff_button, INPUT);
  pinMode(sleep_button, INPUT);
  pinMode(alarm_led, OUTPUT);
  
  //digitalWrite(alarm_led, HIGH);
  alarms.reserve(512); // Pre-reserve the memory, so later the inserts and deletes will be much faster
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

  Serial.print("ready: ");
  Serial.println(dbg++); //####################  2
  //###########################  Initialize the network time protocol ######################################
  NTP.begin("pool.ntp.org", 1, true);
  NTP.setInterval(63);

  Serial.print("ready: ");
  Serial.println(dbg++); //####################  3
  //########################### build Webpage string  ######################################################
  WECKER += "<title>Wecker</title>";
  WECKER += "<h1>Wecker Web Server</h1>";
  WECKER += "<p>";
  WECKER += "Hour, Minute, Song, Active";
  WECKER += "<form>";
  WECKER += "<form action=\"WeckerForm.asp\" method=\"get\">";
  WECKER += "<input type=\"number\" min=\"0\" max=\"23\" name=\"hour\" value=\"Hour\" >";
  WECKER += "<input type=\"number\" min=\"0\" max=\"59\" name=\"minute\" value=\"Minute\" >";
  WECKER += "<input type=\"number\" min=\"1\" max=\"";
  WECKER += SONGCOUNT;
  WECKER += "\" name=\"song\" value=\"Song\" >"; //Remember Song count!!!
  WECKER += "<input type=\"number\" min=\"0\" max=\"1\" name=\"active\" value=\"Active?\" >";
  WECKER += "<input type=\"submit\" value=\"Abschicken\">";
  WECKER += "</form>";
  //WECKER += "<a href=\"wecker\">";
  WECKER += "</p>";

  SONGINFO += "<p>";
  SONGINFO += "<OL>";

  for(int si = 0; si < SONGCOUNT; si++){
    SONGINFO += "<LI>" + SONGS[si] + "<br>";
  }
//  SONGINFO += "01-The Package<br>";
//  SONGINFO += "02-Moon<br>";
//  SONGINFO += "03-Agostina<br>";
//  SONGINFO += "04-Piano Lessons<br>";
//  SONGINFO += "05-Dreams<br>";
//  SONGINFO += "06-Fight<br>";
//  SONGINFO += "07-Grand Canyon<br>";
//  SONGINFO += "08-Night<br>";
//  SONGINFO += "09-Deep Peace<br>";
//  SONGINFO += "10-The Blizzard<br>";
//  SONGINFO += "11-Horizons<br>";
//  SONGINFO += "12-Terminal<br>";
//  SONGINFO += "13-Earthrise<br>";
//  SONGINFO += "14-Away<br>";
//  SONGINFO += "15-Greetings<br>";
//  SONGINFO += "16-Fadeaway<br>";
//  SONGINFO += "17-A Daisy Through Concrete<br>";
//  SONGINFO += "18-A Pillow Of Winds<br>";

  SONGINFO += "</UL>";
  SONGINFO += "</p>";
  //########################## start mdns and authentication service  ############################################################
  //if (mdns.begin("esp8266", WiFi.localIP())) {} //start mdns

  if (MDNS.begin("esp8266")) {}
  
  ArduinoOTA.begin(); //start authentication service

  Serial.print("ready: ");
  Serial.println(dbg++); //####################  4
  //##########################  Create Webserver handlers ##################################################
  server.on("/", [](){
    if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
    server.send(200, "text/html", WECKER);
  });
  server.on("/wecker", [](){
    String arguments = "";
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      arguments += server.argName ( i );
    }
    if(arguments == "hourminutesongactive"){  //check for new Alarm
      char hours[16];
      char mins[16];
      char songs[16];
      char activ[16];
      String hours_s = server.arg (0);
      String mins_s = server.arg (1);
      String songs_s = server.arg (2);
      String activ_s = server.arg (3);
      hours_s.toCharArray(hours, sizeof(hours_s));
      mins_s.toCharArray(mins, sizeof(mins_s));
      songs_s.toCharArray(songs, sizeof(songs_s));
      activ_s.toCharArray(activ, sizeof(activ_s));
      if(atoi(songs) != 0){
      newAlarm(atoi(hours), atoi(mins), atoi(songs), atoi(activ));
      }
    }
    if((server.args() == 1) && (arguments.substring(0,3) == "del")){  //check for deleted or toggled Alarm
      String alar = arguments.substring(3);
      char alrm[16];
      alar.toCharArray(alrm, alar.length() + 1);
      long checksum;
      char *ptr;
      checksum = strtol(alrm, &ptr, 10);
      char del[8];
      String dels = server.arg(0);
      dels.toCharArray(del, sizeof(dels));
      int delA = atoi(del);
      if(delA == 1)
        delAlarm(checksum);
      toggleAlarm(checksum);
    }
    alarmsactive = activeAlarms();  //update the alarmsactive variable
    if(alarmsactive)  //update the active_alarms variable
      active_alarms = getActiveAlarms(); 
    else
      active_alarms = "None";
    server.send(200, "text/html", WECKER + htmlAlarms() + "<a href=\"wecker\">" + SONGINFO);
    yield();
  });
  Serial.print("ready: ");
  Serial.println(dbg++); //####################  5
  //############################  Start Webserver #########################################################
  
  server.begin();

  last = millis();

  attachInterrupt(digitalPinToInterrupt(alarmOff_button), button_alarmOff, RISING);
  attachInterrupt(digitalPinToInterrupt(sleep_button), button_sleep, RISING);

  Serial.print("ready: ");
  Serial.println(dbg++); //####################  6
  

  SoftSerial.begin(9600);
  delay(100);
  Serial.println("SoftSerial established");
  Serial.println("NOW CRASH!!!!!!!!!");
  dfplayer.begin(SoftSerial);
  delay(100);
  Serial.println("dfplayer started");
  dfplayer.volume(15);
  //digitalWrite(alarm_led, HIGH);

  Serial.print("ready: ");
  Serial.println(dbg++); //####################  7

  Serial.println("finished setup");
 }


void loop() {
  // put your main code here, to run repeatedly:
//############################ call handlers  ###################################################
  ArduinoOTA.handle();
  server.handleClient();

//############################ check buttons ####################################

if((last + 200) < millis()){

    //##### debounce Buttons ###########
    if(db_aoff > 0){
      db_aoff = db_aoff - 1;
    }
    if(db_sleep > 0){
      db_sleep = db_sleep - 1;
    }
    //##### show last part of the ip if the user so desires ##############
    if(!show){
      update_time();
    }else{
      int relip = WiFi.localIP()[3];
      
      int lsb = relip % 10;
      int msb = relip / 10;

      digitalWrite(latch, LOW);
      display_4(numbers[0],numbers[0],numbers[msb],numbers[lsb]);
      digitalWrite(latch, HIGH);
    }
    //###### deactivate amp and blink led #################
    if(!sleep){
      digitalWrite(alarm_led, HIGH);
      if(alarmsactive){wake(hour(), minute());}
    }else{
      sleep_wake(hour(), minute());
    }

    
    Serial.print("aoff: ");
    if(aoff){Serial.print("true ;");}
    else{Serial.print("false ;");}
    
    Serial.print("slee: ");
    if(slee){Serial.print("true ;");}
    else{Serial.print("false ;");}
    
    Serial.println("");
    
    last = millis();
    
    
    
  }

  yield();
}
