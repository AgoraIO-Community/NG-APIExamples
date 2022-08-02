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
 * This demo demonstrates how to make a Basic Video Scene
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
                // 连接建立完成 初始化 localAudioTrack、localVideoTrack
                /*
                    1. createOrUpdateRTCStream
                    2. initBasicLocalAudioTrack
                    3. initBasicLocalVideoTrack
                 */
                if (state1 == AgoraRteSceneConnState.CONN_STATE_CONNECTED && mLocalAudioTrack == null) {
                    // Step 1
                    mScene.createOrUpdateRTCStream(mLocalStreamId, new AgoraRtcStreamOptions());
                    // Step 2
                    initBasicLocalAudioTrack();
                    // Step 3
                    initBasicLocalVideoTrack();
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

    private void initBasicLocalAudioTrack(){
        mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
        if(mLocalAudioTrack != null) {
            mLocalAudioTrack.startRecording();
            mScene.publishLocalAudioTrack(mLocalStreamId, mLocalAudioTrack);
        }
    }

    private void initBasicLocalVideoTrack(){
        mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
        // 必须先添加setPreviewCanvas，然后才能 startCapture
        // Must first setPreviewCanvas, then we can startCapture
        addUserPreview(null);
        if (mLocalVideoTrack != null) {
            mLocalVideoTrack.startCapture(null);
            mScene.publishLocalVideoTrack(mLocalStreamId, mLocalVideoTrack);
        }
    }

    /**
     * 1. Create a view contains a TextView/SurfaceView to preview camera then attach it to window.
     * 2. Init AgoraRteVideoCanvas with this TextView/SurfaceView.
     * 3-1. For local stream, just call {@link io.agora.rte.media.track.AgoraRteCameraVideoTrack#setPreviewCanvas(AgoraRteVideoCanvas)}.
     * 3-2. For remote stream, set it as remote video canvas to scene, then subscribe remote Audio/Video stream.
     *
     * 1. 创建一个包含 TextView/SurfaceView 的视图，用来预览摄像头图像，并且添加到界面上。
     * 2. 使用这个 TextView/SurfaceView 初始化 AgoraRteVideoCanvas。
     * 3-1. 对于本地数据流，我们不需要订阅，所以只用 {@link io.agora.rte.media.track.AgoraRteCameraVideoTrack#setPreviewCanvas(AgoraRteVideoCanvas)} 展示就行。
     * 3-2. 对于远端数据流，我们需要设置 {@link io.agora.rte.scene.AgoraRteScene#setRemoteVideoCanvas(String, AgoraRteVideoCanvas)}
     * 然后订阅远端音频流、视频流。
     *
     * @param remoteStreamIdOrNull null => localView, else => remoteView
     */
    private void addUserPreview(@Nullable String remoteStreamIdOrNull) {

        // Prevent show duplicate remote view.
        if(remoteStreamIdOrNull != null && mBinding.containerBasicVideo.findViewWithTag(remoteStreamIdOrNull) != null)
            return;


        // Step 1

        // Create CardView which contains a TextureView
        CardView cardView = ScrollableLinearLayout.getChildVideoCardView(requireContext(), remoteStreamIdOrNull);
        // Add CardView to existing layout
        mBinding.containerBasicVideo.dynamicAddView(cardView);

        // Step 2

        // Create AgoraRteVideoCanvas
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(cardView.getChildAt(0));

        // Set RenderMode
        canvas.renderMode = AgoraRteVideoCanvas.RENDER_MODE_HIDDEN;

        // Enhanced ( Optional, currently not working)
        // Change the renderMode Dynamically
//        cardView.setOnLongClickListener(v -> {
//            int desiredMode = AgoraRteVideoCanvas.RENDER_MODE_FIT;
//            if(canvas.renderMode == desiredMode) desiredMode = AgoraRteVideoCanvas.RENDER_MODE_HIDDEN ;
//            canvas.renderMode = desiredMode;
//            return true;
//        });

        // Remote related stuff
        if(remoteStreamIdOrNull != null){
            // Step 3-2
            mScene.setRemoteVideoCanvas(remoteStreamIdOrNull, canvas);
            mScene.subscribeRemoteAudio(remoteStreamIdOrNull);
            mScene.subscribeRemoteVideo(remoteStreamIdOrNull, new AgoraRteVideoSubscribeOptions());
        }else if(mLocalVideoTrack != null){
            // Step 3-1

            // Local preview
            mLocalVideoTrack.setPreviewCanvas(canvas);
        }
    }

    private void joinScene() {
        doJoinScene(sceneName, mLocalUserId, "");
    }

    @Override
    public void doChangeView() {

    }
}
