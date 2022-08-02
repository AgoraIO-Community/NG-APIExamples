package io.agora.ng_api.util;

import android.Manifest;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Color;
import android.util.Log;
import android.util.TypedValue;
import android.view.HapticFeedbackConstants;
import android.view.View;
import android.view.Window;
import android.view.animation.OvershootInterpolator;

import androidx.annotation.AttrRes;
import androidx.core.content.ContextCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.navigation.NavOptions;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.textfield.TextInputLayout;
import com.google.android.material.transition.MaterialArcMotion;
import com.google.android.material.transition.MaterialContainerTransform;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import io.agora.ng_api.BuildConfig;
import io.agora.ng_api.R;

public class ExampleUtil {
    public static String APPID = "APPID";


    public static SharedPreferences getSp(Context context) {
        return context.getSharedPreferences("sp_agora", Context.MODE_PRIVATE);
    }

    public static final NavOptions defaultNavOptions = new NavOptions.Builder().setEnterAnim(R.anim.slide_in_right)
            .setExitAnim(R.anim.slide_out_left)
            .setPopEnterAnim(R.anim.slide_in_left)
            .setPopExitAnim(R.anim.slide_out_right)
            .build();


    private static final String[] permissions = new String[]{
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.CAMERA
    };

    public static MaterialContainerTransform getTransform(View startView, View endView) {
        MaterialContainerTransform transform = new MaterialContainerTransform();
        transform.setStartView(startView);
        transform.setEndView(endView);
        transform.addTarget(endView);
        transform.setPathMotion(new MaterialArcMotion());
        transform.setDuration(500);
        transform.setScrimColor(Color.TRANSPARENT);
        return transform;
    }

    public static String[] getPermissions(int flag) {
        // 记录所有下标
        List<Integer> indexes = new ArrayList<>(permissions.length);

        int count = 0;
        int currentIndex = 0;
        while (flag != 0) {
            if ((flag & 1) != 0) {
                indexes.add(currentIndex);
                count++;
            }
            flag = flag >> 1;
            currentIndex++;
        }
        if (count == 0) return new String[]{};
        else {
            String[] res = new String[count];
            for (int i = 0; i < count; i++) {
                res[i] = permissions[indexes.get(i)];
            }
            return res;
        }
    }

    public static void hideKeyboard(Window window, View view) {
        WindowInsetsControllerCompat con = WindowCompat.getInsetsController(window, view);
        if (con != null) con.hide(WindowInsetsCompat.Type.ime());
        view.clearFocus();
    }

    public static void showKeyboard(Window window, View view) {
        WindowInsetsControllerCompat con = WindowCompat.getInsetsController(window, view);
        if (con != null) con.show(WindowInsetsCompat.Type.ime());
    }

    /**
     * android.R.attr.actionBarSize
     */
    public static int getAttrResId(Context context, @AttrRes int resId) {
        TypedValue tv = new TypedValue();
        context.getTheme().resolveAttribute(resId, tv, true);
        return tv.resourceId;
    }

    public static int getColorInt(Context context, @AttrRes int resId) {
        TypedValue tv = new TypedValue();
        context.getTheme().resolveAttribute(resId, tv, true);

        if (tv.type == TypedValue.TYPE_STRING) return ContextCompat.getColor(context, tv.resourceId);
        return tv.data;
    }

    public static float dp2px(int dp) {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, Resources.getSystem().getDisplayMetrics());
    }

    public static float sp2px(int sp) {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, sp, Resources.getSystem().getDisplayMetrics());
    }

    public static void utilLog(String msg) {
        if(BuildConfig.DEBUG)
            Log.d("ng-api", msg);
    }

    public static ColorStateList getTint(int tintColor) {
        int[][] state = new int[][]{new int[]{}, new int[]{android.R.attr.state_pressed}};
        int[] color = new int[]{Color.TRANSPARENT, tintColor};
        return new ColorStateList(state, color);
    }


    public static void shakeViewAndVibrateToAlert(TextInputLayout view) {
        view.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
        view.postDelayed(() -> view.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY), 100);
        view.postDelayed(() -> view.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY), 300);

        ObjectAnimator o = ObjectAnimator.ofFloat(view, View.TRANSLATION_X, 0f, 50f, -50f, 0f, 50f, 0f);
        o.setInterpolator(new OvershootInterpolator());
        o.setDuration(500L);
        o.start();
    }

    public static void updateMaterialButtonTint(MaterialButton button, ColorStateList primaryStateList, ColorStateList surfaceStateList) {
        button.setIconTint(primaryStateList);
        button.setTextColor(primaryStateList);
        button.setStrokeColor(surfaceStateList);
    }

    public static boolean isNightMode() {
        return isNightMode(Resources.getSystem().getConfiguration());
    }

    public static boolean isNightMode(Configuration configuration) {
        return (configuration.uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
    }


    public static String durationFormat(Context context, Long duration) {
        return context.getString(
                R.string.duration_format,
                TimeUnit.MILLISECONDS.toHours(duration)%60,
                TimeUnit.MILLISECONDS.toMinutes(duration)%60,
                TimeUnit.MILLISECONDS.toSeconds(duration)%60
        );
    }
}
