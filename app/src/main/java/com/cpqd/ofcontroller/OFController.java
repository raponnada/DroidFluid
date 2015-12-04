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

public class OFController extends Activity
{
	private TextView tv;
    private Handler logUpdaterHandler;
	final private int logUpdaterInterval = 1000;

    private Process process;
    private BufferedReader bufferedReader;

    private Runnable logUpdater = new Runnable() {
		@Override
		public void run() {
			updateLog();
			logUpdaterHandler.postDelayed(logUpdater, logUpdaterInterval);
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState)
    {
		super.onCreate(savedInstanceState);
        try {
            process = Runtime.getRuntime().exec(new String[]{"su", "-c", getFilesDir().getParent() + "/lib/ofswitch"});

            bufferedReader = new BufferedReader(new InputStreamReader(process.getInputStream()));

            tv = new TextView(this);
            ScrollView sv = new ScrollView(this);
            sv.addView(tv);
            sv.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

            tv.setMovementMethod(new ScrollingMovementMethod());
            setContentView(sv);

            logUpdaterHandler = new Handler();
            logUpdater.run();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

	void updateLog()
    {
        try {
			final StringBuilder log = new StringBuilder();

            while (bufferedReader.ready()) {
                final String line = bufferedReader.readLine();

                log.append(line + "\n");
            }

            if (!log.toString().isEmpty()) {
                Log.i("DroidFluid", "Native message: " + log.toString());

                tv.setText(log.toString());
            }
		} catch (IOException e) {
            tv.setText(e.getMessage());
		}
    }
}
