//########################  Functions for handling the Alarms #######################################################
//an Alarm is a long number calculated as such: (hour * 10^6) + (minute * 10^4) + (song * 10) + active
//hh mm sss a
//12 30 999 1

void button_alarmOff(){   //shuts off alarm
  if(db_aoff == 0){
    ringing = false;
    sleep = false;
    dfplayer.pause();
    digitalWrite(alarm_led, HIGH);//turn off amp and light
    cycle = 0;
    nextCycleTime = 0;
    ringTone = 0;
    db_aoff = 3;
    aoff = !aoff;
  }
}
void button_sleep(){    //turns off ringing
  if(db_sleep == 0){
    if(!ringing){
      if(show){
        if(show_ == 3){show_ = 0;show = false;}
        else{show_++;}
      }else{show = true;}
    }else{
    ringing = false;
    dfplayer.pause();
    db_sleep = 3;
    slee = !slee;
    }
  }
}
int *getAlarm(long checksum){   //Takes an Alarm (as long number) and returns an array of 4 integers with the following meaning: [hour,minute,song,active]
  int *ar = (int *) malloc(sizeof(int) * 4);
  ar[0] = int(checksum / long(1000000));
  checksum -= ar[0] * long(1000000);
  ar[1] = int(checksum  / 10000);
  checksum -= ar[1] * long(10000);
  ar[2] = int(checksum / 10);
  checksum -= ar[2] * 10;
  ar[3] = int(checksum);
  return ar;
  yield();
}

void wake(int h, int m){
  int *tmpAlarm;
  for(int i = 0; i < 5; i++){
    tmpAlarm = getAlarm(alarms[i]);
    if( (tmpAlarm[3] == 1) && (tmpAlarm[0] == h) && (tmpAlarm[1] == m) && (tmpAlarm[2] != 0) ){ //ring!
      ringing = true;
      sleep = true;
      ringTone = tmpAlarm[2];
      nextCycleTime = 5 + tmpAlarm[1] + (60 * tmpAlarm[0]);
      cycle++; // 0 -> 1
      digitalWrite(alarm_led, LOW);//turn on amp and light
      dfplayer.play(ringTone);
      Serial.println("wake");
      break;
    }
    free(tmpAlarm); tmpAlarm = NULL;
  }
  
  yield();
}
void sleepWake(int h, int m){
  if(cycle == 3){// last cycle is over shut alarm down
    ringing = false;
    sleep = false;
    dfplayer.pause();
    digitalWrite(alarm_led, HIGH);//turn off amp and light
    cycle = 0;
    nextCycleTime = 0;
    ringTone = 0;
  }else if(nextCycleTime == (m + (60 * h))){// next cycle
    ringing = true;
    nextCycleTime += 5;
    dfplayer.play(ringTone);
    cycle++;
  }
  yield();
}

//########################################## OmnyHandler [ CHECK IMPLEMENTATION ] #######################################
void handleIT() {
  String url = server.uri();
  Serial.print("url: ");
  Serial.println(url);
  url.remove(0,1);
  
  if(url.startsWith("change")){
    url.remove(0,6);
    int alarmIndex = charToInt( url[0] ); //wich alarm gets changed?
    Serial.print("alarmIndex: ");
    Serial.println(alarmIndex);
    url.remove(0,1);
    long tmp = 0;
    for(int i = 0; i < 8; i++){ // extract long out of the string
      tmp = tmp *10; //move one digit
      tmp += (long) charToInt( url[0] ); // add value
      url.remove(0,1); // delete added value
    }
    alarms[alarmIndex] = tmp;
  }
  
  String response = ""; // the response is a String of all alarms
  int *tmpAlarm;
  for(int i = 0; i < 5; i++){// alarms separated by semicolons
    tmpAlarm = getAlarm(alarms[i]);
    for(int j = 0; j < 4; j++){ // values separated by commas
      response += tmpAlarm[j];
      if(j != 3){ response += ","; }
    }
    if(i != 4){ response += ";"; }
    free(tmpAlarm); tmpAlarm = NULL;
  }
  
  Serial.print("response: ");
  Serial.println(response);
  server.send(200, "text/plain", response);
  yield();
}

int charToInt(char c){
  int result = 0;
  switch(c){
          case 48: { result = 0; break; }
          case 49: { result = 1; break; }
          case 50: { result = 2; break; }
          case 51: { result = 3; break; }
          case 52: { result = 4; break; }
          case 53: { result = 5; break; }
          case 54: { result = 6; break; }
          case 55: { result = 7; break; }
          case 56: { result = 8; break; }
          case 57: { result = 9; break; }
          default: { break; }
        }
   return result;
}
//########################  Function for the 7 Segment Display  ##############################################

void shift(int d){  //Shift single Bit
  digitalWrite(ser, d);
  digitalWrite(srck, HIGH);
  digitalWrite(srck, LOW);
}
void digit(int val[]){  //Shift an entire digit
  for(int i = 0; i<8; i++){shift(val[i]);}
}
void display_4(int first[], int second[], int third[], int fourth[]){   //Shift all four digits --> Shift time
  digit(first);
  digit(second);
  digit(third);
  digit(fourth);
}
void update_time(){   //Shift current time
  int h_low = hour() % 10;
  int h_high = hour() / 10;
  int m_low = minute() % 10;
  int m_high = minute() / 10;
  digitalWrite(latch, LOW);
  display_4(numbers[h_high],numbers[h_low],numbers[m_high],numbers[m_low]);
  digitalWrite(latch, HIGH);
  yield();
}

