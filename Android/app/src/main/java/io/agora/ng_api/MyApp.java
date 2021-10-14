package io.agora.ng_api;

import android.app.Application;
import android.content.Context;
import android.widget.Toast;
import androidx.annotation.StringRes;

public class MyApp extends Application {
    private static MyApp instance;
    public static boolean justDebugUIPart = false;

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        instance = this;
    }

    public void shortToast(@StringRes int stringResId){
        Toast.makeText(instance, stringResId, Toast.LENGTH_SHORT).show();
    }
    public void shortToast(String msg){
        Toast.makeText(instance, msg, Toast.LENGTH_SHORT).show();
    }

    public static MyApp getInstance() {
        return instance;
    }
}
