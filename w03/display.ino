




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
