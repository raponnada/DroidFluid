package edu.umkc.ofcontroller;


import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class ControllerSync {


    private boolean master;
    private String local_ip;
    private String local_mac;
    private String master_ip;
    private String master_mac;

    private FileInputStream in;
    private FileOutputStream out;

    /*
     *
     *
     * */
    public void leaderElection() {
        try {
            in = new FileInputStream("/data/data/edu.umkc.droidfluid/lib/devices.txt");
            BufferedReader br = new BufferedReader(new InputStreamReader(in));
            String line = br.readLine();

            String slave_ip;
            String slave_mac;
            int status;
            master = false;

            local_ip = line.split(" ")[0];
            local_mac = line.split(" ")[1];
            master_ip = local_ip;
            master_mac = local_mac;

            while ((line = br.readLine()) != null) {
                slave_ip = line.split(" ")[0];
                slave_mac = line.split(" ")[1];
                status = master_ip.compareToIgnoreCase(slave_ip);
                if (status < 0) {
                    master_ip = slave_ip;
                    master_mac = slave_mac;
                }
            }
            br.close();
        } catch (IOException e) {
        }
    }

    public void heartbeat() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    final DatagramSocket masterSocket = new DatagramSocket(9003);
                    byte[] recHeartBeat = new byte[2048];

                    if (master = true) {
                        while (true) {
                            final DatagramPacket recpkt = new DatagramPacket(recHeartBeat, recHeartBeat.length);
                            masterSocket.receive(recpkt);
                            // TODO send notification
                            if (recpkt.getData().toString() == "alive") {
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

                            final DatagramPacket recpkt = new DatagramPacket(recHeartBeat, recHeartBeat.length);
                            masterSocket.receive(recpkt);
                            // TODO receive notification
                            // TODO: sleep
                            Thread.sleep(30000);

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

                    final DatagramSocket checkSocket = new DatagramSocket(9004);
                    byte[] mastercheck = new byte[2048];
                    while (true) {
                        final DatagramPacket chkrpkt = new DatagramPacket(mastercheck, mastercheck.length);
                        checkSocket.receive(chkrpkt);
                        // TODO send notification
                        if (chkrpkt.getData().toString() == "master") {
                            int port = chkrpkt.getPort();
                            InetAddress srcip = chkrpkt.getAddress();
                            final DatagramPacket chkspkt = new DatagramPacket("yes".getBytes(), "yes".getBytes().length, srcip, port);
                            checkSocket.send(chkspkt);
                        }
                    }

                } catch (Exception e) {

                }
            }
        }).start();
    }
}


