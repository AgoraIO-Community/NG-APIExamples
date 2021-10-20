package io.agora.ng_api.ui.fragment;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;

import java.util.List;

import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentBasicVideoBinding;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.ng_api.view.DynamicView;
import io.agora.ng_api.view.ScrollableLinearLayout;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.media.video.AgoraRteVideoCanvas;
import io.agora.rte.media.video.AgoraRteVideoSubscribeOptions;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;

/**
 * This demo demonstrates how to make a one-to-one video call version 2
 */
public class BasicVideoFragment extends BaseDemoFragment<FragmentBasicVideoBinding> {

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        initView();
        initListener();

        if (!MyApp.justDebugUIPart) {
            initAgoraRteSDK();
            joinScene();
        }
    }

    private void initView() {
        // control the layout style
        mBinding.toggleGroupFgVideo.addOnButtonCheckedListener((group, checkedId, isChecked) -> {
            if (!isChecked) return;
            if (checkedId == R.id.toggle_btn_flex_fg_video) {
                mBinding.containerBasicVideo.setLayoutStyle(DynamicView.STYLE_FLEX);
            } else {
                mBinding.containerBasicVideo.setLayoutStyle(DynamicView.STYLE_SCROLL);
            }
        });
        mBinding.toggleGroupFgVideo.check(R.id.toggle_btn_flex_fg_video);
    }

    private void initListener() {
        mAgoraHandler = new AgoraRteSceneEventHandler() {

            public void onConnectionStateChanged(AgoraRteSceneConnState state, AgoraRteSceneConnState state1, io.agora.rte.scene.AgoraRteConnectionChangedReason reason) {
                ExampleUtil.utilLog("onConnectionStateChanged: " + state.getValue() + ", " + state1.getValue() + ",reason: " + reason.getValue());
                // 连接建立完成
                if (state1 == AgoraRteSceneConnState.CONN_STATE_CONNECTED && mLocalAudioTrack == null) {
                    // RTC stream prepare
                    AgoraRtcStreamOptions option = new AgoraRtcStreamOptions();
                    mScene.createOrUpdateRTCStream(mLocalUserId, option);
                    // 准备视频采集
                    mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
                    // 必须先添加setPreviewCanvas，然后才能 startCapture
                    addUserPreview(null);
                    if (mLocalVideoTrack != null) {
                        mLocalVideoTrack.startCapture(null);
                    }
                    mScene.publishLocalVideoTrack(mLocalUserId, mLocalVideoTrack);
                    // 准备音频采集
                    mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
                    mLocalAudioTrack.startRecording();
                    mScene.publishLocalAudioTrack(mLocalUserId, mLocalAudioTrack);
                } else if (state1 == AgoraRteSceneConnState.CONN_STATE_DISCONNECTED) {
                    ExampleUtil.utilLog("onConnectionStateChanged: CONN_STATE_DISCONNECTED");
                }

            }

            @Override
            public void onRemoteStreamAdded(List<AgoraRteMediaStreamInfo> list) {
                if (mBinding == null) return;
                for (AgoraRteMediaStreamInfo info : list) {
                    addUserPreview(info.getStreamId());
                }
            }

            @Override
            public void onRemoteStreamRemoved(List<AgoraRteMediaStreamInfo> list) {
                if (mBinding == null) return;
                for (AgoraRteMediaStreamInfo info : list) {
                    mBinding.containerBasicVideo.dynamicRemoveViewWithTag(info.getStreamId());

                }
            }
        };
    }

    private void joinScene() {
        doJoinScene(sceneName, mLocalStreamId, "");
    }

    /**
     * @param remoteStreamIdOrNull null => localView, else => remoteView
     */
    private void addUserPreview(@Nullable String remoteStreamIdOrNull) {
        // Create CardView which contains a TextureView
        CardView cardView = ScrollableLinearLayout.getChildVideoCardView(requireContext(), remoteStreamIdOrNull);

        // Add CardView
        mBinding.containerBasicVideo.demoAddView(cardView);
        // Create AgoraRteVideoCanvas
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(cardView.getChildAt(0));

        // Enhanced
//        cardView.setOnLongClickListener(v -> {
//            int desiredMode = AgoraRteVideoCanvas.RENDER_MODE_FIT;
//            if(canvas.renderMode == desiredMode) desiredMode = AgoraRteVideoCanvas.RENDER_MODE_HIDDEN ;
//            canvas.renderMode = desiredMode;
//            return true;
//        });

        // Set RenderMode
        canvas.renderMode = AgoraRteVideoCanvas.RENDER_MODE_HIDDEN;

        // Remote related stuff
        if(remoteStreamIdOrNull != null){
            mScene.setRemoteVideoCanvas(remoteStreamIdOrNull, canvas);
            mScene.subscribeRemoteVideo(remoteStreamIdOrNull, new AgoraRteVideoSubscribeOptions());
            mScene.subscribeRemoteAudio(remoteStreamIdOrNull);
        }else if(mLocalVideoTrack != null){
            // Local preview
            mLocalVideoTrack.setPreviewCanvas(canvas);
        }
    }

    @Override
    public void doChangeView() {

    }
}
