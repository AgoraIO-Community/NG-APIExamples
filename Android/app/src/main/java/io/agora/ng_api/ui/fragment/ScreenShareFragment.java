package io.agora.ng_api.ui.fragment;

import android.os.Bundle;
import android.view.TextureView;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.List;
import java.util.Random;

import io.agora.ng_api.MyApp;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentScreenShareBinding;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.media.track.AgoraRteScreenVideoTrack;
import io.agora.rte.media.video.AgoraRteVideoCanvas;
import io.agora.rte.media.video.AgoraRteVideoEncoderConfiguration;
import io.agora.rte.media.video.AgoraRteVideoSubscribeOptions;
import io.agora.rte.scene.AgoraRteConnectionChangedReason;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;

public class ScreenShareFragment extends BaseDemoFragment<FragmentScreenShareBinding> {
    private final String localMediaStreamId = "media-"+ new Random().nextInt(1024)+1024;
    private AgoraRteScreenVideoTrack screenVideoTrack;
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

    private void initView() {
        mBinding.btnOpenFgScreenShare.setOnClickListener(v -> createScreenVideoTrack());
    }


    private void initListener() {
        mAgoraHandler = new AgoraRteSceneEventHandler() {
            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState oldState, AgoraRteSceneConnState newState, AgoraRteConnectionChangedReason reason) {
                super.onConnectionStateChanged(oldState, newState, reason);
                if(newState == AgoraRteSceneConnState.CONN_STATE_CONNECTED){
                    mBinding.btnOpenFgScreenShare.setEnabled(true);
                }
            }

            @Override
            public void onRemoteStreamAdded(List<AgoraRteMediaStreamInfo> streams) {
                for (AgoraRteMediaStreamInfo stream : streams) {
                    addRemoteView(stream.getStreamId());
                }
            }

            @Override
            public void onRemoteStreamRemoved(List<AgoraRteMediaStreamInfo> streams) {
                for (AgoraRteMediaStreamInfo stream : streams) {
                    mBinding.containerFgScreenShare.dynamicRemoveViewWithTag(stream.getStreamId());
                }
            }
        };
    }

    /**
     * Total 4 Steps
     * 1: createScreenVideoTrack
     * 2: setPreviewCanvas
     * 3: startCaptureScreen
     * 4: publishLocalVideoTrack
     */
    private void createScreenVideoTrack(){
        if(screenVideoTrack == null){
            TextureView textureView = mBinding.containerFgScreenShare.createDemoLayout(TextureView.class);
            mBinding.containerFgScreenShare.demoAddView(textureView);
            mScene.createOrUpdateRTCStream(localMediaStreamId, new AgoraRtcStreamOptions());
            screenVideoTrack = AgoraRteSDK.getRteMediaFactory().createScreenVideoTrack();
            screenVideoTrack.setPreviewCanvas(new AgoraRteVideoCanvas(textureView));
            screenVideoTrack.startCaptureScreen(requireContext()
                    , new AgoraRteVideoEncoderConfiguration.VideoDimensions());
            mScene.publishLocalVideoTrack(localMediaStreamId, screenVideoTrack);
        }
    }

    private void addRemoteView(String streamId){
        TextureView view = mBinding.containerFgScreenShare.createDemoLayout(TextureView.class);
        view.setTag(streamId);
        mBinding.containerFgScreenShare.demoAddView(view);
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(view);
        mScene.setRemoteVideoCanvas(streamId, canvas);

        mScene.subscribeRemoteVideo(streamId, new AgoraRteVideoSubscribeOptions());
        mScene.subscribeRemoteAudio(streamId);
    }

    private void joinChannel(){
        doJoinChannel(channelName, String.valueOf(new Random().nextInt(1024)), "");
    }

    @Override
    public void doChangeView() {

    }
}
