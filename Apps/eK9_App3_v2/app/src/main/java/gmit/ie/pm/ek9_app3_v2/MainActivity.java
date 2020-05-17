package gmit.ie.pm.ek9_app3_v2;

import androidx.appcompat.app.AppCompatActivity;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Method;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private TextView text1;
    private TextView text2;
    private Button scanBtt;
    private Button paredBtt;
    private ListView listView;

    private Handler handler;

    private TextView mBluetoothStatus;
    private Handler mHandler; // Our main handler that will receive callback notifications

    private ConnectedThread mConnectedThread; // bluetooth background worker thread to send and receive data
    private BluetoothAdapter bleAdapter;
    private BluetoothSocket socketBTT = null;
    private ArrayAdapter<String> listOfAdapters;
    Set<BluetoothDevice> pairedDevices;
    private final String TAG = MainActivity.class.getSimpleName();

    // #defines for identifying shared types between calling functions
    private final static int REQUEST_ENABLE_BT = 1; // used to identify adding bluetooth names
    private final static int MESSAGE_READ = 2; // used in bluetooth handler to identify message update
    private final static int CONNECTING_STATUS = 3; // used in bluetooth handler to identify message status

    private static final UUID BTMODULEUUID = UUID.fromString("04fafc201-1fb5-459e-8fcc-c5c9c331914b"); // "random" unique identifier







    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        text1 = (TextView) findViewById(R.id.stateText);
        scanBtt = (Button) findViewById(R.id.scanBD);
        paredBtt = (Button) findViewById(R.id.pairedBD);
        //iditiating list view and passing new data to it
        listView = (ListView)findViewById(R.id.listOf);
        listOfAdapters = new ArrayAdapter<String>(this,android.R.layout.simple_list_item_1);
        listView.setAdapter(listOfAdapters);
        listView.setOnItemClickListener(listOfListener);

        //get bluetooth adapter control
        bleAdapter = BluetoothAdapter.getDefaultAdapter();

        mHandler = new Handler(){
            public void handleMessage(android.os.Message msg){
                if(msg.what == MESSAGE_READ){
                    String readMessage = null;
                    try {
                        readMessage = new String((byte[]) msg.obj, "UTF-8");
                    } catch (UnsupportedEncodingException e) {
                        e.printStackTrace();
                    }
                    text1.setText(readMessage);
                }

                if(msg.what == CONNECTING_STATUS){
                    if(msg.arg1 == 1)
                        mBluetoothStatus.setText("Connected to Device: " + (String)(msg.obj));
                    else
                        mBluetoothStatus.setText("Connection Failed");
                }
            }
        };

        //checking is blt is on
        if (!bleAdapter.isEnabled()) {
            Intent turnOn = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(turnOn, 0);
            Toast.makeText(getApplicationContext(), "Enabling BLE",Toast.LENGTH_LONG).show();
        } else {
            Toast.makeText(getApplicationContext(), "BLE already Enabled", Toast.LENGTH_LONG).show();
        }

        //list for paired devices
        pairedDevices = bleAdapter.getBondedDevices();

        if (pairedDevices.size() > 0) {
            // There are paired devices. Get the name and address of each paired device.
            for (BluetoothDevice device : pairedDevices) {
                String deviceName = device.getName();
                String deviceHardwareAddress = device.getAddress(); // MAC address
            }
        }

        //scan buton starts scan
        scanBtt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(bleAdapter.isDiscovering()){
                    bleAdapter.cancelDiscovery();
                    Toast.makeText(getApplicationContext(),"Scan Canceld",Toast.LENGTH_SHORT).show();
                }else{
                    if(bleAdapter.isEnabled()){
                        listOfAdapters.clear();//cleare list
                        text1.setText("searching...");
                        bleAdapter.startDiscovery();//starting scan
                        Toast.makeText(getApplicationContext(),"Scan Started",Toast.LENGTH_SHORT).show();
                        registerReceiver(bleReceiver, new IntentFilter(BluetoothDevice.ACTION_FOUND));//registering each device was found
                    }else{
                        Toast.makeText(getApplicationContext(),"BLE not on",Toast.LENGTH_SHORT).show();
                    }
                }
            }

           private final BroadcastReceiver bleReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();
                    if(BluetoothDevice.ACTION_FOUND.equals(action)){
                        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                        //adding data to list
                        listOfAdapters.add(device.getName() + " : " + device.getAddress());
                        listOfAdapters.notifyDataSetChanged();
                        if(device.getName().equals("eK9 transmiting")){
                            text1.setText(device.getName());
                            bleAdapter.cancelDiscovery();
                        }
                    }

                }
            };
        });

        paredBtt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                listOfAdapters.clear();
                pairedDevices = bleAdapter.getBondedDevices();
                if(bleAdapter.isEnabled()){
                    //filing list with paired devises if found
                    for (BluetoothDevice device : pairedDevices)
                        listOfAdapters.add(device.getName() + " : " + device.getAddress());
                    Toast.makeText(getApplicationContext(), "Showing Paired Devices", Toast.LENGTH_SHORT).show();
                }else{
                    Toast.makeText(getApplicationContext(), "Bluetooth not on", Toast.LENGTH_SHORT).show();
                }
            }
        });


    }


    private AdapterView.OnItemClickListener listOfListener = new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {

            text1.setText("Connecting...");
            // Get the device MAC address, which is the last 17 chars in the View
            String info = ((TextView) view).getText().toString();
            final String address = info.substring(info.length() - 17);
            final String name = info.substring(0,info.length() - 17);

            new Thread(){
                public void run(){
                    boolean fail = false;

                    BluetoothDevice device = bleAdapter.getRemoteDevice(address);
                    //create a socket for two way comms
                    try{
                        socketBTT = createBluetoothSocket(device);
                    } catch (IOException e){
                        fail = true;
                        Toast.makeText(getBaseContext(), "Socket creation failed", Toast.LENGTH_SHORT).show();
                    }

                    //make a connection
                    try{
                        socketBTT.connect();
                    }catch (IOException e){
                        try{
                            fail = true;
                            socketBTT.close();

                        }catch (IOException e2){
                            Toast.makeText(getBaseContext(), "Socket creation failed", Toast.LENGTH_SHORT).show();
                        }
                    }
                    if(fail == false){
                        mConnectedThread = new ConnectedThread(socketBTT);
                        mConnectedThread.start();

                    }
                }
            }.start();
        }
    };

    private BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
        try {
            final Method m = device.getClass().getMethod("createInsecureRfcommSocketToServiceRecord", UUID.class);
            return (BluetoothSocket) m.invoke(device, BTMODULEUUID);
        } catch (Exception e) {
            Log.e(TAG, "Could not create Insecure RFComm Connection",e);
        }
        return  device.createRfcommSocketToServiceRecord(BTMODULEUUID);
    }

    private class ConnectedThread extends Thread {
        private final BluetoothSocket mmSocket;
        private final InputStream mmInStream;
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket) {
            mmSocket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            // Get the input and output streams, using temp objects because
            // member streams are final
            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) { }

            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run() {
            byte[] buffer = new byte[1024];  // buffer store for the stream
            int bytes; // bytes returned from read()
            // Keep listening to the InputStream until an exception occurs
            while (true) {
                try {
                    // Read from the InputStream
                    bytes = mmInStream.available();
                    if(bytes != 0) {
                        buffer = new byte[1024];
                        SystemClock.sleep(100); //pause and wait for rest of data. Adjust this depending on your sending speed.
                        bytes = mmInStream.available(); // how many bytes are ready to be read?
                        bytes = mmInStream.read(buffer, 0, bytes); // record how many bytes we actually read
                        mHandler.obtainMessage(MESSAGE_READ, bytes, -1, buffer)
                                .sendToTarget(); // Send the obtained bytes to the UI activity
                        //text1.setText(bytes);
                        //text1.notify();
                    }
                } catch (IOException e) {
                    e.printStackTrace();

                    break;
                }
            }
        }

        /* Call this from the main activity to send data to the remote device */
        public void write(String input) {
            byte[] bytes = input.getBytes();           //converts entered String into bytes
            try {
                mmOutStream.write(bytes);
            } catch (IOException e) { }
        }

        /* Call this from the main activity to shutdown the connection */
        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) { }
        }


    }
}
