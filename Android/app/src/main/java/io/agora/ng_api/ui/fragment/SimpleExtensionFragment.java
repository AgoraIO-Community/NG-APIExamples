package io.agora.ng_api.ui.fragment;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;

import android.os.Bundle;
import android.util.JsonReader;
import android.util.JsonWriter;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.slider.Slider;

import io.agora.extension.ExtensionManager;
import io.agora.ng_api.MyApp;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentSimpleExtensionBinding;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.track.AgoraRteCameraVideoTrack;
import io.agora.rte.media.video.AgoraRteVideoCanvas;
import io.agora.rte.scene.AgoraRteConnectionChangedReason;
import io.agora.rte.scene.AgoraRteExtensionProperty;
import io.agora.rte.scene.AgoraRteSceneConfig;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;

public class SimpleExtensionFragment extends BaseDemoFragment<FragmentSimpleExtensionBinding> {

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        initView();
        initListener();
        if (!MyApp.debugMine) {
            initAgoraRteSDK();
            joinChannel();
        }
    }

    private void initListener() {

        mAgoraHandler = new AgoraRteSceneEventHandler() {
            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState oldState, AgoraRteSceneConnState newState, AgoraRteConnectionChangedReason reason) {

                if(newState == AgoraRteSceneConnState.CONN_STATE_CONNECTED && mLocalAudioTrack == null){
                    // RTC stream prepare
                    mScene.createOrUpdateRTCStream(mLocalUserId, new AgoraRtcStreamOptions());

                    // capture video
                    mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
                    if (mLocalVideoTrack != null) {
                        // 必须先添加setPreviewCanvas，然后才能 startCapture
                        addLocalView(mLocalVideoTrack);
                        int res =mLocalVideoTrack.enableExtension(ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_VIDEO_FILTER_NAME);
                        ExampleUtil.utilLog("enableExtensionVideo:"+res);
                        enableWaterMark(res == 0);
                        mLocalVideoTrack.startCapture(null);
                        mScene.publishLocalVideoTrack(mLocalUserId, mLocalVideoTrack);
                    }
                    // capture audio
                    mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
                    if (mLocalAudioTrack != null) {
                        int res = mLocalAudioTrack.enableExtension(ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_AUDIO_FILTER_NAME);
                        ExampleUtil.utilLog("enableExtensionAudio:"+res);
                        mLocalAudioTrack.startRecording();
                        mScene.publishLocalAudioTrack(mLocalUserId, mLocalAudioTrack);
                    }
                }
            }
        };
    }
    private void addLocalView(@NonNull AgoraRteCameraVideoTrack videoTrack){
        TextureView textureView = new TextureView(requireContext());
        textureView.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
        mBinding.getRoot().addView(textureView,0);

        AgoraRteVideoCanvas videoCanvas = new AgoraRteVideoCanvas(textureView);
        videoCanvas.renderMode = AgoraRteVideoCanvas.RENDER_MODE_FIT;
        videoTrack.setPreviewCanvas(videoCanvas);

    }

    private void initView() {
        mBinding.sliderFgSimpleExtension.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                adjustVolume(value);
            }
        });
    }

    private void joinChannel() {
        AgoraRteSceneConfig config = new AgoraRteSceneConfig();
        config.addExtension(ExtensionManager.EXTENSION_NAME);
        doJoinChannel(channelName, mLocalUserId, "",config);
    }

    private void enableWaterMark(boolean enable){
        setExtensionProperty(ExtensionManager.KEY_ENABLE_WATER_MARK, String.valueOf(enable));
    }

    private void adjustVolume(float desiredVolume){
        setExtensionProperty(ExtensionManager.KEY_ADJUST_VOLUME_CHANGE,String.valueOf(desiredVolume));
    }

    private void setExtensionProperty(String key,String jsonValue){
        mScene.setExtensionProperty(new AgoraRteExtensionProperty(mLocalUserId,ExtensionManager.EXTENSION_VENDOR_NAME,ExtensionManager.EXTENSION_NAME, key, jsonValue));
    }

    @Override
    public void doChangeView() {

    }
}
