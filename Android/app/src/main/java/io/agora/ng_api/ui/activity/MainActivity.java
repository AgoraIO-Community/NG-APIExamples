package io.agora.ng_api.ui.activity;

import android.content.res.ColorStateList;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.graphics.ColorUtils;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.widget.ImageViewCompat;
import androidx.fragment.app.Fragment;
import androidx.navigation.NavArgument;
import androidx.navigation.NavController;
import androidx.navigation.NavDestination;
import androidx.navigation.Navigation;
import androidx.transition.Transition;
import androidx.transition.TransitionManager;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.transition.MaterialContainerTransform;

import java.util.ArrayList;
import java.util.List;

import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseActivity;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.bean.DemoInfo;
import io.agora.ng_api.databinding.ActivityMainBinding;
import io.agora.ng_api.ui.fragment.DescriptionFragment;
import io.agora.ng_api.util.ExampleUtil;

public final class MainActivity extends BaseActivity {
    private final List<DemoInfo> demoInfoList = new ArrayList<>();
    private ActivityMainBinding mBinding;
    private NavController.OnDestinationChangedListener onDestinationChangedListener;

    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBinding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(mBinding.getRoot());
        initView();
        initControlView();

        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);

        // Basically this is equivalent to last line, but I want to change something
        // like change scrimMain's size and show FAB shadow when hover, works perfect
        ViewCompat.setOnApplyWindowInsetsListener(mBinding.getRoot(), (v, insets) -> {
            Insets inset = insets.getInsets(WindowInsetsCompat.Type.systemBars());

            int desiredBottom = Math.max(insets.getInsets(WindowInsetsCompat.Type.ime()).bottom,inset.bottom);

            v.setPadding(inset.left,inset.top,inset.right,desiredBottom);
            // FIXME find a better way to achieve this.
            ((ConstraintLayout.LayoutParams)mBinding.scrimMain.getLayoutParams()).setMargins(-inset.left,-inset.top - 1,-inset.right,-desiredBottom);
            return WindowInsetsCompat.CONSUMED;
        });
        // Prevent the first run lagging
//        new Handler(Looper.getMainLooper()).postDelayed(() -> mBinding.scrimMain.performClick(),200);
    }

    /**
     * OnDestinationChangedListener setup
     * Put this code block inside onCreate cause NPE
     */
    @Override
    protected void onStart() {
        super.onStart();
        getNavController().removeOnDestinationChangedListener(onDestinationChangedListener);
        getNavController().addOnDestinationChangedListener(onDestinationChangedListener);
    }

    private void initView() {
        // FAB and CardView animation
        MaterialContainerTransform transformExpand = ExampleUtil.getTransform(mBinding.tFabMain, mBinding.sectionAudioMain.getRoot());
        transformExpand.addListener(new Transition.TransitionListener() {
            @Override
            public void onTransitionStart(@NonNull Transition transition) {
                mBinding.scrimMain.setEnabled(false);
                mBinding.scrimMain.setVisibility(View.VISIBLE);
                mBinding.scrimMain.animate().alpha(1).start();
            }

            @Override
            public void onTransitionEnd(@NonNull Transition transition) {
                mBinding.scrimMain.setEnabled(true);
            }

            @Override
            public void onTransitionCancel(@NonNull Transition transition) {

            }

            @Override
            public void onTransitionPause(@NonNull Transition transition) {

            }

            @Override
            public void onTransitionResume(@NonNull Transition transition) {

            }
        });
        MaterialContainerTransform transformClose = ExampleUtil.getTransform(mBinding.sectionAudioMain.getRoot(), mBinding.tFabMain);
        transformClose.addListener(new Transition.TransitionListener() {
            @Override
            public void onTransitionStart(@NonNull Transition transition) {
                mBinding.scrimMain.setEnabled(false);
                mBinding.tFabMain.setEnabled(false);
                mBinding.scrimMain.animate().alpha(0).start();
            }

            @Override
            public void onTransitionEnd(@NonNull Transition transition) {
                mBinding.scrimMain.setEnabled(true);
                mBinding.tFabMain.setEnabled(true);
                mBinding.scrimMain.setVisibility(View.GONE);
            }

            @Override
            public void onTransitionCancel(@NonNull Transition transition) {

            }

            @Override
            public void onTransitionPause(@NonNull Transition transition) {

            }

            @Override
            public void onTransitionResume(@NonNull Transition transition) {

            }
        });

        mBinding.tFabMain.setOnClickListener(v -> {
            TransitionManager.beginDelayedTransition(mBinding.getRoot(), transformExpand);
            mBinding.tFabMain.setVisibility(View.GONE);
            mBinding.sectionAudioMain.getRoot().setVisibility(View.VISIBLE);
        });
        mBinding.scrimMain.setOnClickListener(v -> {
            TransitionManager.beginDelayedTransition(mBinding.getRoot(), transformClose);
            mBinding.tFabMain.setVisibility(View.VISIBLE);
            mBinding.sectionAudioMain.getRoot().setVisibility(View.GONE);
        });

        // For avoiding overlap with Back Icon
        mBinding.textTitleMain.setSelected(true);
        // Nav back listener
        mBinding.navIconMain.setOnClickListener((v) -> getNavController().popBackStack());

        // Destination change listener
        onDestinationChangedListener = (controller, destination, arguments) -> {
            String title = "";
            // 标题改变
            if (destination.getId() == R.id.descriptionFragment) {
                if (arguments != null) {
                    DemoInfo demoInfo = (DemoInfo) arguments.getSerializable(DemoInfo.key);
                    title = demoInfo.getTitle();
                }
                mBinding.tFabMain.hide();
            } else {
                if (destination.getLabel() != null)
                    title = destination.getLabel().toString();


                if(destination.getId() != R.id.listFragment) {
                    if(arguments != null) {
                        String sceneName = arguments.getString(DescriptionFragment.sceneName);
                        if(sceneName!=null)
                            title += getString(R.string.show_scene_name,sceneName);
                    }
                }
                mBinding.tFabMain.show();
            }
            mBinding.textTitleMain.setText(title);

            // 图标改变
            if (destination.getId() == R.id.listFragment) {
                mBinding.navIconMain.setVisibility(View.INVISIBLE);
            } else {
                mBinding.navIconMain.setVisibility(View.VISIBLE);
            }
        };

    }

    private void initControlView() {

        // "静音"按钮 点击事件
        mBinding.sectionAudioMain.muteBtnSectionAudio.addOnCheckedChangeListener((v, isChecked) -> {
            if (!v.isPressed()) return;
            BaseDemoFragment<?> f = checkDemoAvailable();
            if (f != null && f.mScene != null && f.mLocalAudioTrack != null) {
                if (isChecked) f.mScene.unpublishLocalAudioTrack(f.mLocalAudioTrack);
                else f.mScene.publishLocalAudioTrack(f.mLocalStreamId, f.mLocalAudioTrack);
            }
        });

        // adjust publish audio volume
        // 调整发布音频的音量
        mBinding.sectionAudioMain.volumeDownBtnSectionAudio.setOnClickListener(v -> {
            float currentVolume = mBinding.sectionAudioMain.sliderVolumeSectionAudio.getValue();
            float desireValue = Math.max(0f, currentVolume - 1);
            mBinding.sectionAudioMain.sliderVolumeSectionAudio.setValue(desireValue);
        });
        mBinding.sectionAudioMain.volumeUpBtnSectionAudio.setOnClickListener(v -> {
            float currentVolume = mBinding.sectionAudioMain.sliderVolumeSectionAudio.getValue();
            float desireValue = Math.min(100f, currentVolume + 1);
            mBinding.sectionAudioMain.sliderVolumeSectionAudio.setValue(desireValue);
        });
        mBinding.sectionAudioMain.sliderVolumeSectionAudio.addOnChangeListener((slider, value, fromUser) -> {
            BaseDemoFragment<?> f = checkDemoAvailable();
            if (f != null && f.mLocalAudioTrack != null) f.mLocalAudioTrack.adjustPublishVolume((int) value);
        });
    }

    public NavController getNavController() {
        return Navigation.findNavController(mBinding.containerNpApi);
    }

    public void configData() {
        if (!demoInfoList.isEmpty()) return;

//        for (int i = 0; i < 20; i++) {
//            DemoInfo demoInfo = new DemoInfo(i, i / 10, "Title" + i, 1, DemoInfo.STRING_ADVANCED);
//            demoInfoList.add(demoInfo);
//        }
//        if (!demoInfoList.isEmpty()) return;

        NavDestination destination;
        DemoInfo demoInfo;

        String argsType = getString(R.string.args_nav_type);
        String argsDesc = getString(R.string.args_nav_desc);
        String argsPerm = getString(R.string.args_nav_permission);

        for (NavDestination navDestination : getNavController().getGraph()) {
            destination = navDestination;

            if (destination.getArguments().isEmpty()) continue;

            // handle type( must have)
            // 0 , 1
            // {@link DemoInfo.TYPE_BASIC},{@link DemoInfo.TYPE_ADVANCED}
            NavArgument typeArgument = destination.getArguments().get(argsType);
            // No Type, not a demo.
            if (typeArgument == null) continue;

            Integer type;
            type = (Integer) typeArgument.getDefaultValue();
            if (null == type || type < DemoInfo.TYPE_BASIC || type > DemoInfo.TYPE_ADVANCED)
                throw new IllegalArgumentException("type value out of range.");

            // title
            assert destination.getLabel() != null;
            String title = destination.getLabel().toString();

            // destination id
            Integer descInt;
            NavArgument descArgument = destination.getArguments().get(argsDesc);
            if (descArgument == null)
                throw new IllegalArgumentException("fragment argument " + argsDesc + " required.");
            descInt = (Integer) descArgument.getDefaultValue();
            if (null == descInt || descInt <= 0)
                throw new IllegalArgumentException("fragment argument " + argsDesc + " illegal.");

            // permission
            // 0x01 音频
            // 0x02 视频
            Integer permissionFlag;
            NavArgument permissionArgument = destination.getArguments().get(argsPerm);
            if (permissionArgument == null)
                throw new IllegalArgumentException("fragment argument " + argsPerm + " required.");
            else permissionFlag = (Integer) permissionArgument.getDefaultValue();
            if (null == permissionFlag || permissionFlag < 0)
                throw new IllegalArgumentException("fragment argument " + argsPerm + " illegal.");

            // create new DemoInfo
            demoInfo = new DemoInfo(destination.getId(), type, title, permissionFlag, descInt);
            demoInfoList.add(demoInfo);
        }
    }

    public List<DemoInfo> getDemoInfoList() {
        return demoInfoList;
    }

    private @Nullable BaseDemoFragment<?> checkDemoAvailable() {
        Fragment fragment = getSupportFragmentManager().getPrimaryNavigationFragment();
        if (fragment != null)
            fragment = fragment.getChildFragmentManager().getPrimaryNavigationFragment();
        if (fragment instanceof BaseDemoFragment) return (BaseDemoFragment<?>) fragment;
        else return null;
    }

    /**
     * 当 FAB 是展开的《==》返回键收起
     * Block back this time when FAB is not showing or animation is in progress.
     */
    @Override
    public void onBackPressed() {
        if (mBinding.sectionAudioMain.getRoot().getVisibility() == View.VISIBLE) {
            // 动画结束，调用点击事件；动画过程中，直接忽略这次操作。
            if (mBinding.scrimMain.isEnabled())
                mBinding.scrimMain.performClick();
        } else
            super.onBackPressed();
    }

    @Override
    public void onUIModeChanges() {
        int colorBgd = ExampleUtil.getColorInt(this, android.R.attr.colorBackground);
        int colorScrim = ExampleUtil.getColorInt(this, R.attr.scrimBackground);
        int colorPrimary = ExampleUtil.getColorInt(this, R.attr.colorPrimary);
        int colorOnPrimary = ExampleUtil.getColorInt(this, R.attr.colorOnPrimary);
        int colorSurface = ExampleUtil.getColorInt(this, R.attr.colorSurface);
        int colorOnSurface = ExampleUtil.getColorInt(this, R.attr.colorOnSurface);

        int colorTextPrimary = ExampleUtil.getColorInt(this, android.R.attr.textColorPrimary);
        int colorTextSecondary = ExampleUtil.getColorInt(this, android.R.attr.textColorSecondary);
        // Default disabledTextColor is colorOnSurface with alpha*0.38
        int colorTextDisabled = ColorUtils.setAlphaComponent(colorOnSurface, (int) (255 * 0.38));
        // default StrokeColor is colorOnSurface with alpha*0.12
        int colorStroke = ColorUtils.setAlphaComponent(colorOnSurface, (int) (255 * 0.12));

        // Config Window color
        getWindow().getDecorView().setBackgroundColor(colorBgd);

        // Config main views' color
        ImageViewCompat.setImageTintList(mBinding.navIconMain, ColorStateList.valueOf(colorTextPrimary));
        mBinding.textTitleMain.setTextColor(colorTextPrimary);
        mBinding.scrimMain.setBackgroundColor(colorScrim);
        mBinding.tFabMain.setSupportImageTintList(ColorStateList.valueOf(colorOnPrimary));

        // Config ControlView's color
        ((CardView)mBinding.sectionAudioMain.getRoot()).setCardBackgroundColor(colorSurface);
        ColorStateList primaryStateList = new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked, android.R.attr.state_enabled}, new int[]{android.R.attr.state_enabled}, new int[]{}}, new int[]{colorPrimary, colorTextSecondary, colorTextDisabled});
        ColorStateList surfaceStateList = new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked}, new int[]{}}, new int[]{colorPrimary, colorStroke});

        ExampleUtil.updateMaterialButtonTint(mBinding.sectionAudioMain.muteBtnSectionAudio, primaryStateList, surfaceStateList);

        for (int i = 0; i < mBinding.sectionAudioMain.voiceSwitchGroupSectionAudio.getChildCount(); i++)
            ExampleUtil.updateMaterialButtonTint((MaterialButton) mBinding.sectionAudioMain.voiceSwitchGroupSectionAudio.getChildAt(i), primaryStateList, surfaceStateList);
    }
}
