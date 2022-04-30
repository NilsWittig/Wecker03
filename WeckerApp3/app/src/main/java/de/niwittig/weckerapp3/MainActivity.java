package de.niwittig.weckerapp3;

import android.app.TimePickerDialog;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.TimePicker;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Calendar;
import java.util.concurrent.Callable;

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
    static final int times = 5;
    static final String filename = "w03file.txt";

    boolean start = true;
    boolean network_error = false;

    ImageButton refresh;

    TextView timeTexts[] = new TextView[times];
    int timeTextIds[] = {R.id.time0, R.id.time1, R.id.time2, R.id.time3, R.id.time4};
    int times_hour[] = new int[times];
    int times_minute[] = new int[times];

    EditText editText;
    String songs_default = "ThePackage; Moon; Agostina; PianoLessons; Dreams; Fight; GrandCanyon; Night; DeepPeace; TheBlizzard; Horizons; Terminal; Earthrise; Away; Greetings; Fadeaway; ADaisyThroughConcrete; APillowOfWinds; ";
    String songs_array[];

    EditText url;
    String defaultURL = "192.168.0.115";

    Spinner spinners[] = new Spinner[times];
    int spinner_pos[] = new int[times];
    int spinnerIds[] = {R.id.spinner0, R.id.spinner1, R.id.spinner2, R.id.spinner3, R.id.spinner4};

    Switch switches[] = new Switch[times];
    int switchIds[] = {R.id.switch0, R.id.switch1, R.id.switch2, R.id.switch3, R.id.switch4};
    boolean actives[] = new boolean[times];

    File directory;
    String app_dir = "config";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        fileRead();

        for (int i = 0; i < times; i++) {
            timeTexts[i] = (TextView) findViewById(timeTextIds[i]);
            int finalI = i;
            timeTexts[i].setOnClickListener(new View.OnClickListener() {
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
                            times_hour[finalI] = selectedHour;
                            times_minute[finalI] = selectedMinute;
                            String time_s = "";
                            if(times_hour[finalI] < 10) time_s += "0";
                            time_s += times_hour[finalI];
                            time_s += ":";
                            if(times_minute[finalI] < 10) time_s += "0";
                            time_s += times_minute[finalI];
                            timeTexts[finalI].setText(time_s);
                        }
                    }, hour, minute, true);//Yes 24 hour time
                    mTimePicker.setTitle("Select Time");
                    mTimePicker.show();

                }
            });
        }

        songs_array = songs_default.split("; ");
        ArrayAdapter aa = new ArrayAdapter(this, android.R.layout.simple_spinner_item, songs_array);
        aa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        for (int i = 0; i < times; i++) {
            int finalI = i;
            spinners[i] = (Spinner) findViewById(spinnerIds[i]);
            spinners[i].setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                    spinner_pos[finalI] = i;
                }

                @Override
                public void onNothingSelected(AdapterView<?> adapterView) {
                }
            });
            spinners[i].setAdapter(aa);
        }

        editText = (EditText) findViewById(R.id.songs);
        editText.setText(songs_default);

        url = (EditText) findViewById(R.id.ip);
        url.setText(defaultURL);

        refresh = (ImageButton) findViewById(R.id.imageButton);
        refresh.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                refresh();
            }
        });

        for (int i = 0; i < times; i++) {
            int finalI = i;
            actives[i] = false;
            switches[i] = (Switch) findViewById(switchIds[i]);
            switches[i].setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    actives[finalI] = switches[finalI].isChecked();
                }
            });
        }
        //Toast.makeText(getApplicationContext(), "Ready", Toast.LENGTH_LONG).show();
        refresh();
    }

    void refresh() {
        defaultURL = url.getText().toString();
        songs_default = editText.getText().toString();
        songs_array = songs_default.split("; ");
        ArrayAdapter aa = new ArrayAdapter(MainActivity.this, android.R.layout.simple_spinner_item, songs_array);
        aa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        for (int i = 0; i < times; i++) {
            spinners[i].setAdapter(aa);
        }
        fileWrite();
        fileRead();
        network_error = false;
        String resp = "";
        if(start) {
            resp = weckerGet();
            if(!network_error) updateParams(resp);
            start = false;
            Toast.makeText(getApplicationContext(),"Synched" , Toast.LENGTH_LONG).show();
        } else {
            resp = weckerSet();
            if(!network_error) updateParams(resp);
            Toast.makeText(getApplicationContext(),"Refreshed" , Toast.LENGTH_LONG).show();
        }
    }

    void fileWrite() {
        String data = defaultURL + "#" + songs_default;
        try {
            OutputStreamWriter outputStreamWriter = new OutputStreamWriter(getApplicationContext().openFileOutput(filename, Context.MODE_PRIVATE));
            outputStreamWriter.write(data);
            outputStreamWriter.close();
            System.out.println("save file write:" + data);
        } catch (IOException e) {
            System.out.println("write fail");
            Toast.makeText(getApplicationContext(), "save file unwritable", Toast.LENGTH_LONG).show();
        }
    }

    void fileRead() {
        String ret = "";
        try {
            InputStream inputStream = getApplicationContext().openFileInput(filename);
            if (inputStream != null) {
                InputStreamReader inputStreamReader = new InputStreamReader(inputStream);
                BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
                String receiveString = "";
                StringBuilder stringBuilder = new StringBuilder();
                while ((receiveString = bufferedReader.readLine()) != null) {
                    stringBuilder.append(receiveString);
                }
                inputStream.close();
                ret = stringBuilder.toString();
            }
        } catch (FileNotFoundException e) {
            Toast.makeText(getApplicationContext(), "save file not found: create it", Toast.LENGTH_LONG).show();
            System.out.println("file not found: create it");
            fileWrite();
        } catch (IOException e) {
            System.out.println("read fail");
            Toast.makeText(getApplicationContext(), "save file unreadable", Toast.LENGTH_LONG).show();
        }
        String splits[] = ret.split("#");
        if (splits.length != 2) {
            Toast.makeText(getApplicationContext(), "save file corrupt: rewriting", Toast.LENGTH_LONG).show();
            fileWrite();
        } else {
            defaultURL = splits[0];
            songs_default = splits[1];
        }
        System.out.println("save file read:" + ret);
    }


    String getCallById(int id) {
        String s = new String("");
        s += id + ",";
        s += times_hour[id] + ",";
        s += times_minute[id] + ",";
        s += spinner_pos[id] + ",";
        s += actives[id] ? "1" : "0";
        return s;
    }

    String getAllCalls() {
        String s = new String();
        for (int i = 0; i < times; i++) {
            s += getCallById(i);
            if (i < (times - 1)) s += ";";
        }
        return s;
    }

    String getCmdById(int id) {
        String s = new String("");
        s += id;
        if(times_hour[id] < 10) s += "0";
        s += times_hour[id];
        if(times_minute[id] < 10) s += "0";
        s += times_minute[id];
        if(spinner_pos[id] < 100) s += "0";
        if(spinner_pos[id] < 10) s += "0";
        s += spinner_pos[id];
        s += actives[id] ? "1" : "0";
        return s;
    }

    void updateParams(String resp) {
        String splits[] = resp.split(";");
        int id = 0;
        for(String alarm : splits) {
            String params[] = alarm.split(",");
            times_hour[id] = Integer.parseInt(params[0]);
            times_minute[id] = Integer.parseInt(params[1]);
            spinner_pos[id] = Integer.parseInt(params[2]);
            actives[id] = (Integer.parseInt(params[3]) == 1);

            String time_s = "";
            if(times_hour[id] < 10) time_s += "0";
            time_s += times_hour[id];
            time_s += ":";
            if(times_minute[id] < 10) time_s += "0";
            time_s += times_minute[id];
            timeTexts[id].setText(time_s);
            spinners[id].setSelection(spinner_pos[id], true);
            switches[id].setChecked(actives[id]);
            id++;
        }
    }


    String weckerGet() {
        String ret = "";
        String full_url = "http://" + defaultURL + "/";
        System.out.println("full_url:" + full_url);

        httpGet getThread = new httpGet(full_url, getApplicationContext());
        Thread thread = new Thread(getThread);
        thread.start();
        try {
            thread.join(500);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        ret = getThread.getResponse();

        if(ret == null) {
            Toast.makeText(getApplicationContext(),"network error" , Toast.LENGTH_LONG).show();
            ret = getAllCalls();
            network_error = true;
        }else if(ret.startsWith("Error:") || ret.charAt(0) == 'E') {
            Toast.makeText(getApplicationContext(),ret , Toast.LENGTH_LONG).show();
            ret = getAllCalls();
            network_error = true;
        }
        return ret;
    }

    String weckerSet() {
        String ret = "Error: timeout";
        String full_url = "http://" + defaultURL + "/";
        for(int i = 0; i < times; i++) {
            String full_url_set = full_url;
            full_url_set += "change";
            full_url_set += getCmdById(i);
            System.out.println("full_url_set:" + full_url_set);
            httpGet getThread = new httpGet(full_url_set, getApplicationContext());
            Thread thread = new Thread(getThread);
            thread.start();
            try {
                //thread.join();
                thread.join(500);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            ret = getThread.getResponse();
        }
        if(ret == null) {
            Toast.makeText(getApplicationContext(),"network error" , Toast.LENGTH_LONG).show();
            ret = getAllCalls();
            network_error = true;
        }else if(ret.startsWith("Error:") || ret.charAt(0) == 'E') {
            Toast.makeText(getApplicationContext(),ret , Toast.LENGTH_LONG).show();
            ret = getAllCalls();
            network_error = true;
        }
        return ret;
    }
}

class httpGet implements Runnable {
    private final String full_url;
    private final Context context;
    private String response;

    public String getResponse() {
        return response;
    }
    @Override
    public void run() {
        URL url = null;
        HttpURLConnection connection = null;
        try {
            url = new URL(full_url);
            connection = (HttpURLConnection) url.openConnection();
            connection.setConnectTimeout(500);
            connection.setReadTimeout(500);
            InputStream is = connection.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
            StringBuilder result = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) { result.append(line); }
            response = result.toString();
            is.close();
            System.out.println("response:" + response);
        } catch (java.net.SocketTimeoutException e) {
            response = "Error: SocketTimeoutException";
            System.out.println("SocketTimeoutException:");
            e.printStackTrace();
        } catch (MalformedURLException e) {
            response = "Error: MalformedURLException";
            System.out.println("MalformedURLException:");
            e.printStackTrace();
        } catch (IOException e) {
            response = "Error: IOException";
            System.out.println("IOException:");
            e.printStackTrace();
        } finally {
            if(connection != null) connection.disconnect();
        }
    }
    public httpGet(String full_url, Context context) {
        this.full_url = full_url;
        this.context = context;
    }
}
