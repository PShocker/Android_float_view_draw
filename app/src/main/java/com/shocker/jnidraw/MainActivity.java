package com.shocker.jnidraw;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.PixelFormat;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

import com.shocker.jnidraw.databinding.ActivityMainBinding;

import java.io.OutputStream;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'jnidraw' library on application startup.
    static {
        System.loadLibrary("jnidraw");
    }


    public int getLayoutType() {
        int LAYOUT_FLAG;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_PHONE;
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_PHONE;
        } else {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_SYSTEM_ALERT;
        }
        return LAYOUT_FLAG;
    }

    //申请悬浮窗权限
    void getFloatPermission(){
        if (android.os.Build.VERSION.SDK_INT >= 23 && !Settings.canDrawOverlays(this)) {   //Android M Or Over
            Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName()));
            startActivityForResult(intent, 123);
        }
    }


    //创建root进程,用udp通信
    public void StartSuProcess() throws Exception{
        Process p = Runtime.getRuntime().exec("su");
        OutputStream os = p.getOutputStream();
        p.getErrorStream();
        String str =getBaseContext().getApplicationInfo().nativeLibraryDir+"/libjni_draw.so";
        os.write((str + "\n").getBytes());
        os.write("exit\n".getBytes());
        os.flush();
        os.close();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getFloatPermission();
        try {
            StartSuProcess();
        } catch (Exception e) {
            e.printStackTrace();
        }
        DisplayMetrics outMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(outMetrics);
        int widthPixels = outMetrics.widthPixels;
        int heightPixels = outMetrics.heightPixels;

        WindowManager windowManager = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
        OverlayView overlayView = new OverlayView(this,widthPixels,heightPixels);
        final WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                getLayoutType(),
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                PixelFormat.TRANSLUCENT);
        if (Build.VERSION.SDK_INT >= 28) {
            params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES ;
        }
        windowManager.addView(overlayView, params);
    }

}