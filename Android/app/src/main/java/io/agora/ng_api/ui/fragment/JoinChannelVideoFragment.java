package io.agora.ng_api.ui.fragment;

import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.TextureView;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.List;

import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentJoinChannelVideoBinding;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.ng_api.view.DynamicView;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.media.video.AgoraRteVideoCanvas;
import io.agora.rte.media.video.AgoraRteVideoSubscribeOptions;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;
import io.agora.rte.user.AgoraRteUserInfo;

/**
 * This demo demonstrates how to make a one-to-one video call version 2
 */
public class JoinChannelVideoFragment extends BaseDemoFragment<FragmentJoinChannelVideoBinding> {

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
        audioManager = (AudioManager) requireContext().getSystemService(Context.AUDIO_SERVICE);
        // control the layout style
        mBinding.toggleGroupFgVideo.addOnButtonCheckedListener((group, checkedId, isChecked) -> {
            if (!isChecked) return;
            if (checkedId == R.id.toggle_btn_flex_fg_video) {
                mBinding.containerJoinChannelVideo.setLayoutStyle(DynamicView.STYLE_FLEX);
            } else {
                mBinding.containerJoinChannelVideo.setLayoutStyle(DynamicView.STYLE_SCROLL);
            }
        });
        mBinding.toggleGroupFgVideo.check(R.id.toggle_btn_flex_fg_video);
    }

    private void initListener() {
        mAgoraHandler = new AgoraRteSceneEventHandler() {

            /**
             * 场景网络状态回调
             * @param state from state
             * @param state1 to state
             * @param reason 状态改变的原因
             */
            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState state, AgoraRteSceneConnState state1, io.agora.rte.scene.AgoraRteConnectionChangedReason reason) {
                ExampleUtil.utilLog("onConnectionStateChanged: " + state.getValue() + ", " + state1.getValue() + ",reason: " + reason.getValue() + "，\nThread:" + Thread.currentThread().getName());
                // 连接建立完成
                if (state1 == AgoraRteSceneConnState.CONN_STATE_CONNECTED && mLocalAudioTrack == null) {
                    // RTC stream prepare
                    AgoraRtcStreamOptions option = new AgoraRtcStreamOptions();
                    mScene.createOrUpdateRTCStream(mLocalUserId, option);
                    // 准备视频采集
                    mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
                    // 必须先添加setPreviewCanvas，然后才能 startCapture
                    addLocalView();
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

            /**
             * 远端用户进入场景
             * @param list 所有用户
             */
            @Override
            public void onRemoteUserJoined(List<AgoraRteUserInfo> list) {
                ExampleUtil.utilLog("onRemoteUserJoined: " + list.toString());
                userInfoList.addAll(list);
            }

            /**
             * 远端用户离开场景
             */
            @Override
            public void onRemoteUserLeft(List<AgoraRteUserInfo> list) {
                ExampleUtil.utilLog("onRemoteUserLeft: " + list.toString());
                userInfoList.removeAll(list);
            }

            /**
             * 场景内远端用户开始推流
             */
            @Override
            public void onRemoteStreamAdded(List<AgoraRteMediaStreamInfo> list) {
                if (mBinding == null) return;
                streamInfoList.addAll(list);
                for (AgoraRteMediaStreamInfo info : list) {
                    addRemoteView(info.getStreamId());
                }
            }

            /**
             * OnRemoteStreamRemoved
             */
            @Override
            public void onRemoteStreamRemoved(List<AgoraRteMediaStreamInfo> list) {
                ExampleUtil.utilLog("onRemoteStreamRemoved: " + Thread.currentThread().getName());
                if (mBinding == null) return;
                streamInfoList.removeAll(list);
                for (AgoraRteMediaStreamInfo info : list) {
                    mBinding.containerJoinChannelVideo.dynamicRemoveViewWithTag(info.getStreamId());
                }
            }
        };
    }

    private void joinChannel() {
        doJoinChannel(channelName, mLocalUserId, "");
    }


    private void addLocalView() {
        TextureView view = new TextureView(requireContext());
        mBinding.containerJoinChannelVideo.demoAddView(view);
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(view);
        canvas.renderMode = AgoraRteVideoCanvas.RENDER_MODE_FIT;
        if (mLocalVideoTrack != null) {
            mLocalVideoTrack.setPreviewCanvas(canvas);
        }
    }

    private void addRemoteView(String streamId) {
        TextureView view = new TextureView(requireContext());
        view.setTag(streamId);
        mBinding.containerJoinChannelVideo.demoAddView(view);
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(view);
        mScene.setRemoteVideoCanvas(streamId, canvas);

        mScene.subscribeRemoteVideo(streamId, new AgoraRteVideoSubscribeOptions());
        mScene.subscribeRemoteAudio(streamId);
    }

    @Override
    public void doChangeView() {

    }
}
