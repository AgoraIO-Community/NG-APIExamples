package io.agora.ng_api.ui.fragment;

import android.content.res.ColorStateList;
import android.os.Bundle;
import android.view.TextureView;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.ColorUtils;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.List;

import io.agora.extension.ExtensionManager;
import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentSimpleExtensionBinding;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.media.video.AgoraRteVideoCanvas;
import io.agora.rte.media.video.AgoraRteVideoSubscribeOptions;
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

    private void initView() {
        mBinding.sliderFgSimpleExtension.setValue(100);
    }

    private void initListener() {
        mBinding.sliderFgSimpleExtension.addOnChangeListener((slider, value, fromUser) -> adjustVolume(value));
        mBinding.sliderFgSimpleExtension.setLabelFormatter(value -> String.valueOf((int) value));
        mBinding.btnEnableWaterMarkFgSimpleExtension.addOnCheckedChangeListener((button, isChecked) -> enableWaterMark(isChecked));

        mAgoraHandler = new AgoraRteSceneEventHandler() {
            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState oldState, AgoraRteSceneConnState newState, AgoraRteConnectionChangedReason reason) {

                if (newState == AgoraRteSceneConnState.CONN_STATE_CONNECTED && mLocalAudioTrack == null) {
                    // Step 1
                    mScene.createOrUpdateRTCStream(mLocalStreamId, new AgoraRtcStreamOptions());
                    // Step 2
                    initLocalAudioTrackWithExtension();
                    // Step 3
                    initLocalVideoTrackWithExtension();
                }
            }

            @Override
            public void onRemoteStreamAdded(List<AgoraRteMediaStreamInfo> streams) {
                for (AgoraRteMediaStreamInfo info : streams) {
                    addRemoteView(info.getStreamId());
                }
            }

            @Override
            public void onRemoteStreamRemoved(List<AgoraRteMediaStreamInfo> streams) {
                for (AgoraRteMediaStreamInfo info : streams){
                    mBinding.containerFgSimpleExtension.dynamicRemoveViewWithTag(info.getStreamId());
                    mScene.unsubscribeRemoteAudio(info.getStreamId());
                    mScene.unsubscribeRemoteVideo(info.getStreamId());
                }
            }
        };
    }

    private void initLocalAudioTrackWithExtension(){
        mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
        if(mLocalAudioTrack != null) {
            int res = mLocalAudioTrack.enableExtension(ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_AUDIO_FILTER_VOLUME);
            ExampleUtil.utilLog("enableExtensionAudio:" + res);
            mLocalAudioTrack.startRecording();
            mScene.publishLocalAudioTrack(mLocalStreamId, mLocalAudioTrack);
        }
    }

    private void initLocalVideoTrackWithExtension(){
        mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
        // 必须先添加setPreviewCanvas，然后才能 startCapture
        // Must first setPreviewCanvas, then we can startCapture
        addLocalView(mBinding.containerFgSimpleExtension);
        if (mLocalVideoTrack != null) {
            int res = mLocalVideoTrack.enableExtension(ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_VIDEO_FILTER_WATERMARK);
            ExampleUtil.utilLog("enableExtensionVideo:" + res);
            mLocalVideoTrack.startCapture(null);
            mScene.publishLocalVideoTrack(mLocalStreamId, mLocalVideoTrack);
        }
    }

    private void addRemoteView(@NonNull String streamId) {
        TextureView textureView = new TextureView(requireContext());
        textureView.setTag(streamId);

        mBinding.containerFgSimpleExtension.dynamicAddView(textureView);

        AgoraRteVideoCanvas videoCanvas = new AgoraRteVideoCanvas(textureView);
        videoCanvas.renderMode = AgoraRteVideoCanvas.RENDER_MODE_HIDDEN;
        mScene.setRemoteVideoCanvas(streamId, videoCanvas);

        mScene.subscribeRemoteAudio(streamId);
        mScene.subscribeRemoteVideo(streamId, new AgoraRteVideoSubscribeOptions());
    }

    private void joinScene() {
        doJoinScene(sceneName, mLocalUserId, "");
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
            return mLocalAudioTrack.setExtensionProperty(new AgoraRteExtensionProperty(mLocalStreamId, ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_AUDIO_FILTER_VOLUME, ExtensionManager.KEY_ADJUST_VOLUME_CHANGE, jsonValue));
        return -1;
    }

    private int setVideoWaterMarkProperty(String jsonValue) {
        if (mLocalVideoTrack != null)
            return mLocalVideoTrack.setExtensionProperty(new AgoraRteExtensionProperty(mLocalStreamId, ExtensionManager.EXTENSION_VENDOR_NAME, ExtensionManager.EXTENSION_VIDEO_FILTER_WATERMARK, ExtensionManager.KEY_ENABLE_WATER_MARK, jsonValue));
        return -1;
    }

    @Override
    public void doChangeView() {
        int colorPrimary = ExampleUtil.getColorInt(requireContext(), R.attr.colorPrimary);
        int colorOnSurface = ExampleUtil.getColorInt(requireContext(), R.attr.colorOnSurface);
        int colorStroke = ColorUtils.setAlphaComponent(colorOnSurface, (int) (255 * 0.12));


        int colorTextSecondary = ExampleUtil.getColorInt(requireContext(), android.R.attr.textColorSecondary);
        int colorTextDisabled = ColorUtils.setAlphaComponent(colorOnSurface, (int) (255 * 0.38));

        ColorStateList primaryStateList = new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked, android.R.attr.state_enabled}, new int[]{android.R.attr.state_enabled}, new int[]{}}, new int[]{colorPrimary, colorTextSecondary, colorTextDisabled});
        ColorStateList surfaceStateList = new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked}, new int[]{}}, new int[]{colorPrimary, colorStroke});

        ExampleUtil.updateMaterialButtonTint(mBinding.btnEnableWaterMarkFgSimpleExtension, primaryStateList, surfaceStateList);
    }
}
