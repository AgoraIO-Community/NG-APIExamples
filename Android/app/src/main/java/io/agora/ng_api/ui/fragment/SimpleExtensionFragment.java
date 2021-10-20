package io.agora.ng_api.ui.fragment;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;

import android.os.Bundle;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.json.JSONException;
import org.json.JSONObject;

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
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;

public class SimpleExtensionFragment extends BaseDemoFragment<FragmentSimpleExtensionBinding> {

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        initView();
        initListener();
        if (!MyApp.justDebugUIPart) {
            initAgoraRteSDK(true);
            joinScene();
        }
    }

    private void initListener() {
        mBinding.sliderFgSimpleExtension.addOnChangeListener((slider, value, fromUser) -> adjustVolume(value));
        mBinding.sliderFgSimpleExtension.setLabelFormatter(value -> String.valueOf((int) value));
        mBinding.btnEnableWaterMarkFgSimpleExtension.addOnCheckedChangeListener((button, isChecked) -> enableWaterMark(isChecked));

        mAgoraHandler = new AgoraRteSceneEventHandler() {
            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState oldState, AgoraRteSceneConnState newState, AgoraRteConnectionChangedReason reason) {

                if (newState == AgoraRteSceneConnState.CONN_STATE_CONNECTED && mLocalAudioTrack == null) {
                    // RTC stream prepare
                    mScene.createOrUpdateRTCStream(mLocalUserId, new AgoraRtcStreamOptions());

                    // capture video
                    mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
                    if (mLocalVideoTrack != null) {
                        // 必须先添加setPreviewCanvas，然后才能 startCapture
                        addLocalView(mLocalVideoTrack);
                        int res = mLocalVideoTrack.enableExtension(ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_VIDEO_FILTER_NAME);
                        ExampleUtil.utilLog("enableExtensionVideo:" + res);
                        mLocalVideoTrack.startCapture(null);
                        mScene.publishLocalVideoTrack(mLocalUserId, mLocalVideoTrack);
                    }
                    // capture audio
                    mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
                    if (mLocalAudioTrack != null) {
                        int res = mLocalAudioTrack.enableExtension(ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_AUDIO_FILTER_NAME);
                        ExampleUtil.utilLog("enableExtensionAudio:" + res);
                        mLocalAudioTrack.startRecording();
                        mScene.publishLocalAudioTrack(mLocalUserId, mLocalAudioTrack);
                    }
                }
            }
        };
    }

    private void addLocalView(@NonNull AgoraRteCameraVideoTrack videoTrack) {
        TextureView textureView = new TextureView(requireContext());
        textureView.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
        mBinding.getRoot().addView(textureView, 0);

        AgoraRteVideoCanvas videoCanvas = new AgoraRteVideoCanvas(textureView);
        videoCanvas.renderMode = AgoraRteVideoCanvas.RENDER_MODE_FIT;
        videoTrack.setPreviewCanvas(videoCanvas);
    }

    private void initView() {
        mBinding.sliderFgSimpleExtension.setValue(100);
    }

    private void joinScene() {
        doJoinScene(sceneName, mLocalStreamId, "");
    }

    private void enableWaterMark(boolean enable) {
        if (mLocalVideoTrack == null) return;
        String jsonValue = null;
        JSONObject o = new JSONObject();
        try {
            o.put(ExtensionManager.ENABLE_WATER_MARK_STRING, "hello world");
            o.put(ExtensionManager.ENABLE_WATER_MARK_FLAG, enable);
            jsonValue = o.toString();
        } catch (JSONException e) {
            e.printStackTrace();
        }
        if (jsonValue != null) {
            int res = setVideoWaterMarkProperty(jsonValue);
            ExampleUtil.utilLog("res enableWaterMark:" + res);
        }
    }

    private void adjustVolume(float desiredVolume) {
        if (mLocalAudioTrack == null) return;
        int res = setAudioVolumeProperty(String.valueOf(desiredVolume));
        ExampleUtil.utilLog("res adjustVolume: " + res);
    }

    private int setAudioVolumeProperty(String jsonValue) {
        if (mLocalAudioTrack != null)
            return mLocalAudioTrack.setExtensionProperty(new AgoraRteExtensionProperty(mLocalUserId, ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_AUDIO_FILTER_NAME, ExtensionManager.KEY_ADJUST_VOLUME_CHANGE, jsonValue));
        return -1;
    }

    private int setVideoWaterMarkProperty(String jsonValue) {
        if (mLocalVideoTrack != null)
            return mLocalVideoTrack.setExtensionProperty(new AgoraRteExtensionProperty(mLocalUserId, ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_VIDEO_FILTER_NAME, ExtensionManager.KEY_ENABLE_WATER_MARK, jsonValue));
        return -1;
    }

    @Override
    public void doChangeView() {

    }
}
