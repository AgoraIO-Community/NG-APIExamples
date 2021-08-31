package io.agora.ng_api.ui.fragment;

import android.content.Context;
import android.graphics.Color;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentJoinChannelAudioBinding;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.*;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;
import io.agora.rte.user.AgoraRteUserInfo;

import java.util.List;
import java.util.Random;

/**
 * This demo demonstrates how to make a one-to-one video call version 2
 */
public class JoinChannelAudioFragment extends BaseDemoFragment<FragmentJoinChannelAudioBinding> {

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
                if (mBinding == null) return;
                // 连接建立完成
                if (state1 == AgoraRteSceneConnState.CONN_STATE_CONNECTED) {
                    AgoraRtcStreamOptions option = new AgoraRtcStreamOptions();
                    mScene.createOrUpdateRTCStream(mLocalUserId, option);
                    // 必须先添加setPreviewCanvas，然后才能 startCapture
                    addVoiceView();

                    // 准备音频采集
                    mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
                    mLocalAudioTrack.adjustPublishVolume(100);
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
                if (mBinding == null) return;
                userInfoList.addAll(list);
            }

            /**
             * 远端用户离开场景
             */
            @Override
            public void onRemoteUserLeft(List<AgoraRteUserInfo> list) {
                ExampleUtil.utilLog("onRemoteUserLeft: " + list.toString());
                if(mBinding == null) return;
                userInfoList.removeAll(list);
            }

            /**
             * 场景内远端用户开始推流
             */
            @Override
            public void onRemoteStreamAdded(List<AgoraRteMediaStreamInfo> list) {
                if (mBinding == null) return;
                streamInfoList.addAll(list);
                for (AgoraRteMediaStreamInfo info : list)
                    addVoiceView(info);
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
                    mBinding.containerJoinChannelAudio.dynamicRemoveViewWithTag(info.getStreamId());
                }
            }

        };

    }

    private void addVoiceView() {
        FrameLayout view = mBinding.containerJoinChannelAudio.createDemoLayout(FrameLayout.class);
        // config title
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.gravity = Gravity.CENTER;

        TextView title = new TextView(requireContext());
        title.setLayoutParams(lp);
        title.setText(getString(R.string.local_user_id_format,mLocalUserId));

        view.setBackgroundColor(Color.rgb(new Random().nextInt(255), new Random().nextInt(255), new Random().nextInt(255)));
        view.addView(title);
        mBinding.containerJoinChannelAudio.demoAddView(view);
    }

    private void addVoiceView(AgoraRteMediaStreamInfo info) {
        FrameLayout view = mBinding.containerJoinChannelAudio.createDemoLayout(FrameLayout.class);
        view.setTag(info.getStreamId());
        view.setBackgroundColor(Color.rgb(new Random().nextInt(255), new Random().nextInt(255), new Random().nextInt(255)));

        // config title
        TextView title = new TextView(requireContext());
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.gravity = Gravity.CENTER;
        title.setLayoutParams(lp);
        title.setText(info.getUserId());

        view.addView(title);

        mBinding.containerJoinChannelAudio.demoAddView(view);
        mScene.subscribeRemoteAudio(info.getStreamId());
    }

    private void joinChannel() {
        doJoinChannel(channelName, mLocalUserId, "");
    }

    @Override
    public void doChangeView() {

    }
}
