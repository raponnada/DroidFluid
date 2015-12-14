package edu.umkc.ofcontroller;


import android.app.Activity;
import android.util.Log;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;

public class ControllerSync {


    private boolean master;
    private String local_ip;
    private String local_mac;
    private String master_ip;
    private String master_mac;


    public boolean isMaster() {
        return master;
    }
    public void setMaster(boolean master) {
        this.master = master;
    }

    public String getLocal_ip() {
        return local_ip;
    }
    public void setLocal_ip(String local_ip) {
        this.local_ip = local_ip;
    }

    public String getLocal_mac() {
        return local_mac;
    }
    public void setLocal_mac(String local_mac) {
        this.local_mac = local_mac;
    }

    public void setMaster_ip(String master_ip){ this.master_ip = master_ip; }
    public String getMaster_ip(){ return this.master_ip; }

    public void setMaster_mac(String master_mac){ this.master_mac = master_mac; }
    public String getMaster_mac(){ return this.master_mac; }

    private FileInputStream in;
    private FileOutputStream out;

    /*
     *
     *
     * */
    public void leaderElection() {
        try {

            OFController.processArp = Runtime.getRuntime().exec(new String[]{"su", "-c", OFController.fileDir.getParent() + "/lib/libarpscan.so"});
            in = new FileInputStream("/sdcard/Dfluid/devices.txt");

            BufferedReader br = new BufferedReader(new InputStreamReader(in));
            String line = br.readLine();

            String slave_ip;
            String slave_mac;
            int status;
            master = false;

            local_ip = line.split(" ")[0];
            local_mac = line.split(" ")[1];
            Log.i("local_ip",local_ip);
            Log.i("local_mac",local_mac);

            master_ip = local_ip;
            master_mac = local_mac;
            Log.i("master_ip",master_ip);
            Log.i("master_mac",master_mac);

            while ((line = br.readLine()) != null) {
                slave_ip = line.split(" ")[0];
                slave_mac = line.split(" ")[1];
                Log.i("slave_ip",slave_ip);
                Log.i("slave_mac",slave_mac);

                //if (slave_ip.split(".")[3].compareTo("1") == 0 || slave_ip.split(".")[3].compareTo("255") == 0 )
                //    continue;
                status = master_mac.compareToIgnoreCase(slave_mac);
                Log.i("Status",Integer.toString(status));
                if (status < 0) {
                    master_ip = slave_ip;
                    master_mac = slave_mac;
                }
            }

            if(master_ip.compareTo(local_ip) == 0)
            {
                master = true;
                Thread t = new Thread(new Controller());
                t.start();
            }
            Log.i("master_ip",master_ip);
            Log.i("master_mac",master_mac);

            OFController.process = Runtime.getRuntime().exec(new String[]{"su", "-c", OFController.fileDir.getParent() + "/lib/libofswitch.so"});
            br.close();
        } catch (IOException e) {
        }
    }

    public void heartbeat() {
        Log.i("hearbeat recievert1"," socket setup");
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    final DatagramSocket masterSocket = new DatagramSocket(9003);
                    byte[] recHeartBeat = new byte[2048];
                    Log.i("hearbeat recievert2"," socket setup");
                    if (master) {
                        while (true) {
                            Log.i("cheking heartbeats"," on 9003");
                            final DatagramPacket recpkt = new DatagramPacket(recHeartBeat, recHeartBeat.length);
                            masterSocket.receive(recpkt);
                            // TODO send notification
                            final String recpktBody = new String(recpkt.getData());
                            if (recpktBody.startsWith("alive")) {
                                int port = recpkt.getPort();
                                InetAddress srcip = recpkt.getAddress();
                                final DatagramPacket senpkt = new DatagramPacket("yes".getBytes(), "yes".getBytes().length, srcip, port);
                                masterSocket.send(senpkt);
                            }
                        }
                    } else {
                        while (true) {
                            InetAddress destip = InetAddress.getByName(master_ip);
                            final DatagramPacket senpkt = new DatagramPacket("alive".getBytes(), "alive".getBytes().length, destip, 9003);
                            masterSocket.send(senpkt);
                            Log.i("HearBeat msg sent", master_ip);

                            // Acknowledge to be recieved in 3 secs. ::: Timeout=3sec
                            masterSocket.setSoTimeout(2000);
                            final DatagramPacket recpkt = new DatagramPacket(recHeartBeat, recHeartBeat.length);
                            try {
                                masterSocket.receive(recpkt);
                                Log.i("HeartBeat Ack recieved", recpkt.getAddress().toString());
                            }catch (SocketTimeoutException e){
                                leaderElection();
                            }

                            // TODO receive notification
                            // TODO: sleep
                            Thread.sleep(10000);

                        }
                    }
                } catch (Exception e) {

                }
            }
        }).start();
    }

    public void masterCheck() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    String status = "no";
                    final DatagramSocket checkSocket = new DatagramSocket(9004);
                    byte[] mastercheck = new byte[1024];
                    while (true) {
                        final DatagramPacket chkrpkt = new DatagramPacket(mastercheck, mastercheck.length);
                        Log.i("master check", " 9004");
                        checkSocket.receive(chkrpkt);
                        // TODO send notification
                        Log.i("master check pkt in"," 9004");

                        final String pktData = new String(chkrpkt.getData());
                        Log.i("pkt body", pktData);
                        if (pktData.startsWith("master")) {
                            int port = chkrpkt.getPort();
                            InetAddress srcip = chkrpkt.getAddress();
                            Log.i("src port", Integer.toString(port));
                            Log.i("src port", srcip.toString())
                            ;
                            if(master){
                                status = "yes";
                            }
                            else {
                                status = "no";
                            }
                            final DatagramPacket chkspkt = new DatagramPacket(status.getBytes(), status.getBytes().length, srcip, port);
                            checkSocket.send(chkspkt);
                        }
                    }

                } catch (Exception e) {

                }
            }
        }).start();
    }


}


