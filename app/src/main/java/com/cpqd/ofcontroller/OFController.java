package com.cpqd.ofcontroller;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import android.os.Bundle;
import android.os.Handler;
import android.app.Activity;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.ViewGroup.LayoutParams;
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
    private Handler logUpdaterHandler;
	final private int logUpdaterInterval = 1000;

    private Process process;
    private BufferedReader bufferedReader;

    private Runnable logUpdater = new Runnable() {
		@Override
		public void run() {
			updateSwitchLog();
			logUpdaterHandler.postDelayed(logUpdater, logUpdaterInterval);
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState)
    {
		super.onCreate(savedInstanceState);
        try {
            process = Runtime.getRuntime().exec(new String[]{"su", "-c", getFilesDir().getParent() + "/lib/libofswitch.so"});

            bufferedReader = new BufferedReader(new InputStreamReader(process.getInputStream()));

            switchTextView = new TextView(this);
            final ScrollView sv = new ScrollView(this);
            sv.addView(switchTextView);
            sv.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

            switchTextView.setMovementMethod(new ScrollingMovementMethod());
            setContentView(sv);

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
}
