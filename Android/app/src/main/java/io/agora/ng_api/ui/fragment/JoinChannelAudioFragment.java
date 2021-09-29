package io.agora.ng_api.ui.fragment;

import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;

import java.util.List;

import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentJoinChannelAudioBinding;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.ng_api.view.ScrollableLinearLayout;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;

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

            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState state, AgoraRteSceneConnState state1, io.agora.rte.scene.AgoraRteConnectionChangedReason reason) {
                ExampleUtil.utilLog("onConnectionStateChanged: " + state.getValue() + ", " + state1.getValue() + ",reason: " + reason.getValue() + "，\nThread:" + Thread.currentThread().getName());
                if (mBinding == null) return;
                // 连接建立完成
                if (state1 == AgoraRteSceneConnState.CONN_STATE_CONNECTED && mLocalAudioTrack == null) {
                    AgoraRtcStreamOptions option = new AgoraRtcStreamOptions();
                    mScene.createOrUpdateRTCStream(mLocalUserId, option);
                    // 必须先添加setPreviewCanvas，然后才能 startCapture
                    addVoiceView(null);

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
                for (AgoraRteMediaStreamInfo info : list)
                    addVoiceView(info);
            }

            @Override
            public void onRemoteStreamRemoved(List<AgoraRteMediaStreamInfo> list) {
                ExampleUtil.utilLog("onRemoteStreamRemoved: " + Thread.currentThread().getName());
                if (mBinding == null) return;
                for (AgoraRteMediaStreamInfo info : list) {
                    mBinding.containerJoinChannelAudio.dynamicRemoveViewWithTag(info.getStreamId());
                }
            }

        };

    }


    private void addVoiceView(@Nullable AgoraRteMediaStreamInfo info) {
        String tag = info == null ? null : info.getStreamId();
        String title = info == null ? getString(R.string.local_user_id_format,mLocalUserId) : info.getUserId();
        CardView cardView = ScrollableLinearLayout.getChildAudioCardView(requireContext(),tag,title);

        mBinding.containerJoinChannelAudio.demoAddView(cardView);

        if(tag!=null)
            mScene.subscribeRemoteAudio(tag);
    }

    private void joinChannel() {
        doJoinChannel(channelName, mLocalUserId, "");
    }

    @Override
    public void doChangeView() {

    }
}
