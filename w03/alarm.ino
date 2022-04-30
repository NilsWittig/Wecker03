









//########################  Functions for handling the Alarms #######################################################
//an Alarm is a long number calculated as such: (hour * 10^6) + (minute * 10^4) + (song * 10) + active
//hh mm sss a
//12 30 999 1
bool isAlarm(long checksum){    //Checks if an Alarm is in the list of Alarms
  for (SimpleList<long>::iterator itr = alarms.begin(); itr != alarms.end();){
    if ((*itr) ==  checksum)
    {return true;}
    ++itr;
  }
  return false;
}
void newAlarm(long h, long m, long s, long a){    //adds a new Alarm to the list of Alarms, if the added Alarm isn't in the list yet
  long na = (h * long(1000000)) + (m * long(10000)) + (s * 10) + a;
  if (not isAlarm(na)){
    alarms.push_back(na);
  }
}
void delAlarm(long checksum){   //deletes an Alarm from the list of Alarms
  for (SimpleList<long>::iterator itr = alarms.begin(); itr != alarms.end();){
    if ((*itr) ==  checksum)
    {
      itr = alarms.erase(itr);
      break;
      }
    ++itr;
  }
}
void toggleAlarm(long checksum){    //toggles the active bit of an Alarm in the list of Alarms
  for (SimpleList<long>::iterator itr = alarms.begin(); itr != alarms.end();){
    if ((*itr) ==  checksum)
    {
      int *x = getAlarm(*itr);
      if(x[3] == 1){
        (*itr) = checksum - long(1);
      }else{
        (*itr) = checksum + long(1);
      }
      free(x); x=NULL;
      break;
      }
    ++itr;
  }
  yield();
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
  yield();
  return ar;
}
String htmlAlarms(){    //builds a html string to be append to the website string wich contains info about all Alarms and the ability to edit them
  
  String ahtml = "";
  ahtml += "<UL>";
  for (SimpleList<long>::iterator itr = alarms.begin(); itr != alarms.end();){
    int *x = getAlarm(*itr);
    ahtml += "<LI>";
    //ahtml += "<p>";
    ahtml += "<form>";
    ahtml += "<form action=\"AlarmMod.asp\" method=\"get\">";
    //ahtml += "#### ";
    if(x[0] < 10){
      ahtml += "0";
    }
    ahtml += x[0];
    ahtml += ":";
    if(x[1] < 10){
      ahtml += "0";
    }
    ahtml += x[1];
    //ahtml += ", Song: ";
    //ahtml += x[2];
    ahtml += ", " + SONGS[x[2] - 1] + ", ";
    int len = SONGS[x[2] - 1].length();
    for(int space = 0; space < 30 - len; space++){
      ahtml += "&ensp;";
    }
    if(x[3] == 0){
      ahtml += "Inaktiv ";
    }else{
      ahtml += "Aktiv &ensp;";
    }
    //ahtml += " ####";
    ahtml += "<input type=\"number\" min=\"0\" max=\"1\" name=\"del";
    ahtml += (*itr);
    ahtml += "\" value=\"Del\" >";
    ahtml += "<input type=\"submit\" value=\"Toggle/Delete\">";
    ahtml += "</form>";
    //ahtml += "</p>";
    free(x); x=NULL;
    ++itr;
  }
  yield();
  ahtml += "</UL>";
  return ahtml;
}
bool activeAlarms(){    //Checks if at least one Alarm in the list of Alarms is active
  for (SimpleList<long>::iterator itr = alarms.begin(); itr != alarms.end();){
    int *x = getAlarm(*itr);
    if(x[3] == 1){
      free(x); x=NULL;
      return true;
    }
    free(x); x=NULL;
    ++itr;
  }
  yield();
  return false;
}
String getActiveAlarms(){   //returns a String with hour and minute of every active Alarm
  String acal = "";
  for (SimpleList<long>::iterator itr = alarms.begin(); itr != alarms.end();){
    int *x = getAlarm(*itr);
    if(x[3] == 1){
      acal += "[";
      acal += x[0];
      acal += ":";
      acal += x[1];
      acal += "]";
    }
    free(x); x=NULL;
    ++itr;
  }
  yield();
  return acal;
}
//################################# Functions for handling waking / ringing of Alarms and sleepmode ########################
void sleep_wake(int hour, int min){   //rings out of sleep mode         needs to be fixed currently doesn't stop the mp3 from playing!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! and thats ok
  if(next_repeat == (min + (60 * hour))){
    dfplayer.play(curr_song);
    ringing = true;
    if(curr_repeat == 0){
      sleep = false;
      curr_song = 1;
      next_repeat = -1;
      curr_checksum = -1;
    }else{
      curr_repeat -= 1;
      next_repeat += 5;
    }
  }else if(next_repeat == -1){
    if(alarm_off == 200){
      ringing = false;
      sleep = false;
      alarm_off = 0;
    }else{
      alarm_off++;
    }
  }
  yield();
}
void wake(int hour, int min){   //ring for the first time and sets everything up for sleepmode
  for (SimpleList<long>::iterator itr = alarms.begin(); itr != alarms.end();){
    int *x = getAlarm(*itr);
    if((x[0] == hour) && (x[1] == min) && (x[3] == 1)){
      digitalWrite(alarm_led, LOW);
      ringing = true;
      sleep = true;
      curr_song = x[2];
      dfplayer.play(curr_song);
      next_repeat = 5 + (x[1] + (60 * x[0]));
      curr_checksum = (*itr);
      curr_repeat = repeats;
      free(x); x=NULL;
      break;
    }
    free(x); x=NULL;
    ++itr;
  }
  yield();
}
void button_alarmOff(){   //turns off ringing and sleepmode
  if(db_aoff == 0){
    ringing = false;
    sleep = false;
    aoff = !aoff;
  
    dfplayer.pause();

    db_aoff = 4;
  }
}
void button_sleep(){    //tunrs off ringing
  if(db_sleep == 0){
    if(!ringing){ // show ip
      show = !show;
    }
    ringing = false;
    slee = !slee;
  
    dfplayer.pause();

    db_sleep = 4;
  }
}









