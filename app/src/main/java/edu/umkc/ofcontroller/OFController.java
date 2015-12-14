package edu.umkc.ofcontroller;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.app.Activity;
import android.util.Log;
import android.widget.ScrollView;
import android.widget.TextView;

class Controller implements Runnable
{
    static {
        System.loadLibrary("gnustl_shared");
        System.loadLibrary("ofcontroller");
    }

    public void run() {
        startController(6653, 4);
    }

    native void startController(int port, int nthreads);
}


public class OFController extends Activity
{
	private TextView switchTextView;
    private TextView controllerTextView;

    public ArrayList<String> device_ips;
    public ArrayList<String> device_macs;
    private Handler logUpdaterHandler;
	final private int logUpdaterInterval = 1000;

    public static File fileDir;
    public static Process process;
    public static Process processArp;
    private BufferedReader bufferedReader;

    public int devCount;
    public String line;
    public FileInputStream in;
    public BufferedReader br;
    public ControllerSync cntrlSync;

    private Runnable logUpdater = new Runnable() {
		@Override
		public void run() {
			updateSwitchLog();
            updateControllerLog();
			logUpdaterHandler.postDelayed(logUpdater, logUpdaterInterval);
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState)
    {
		super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ofcontroller);

        //Controller UI
        controllerTextView = (TextView)findViewById(R.id.controllerView);
        final ScrollView controllerSV = (ScrollView)findViewById(R.id.scrollView);

        //switch UI
        switchTextView = (TextView)findViewById(R.id.switchView);
        final ScrollView sv = (ScrollView)findViewById(R.id.scrollView2);

        execution_flow();
        logUpdaterHandler = new Handler();
        logUpdater.run();

    }

    public void execution_flow(){

        try {
            fileDir = getFilesDir();
            processArp = Runtime.getRuntime().exec(new String[]{"su", "-c", fileDir.getParent() + "/lib/libarpscan.so"});
            device_ips = new ArrayList<String>();
            device_macs = new ArrayList<String>();
            Thread.sleep(10000);
            in = new FileInputStream("/sdcard/Dfluid/devices.txt");
            br = new BufferedReader(new InputStreamReader(in));
            devCount = 0;
            cntrlSync = new ControllerSync();

            Log.i("Something is","wrong");
            while ((line = br.readLine()) != null) {
                devCount++;
                if (devCount == 1) {
                    cntrlSync.setLocal_ip(line.split(" ")[0]);
                    Log.i("localip", cntrlSync.getLocal_ip());
                    cntrlSync.setLocal_mac(line.split(" ")[1]);
                    Log.i("localmac", cntrlSync.getLocal_mac());
                }

                Log.i("devcount",String.valueOf(devCount));
                device_ips.add(line.split(" ")[0]);
                device_macs.add(line.split(" ")[1]);
            }

            /** thread to acknowledge that this device is master controller or not  **/
            cntrlSync.masterCheck();
            if (devCount > 1) {
                // TODO probe all the controllers on the network
                cntrlSync.setMaster(false);
                final DatagramSocket slaveSocket;

                Log.i("deviceips", device_ips.get(0));
                Log.i("deviceips", device_ips.get(1));
                Thread temp_t = new Thread(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            DatagramPacket masterCheckPkt = null;
                            DatagramSocket slaveSocket = new DatagramSocket(9002);
                            for (String tmpIp : device_ips) {
                                Log.i("tmpIp", tmpIp);
                                if (tmpIp.compareTo(cntrlSync.getLocal_ip()) == 0) {// && tmpIp.split(".")[3].compareTo("1") != 0 && tmpIp.split(".")[3].compareTo("255") != 0 ) {
                                    continue;
                                }
                                masterCheckPkt = new DatagramPacket("master".getBytes(), "master".getBytes().length, InetAddress.getByName(tmpIp), 9004);
                                Log.i("ips:", tmpIp);
                                slaveSocket.send(masterCheckPkt);

                            }
                            while (true) {
                                byte[] recvMaster = new byte[2048];
                                final DatagramPacket recpkt = new DatagramPacket(recvMaster, recvMaster.length);
                                Log.i("recpkt:", "No packet has been recieved so far");
                                slaveSocket.receive(recpkt);
                                final String recpktbody = new String(recpkt.getData());
                                Log.i("recpkt data:",recpktbody);
                                if (recpktbody.startsWith("yes")) {
                                    cntrlSync.setMaster_ip(recpkt.getAddress().toString().substring(1));
                                    Log.i("Master ip", cntrlSync.getMaster_ip());
                                    break;
                                }
                            }
                        } catch (Exception e) {
                            e.printStackTrace();

                        }
                    }
                });
                temp_t.start();
                temp_t.join();
            }
            else {
                    cntrlSync.setMaster(true);// set this device as a masterController
                    cntrlSync.setMaster_ip(cntrlSync.getLocal_ip());
                    cntrlSync.setMaster_mac(cntrlSync.getLocal_mac());
                    //Log.i("masterip", cntrlSync.getMaster_ip());
                    //Log.i("mastermac", cntrlSync.getMaster_mac());
                    Thread t = new Thread(new Controller());
                    t.start();
            }



            cntrlSync.heartbeat();           // acknowledge heartbeat messages from slave controllers
            leaderIp(cntrlSync);
            process = Runtime.getRuntime().exec(new String[]{"su", "-c", getFilesDir().getParent() + "/lib/libofswitch.so"});


        } catch (IOException e) {
            e.printStackTrace();
        }
        catch (InterruptedException e) {
            e.printStackTrace();
        }

    }
    public void leaderIp(final ControllerSync ctrlSync) {
        (new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    final DatagramSocket serverSocket = new DatagramSocket(9000);
                    byte[] receiveData = new byte[1024];
                    byte[] sendData = new byte[1024];

                    while(true) {
                        final DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
                        Log.i("DroidFluid", "WAITING...");
                        serverSocket.receive(receivePacket);

                        final String sentence = new String(receivePacket.getData());
                        Log.i("DroidFluid", "RECEIVED: " + sentence);

                        final int port = receivePacket.getPort();
                        final InetAddress IPAddress = receivePacket.getAddress();

                        sendData = ctrlSync.getMaster_ip().getBytes();

                        final DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, port);
                        serverSocket.send(sendPacket);
                    }
                }
                catch (Exception e) {
                    Log.i("DroidFluid", "Error: " + e.getMessage());
                }
            }
        })).start();
    }

    void updateSwitchLog()
    {
        try {
			final StringBuilder log = new StringBuilder();
            bufferedReader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            while (bufferedReader.ready()) {
                final String sline = bufferedReader.readLine();

                log.append(sline + "\n");
            }

            if (!log.toString().isEmpty()) {
                Log.i("DroidFluid", "Native message: " + log.toString());

                switchTextView.setText(log.toString());
            }
		} catch (IOException e) {
            switchTextView.setText(e.getMessage());
		}
    }
    void updateControllerLog() {
        String pid = android.os.Process.myPid() + "";
        try {
            Process processs = Runtime.getRuntime().exec(
                    "logcat -d OFCONTROLLER:V *:S");
            BufferedReader bufferedReaderCntrl = new BufferedReader(
                    new InputStreamReader(processs.getInputStream()));

            StringBuilder log = new StringBuilder();
            String logline;
            while ((logline = bufferedReaderCntrl.readLine()) != null) {
                if (logline.contains(pid)) {
                    log.append(logline.split(": ")[1] + "\n");
                }
            }

            controllerTextView.setText(log.toString());
        } catch (IOException e) {
        }
    }
}
