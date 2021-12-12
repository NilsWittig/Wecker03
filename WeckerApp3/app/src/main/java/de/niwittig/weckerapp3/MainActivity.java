package de.niwittig.weckerapp3;

import android.app.TimePickerDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.TimePicker;

import java.util.Calendar;

/*
http://192.168.0.115/change114550131

change A HH MM SSS E
A = alarm number [0:4]
HH = hour 24h
MM = minute
SSS = song number
E = Enable 0=>off

returns all alarm info:
0,0,0,0;14,55,13,1;0,0,0,0;0,0,0,0;0,0,0,0

HH,MM,SSS,E;HH,MM,SSS,E;HH,MM,SSS,E;HH,MM,SSS,E;HH,MM,SSS,E
A = 0      |    1      |    2      |     3     |   4
 */

public class MainActivity extends AppCompatActivity {

    TextView timeText0;
    Button refresh;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        timeText0 = (TextView) findViewById(R.id.time0);
        timeText0.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                Calendar mcurrentTime = Calendar.getInstance();
                int hour = mcurrentTime.get(Calendar.HOUR_OF_DAY);
                int minute = mcurrentTime.get(Calendar.MINUTE);
                TimePickerDialog mTimePicker;
                mTimePicker = new TimePickerDialog(MainActivity.this, new TimePickerDialog.OnTimeSetListener() {
                    @Override
                    public void onTimeSet(TimePicker timePicker, int selectedHour, int selectedMinute) {
                        timeText0.setText( selectedHour + ":" + selectedMinute);
                    }
                }, hour, minute, true);//Yes 24 hour time
                mTimePicker.setTitle("Select Time");
                mTimePicker.show();

            }
        });
    }
}