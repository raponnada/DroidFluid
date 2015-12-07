package edu.umkc.ofcontroller;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;


import android.os.Bundle;
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

    private Handler logUpdaterHandler;
	final private int logUpdaterInterval = 1000;

    private Process process;
    private Process processArp;
    private BufferedReader bufferedReader;


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

        try {

            process = Runtime.getRuntime().exec(new String[]{"su", "-c", getFilesDir().getParent() + "/lib/libofswitch.so"});
            processArp = Runtime.getRuntime().exec(new String[]{"su", "-c", getFilesDir().getParent() + "/lib/libarpscan.so"});

            Thread t = new Thread(new Controller());
            t.start();
            //Controller UI
            controllerTextView = (TextView)findViewById(R.id.controllerView);
            final ScrollView controllerSV = (ScrollView)findViewById(R.id.scrollView);


            //switch UI
            bufferedReader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            switchTextView = (TextView)findViewById(R.id.switchView);
            final ScrollView sv = (ScrollView)findViewById(R.id.scrollView2);


            logUpdaterHandler = new Handler();
            logUpdater.run();


        } catch (IOException e) {
            e.printStackTrace();
        }
    }

	void updateSwitchLog()
    {
        try {
			final StringBuilder log = new StringBuilder();

            while (bufferedReader.ready()) {
                final String line = bufferedReader.readLine();

                log.append(line + "\n");
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
            Process process = Runtime.getRuntime().exec(
                    "logcat -d OFCONTROLLER:V *:S");
            BufferedReader bufferedReaderCntrl = new BufferedReader(
                    new InputStreamReader(process.getInputStream()));

            StringBuilder log = new StringBuilder();
            String line;
            while ((line = bufferedReaderCntrl.readLine()) != null) {
                if (line.contains(pid)) {
                    log.append(line.split(": ")[1] + "\n");
                }
            }

            controllerTextView.setText(log.toString());
        } catch (IOException e) {
        }
    }
}
