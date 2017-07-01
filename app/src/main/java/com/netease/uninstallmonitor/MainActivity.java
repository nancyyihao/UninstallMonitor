package com.netease.uninstallmonitor;

import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText("hello world!");

        String pkgDir = "/data/data/" + getPackageName();

        monitorUsingNotify(pkgDir, Build.VERSION.SDK_INT, "https://www.google.com");
    }

    public native void monitorUsingPoll(String pkgDir, int sdkVersion, String openUrl);

    /**
     * recommend using this way
     *
     * @param pkgDir
     * @param sdkVersion
     * @param openUrl
     */
    public native void monitorUsingNotify(String pkgDir, int sdkVersion, String openUrl);


    static {
        System.loadLibrary("uninstall_feedback");
    }
}
