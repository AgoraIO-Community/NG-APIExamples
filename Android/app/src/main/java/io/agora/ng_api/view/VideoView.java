package io.agora.ng_api.view;

import android.animation.Animator;
import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.SurfaceView;
import android.view.View;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.ProgressBar;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import com.google.android.material.slider.LabelFormatter;
import com.google.android.material.slider.Slider;
import io.agora.ng_api.R;
import io.agora.ng_api.util.ExampleUtil;

public class VideoView extends FrameLayout {
    private final Handler mHandler = new Handler(Looper.getMainLooper());
    public final SurfaceView mSurfaceView = new SurfaceView(getContext());
    public final CheckBox mPlayBtn = new CheckBox(getContext());
    public final ProgressBar mLoadingView = new ProgressBar(getContext());
    public final Slider mProgressSlider = new Slider(getContext());

    public Runnable hideOverlayRunnable = () -> {
        mPlayBtn.animate().alpha(0).setDuration(500).setListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                mPlayBtn.setVisibility(GONE);
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        }).start();
        mProgressSlider.animate().alpha(0).setDuration(500).setListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                mProgressSlider.setVisibility(GONE);
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        }).start();
    };
    public Runnable showOverlayRunnable = () -> {
        mPlayBtn.animate().alpha(1).setDuration(500).setListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
                mPlayBtn.setVisibility(VISIBLE);
            }

            @Override
            public void onAnimationEnd(Animator animation) {
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        }).start();
        mProgressSlider.animate().alpha(1).setDuration(500).setListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
                mProgressSlider.setVisibility(VISIBLE);
            }

            @Override
            public void onAnimationEnd(Animator animation) {
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        }).start();
    };

    public VideoView(@NonNull Context context) {
        this(context, null);
    }

    public VideoView(@NonNull Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public VideoView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public VideoView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        LayoutParams lp4SurfaceView = new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        lp4SurfaceView.gravity = Gravity.CENTER_VERTICAL;
        mSurfaceView.setLayoutParams(lp4SurfaceView);

        LayoutParams lp4Slider = new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        lp4Slider.gravity = Gravity.BOTTOM;
        lp4Slider.bottomMargin = (int) ExampleUtil.dp2px(120);
        mProgressSlider.setLayoutParams(lp4Slider);
        mProgressSlider.setLabelBehavior(LabelFormatter.LABEL_GONE);
        mProgressSlider.setVisibility(GONE);

        LayoutParams lp4LoadingView = new FrameLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lp4LoadingView.gravity = Gravity.CENTER;
        mLoadingView.setLayoutParams(lp4LoadingView);
        mLoadingView.setIndeterminate(true);


        LayoutParams lp4CheckBox = new FrameLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lp4CheckBox.gravity = Gravity.CENTER;
        mPlayBtn.setLayoutParams(lp4CheckBox);
        mPlayBtn.setChecked(true);
        mPlayBtn.setButtonDrawable(R.drawable.selector_player_checkable);
        mPlayBtn.setVisibility(GONE);
        mPlayBtn.setAlpha(0f);

        this.addView(mSurfaceView);
        this.addView(mLoadingView);
        this.addView(mPlayBtn);
        this.addView(mProgressSlider);

        this.setOnClickListener(v -> {
            if (mPlayBtn.getVisibility() == VISIBLE)
                hideOverlay();
            else showOverlay();
        });
    }

    public void hideOverlay(){
        mHandler.removeCallbacksAndMessages(null);
        mHandler.post(hideOverlayRunnable);
    }

    public void showOverlay() {
        mHandler.removeCallbacksAndMessages(null);
        mHandler.post(showOverlayRunnable);
        mHandler.postDelayed(hideOverlayRunnable, 2000);
    }


}
