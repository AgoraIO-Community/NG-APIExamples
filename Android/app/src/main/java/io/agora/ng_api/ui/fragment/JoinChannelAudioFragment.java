package io.agora.ng_api.ui.fragment;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

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
import io.agora.rte.statistics.AgoraRteLocalAudioStats;
import io.agora.rte.statistics.AgoraRteRemoteAudioStats;

/**
 * This demo demonstrates how to make a one-to-one video call version 2
 */
public class JoinChannelAudioFragment extends BaseDemoFragment<FragmentJoinChannelAudioBinding> {

    private final Map<String, MutableLiveData<String>> liveStat = new HashMap<>();

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

                    // Stop update stat
                    LiveData<String> test = liveStat.get(info.getStreamId());
                    if (test != null) {
                        test.removeObservers(getViewLifecycleOwner());
                        liveStat.remove(info.getStreamId());
                    }
                    // Remove view
                    mBinding.containerJoinChannelAudio.dynamicRemoveViewWithTag(info.getStreamId());
                }
            }

            @Override
            public void onLocalStreamAudioStats(String streamId, AgoraRteLocalAudioStats stats) {
                MutableLiveData<String> test = liveStat.get(mLocalUserId);
                if (test != null) {
                    String sb = "ChannelCount:" + stats.getNumChannels() + "\n" +
                            stats.getSendBitrateInKbps() + "Kbps " +
                            stats.getSentSampleRate() + "Hz";
                    test.setValue(sb);
                }
            }

            @Override
            public void onRemoteStreamAudioStats(String streamId, AgoraRteRemoteAudioStats stats) {
                MutableLiveData<String> test = liveStat.get(streamId);
                if (test != null) {
                    String sb = "ChannelCount:" + stats.getNumChannels() + "\n" +
                            stats.getReceivedBitrate() + "Kbps " +
                            stats.getReceivedSampleRate() + "Hz "+stats.getAudioLossRate()+"%";
                    test.setValue(sb);
                }
            }
        };

    }


    private void addVoiceView(@Nullable AgoraRteMediaStreamInfo info) {
        String tag = info == null ? null : info.getStreamId();
        String title = info == null ? getString(R.string.local_user_id_format, mLocalUserId) : info.getUserId();
        CardView cardView = ScrollableLinearLayout.getChildAudioCardView(requireContext(), tag, title);

        mBinding.containerJoinChannelAudio.demoAddView(cardView);

        // Start listen data
        MutableLiveData<String> mutableLiveData = new MutableLiveData<>();
        mutableLiveData.observe(getViewLifecycleOwner(), s -> ((TextView) cardView.getChildAt(0)).setText(s));

        if (tag != null) {
            liveStat.put(tag, mutableLiveData);
            // Start receive audio data
            mScene.subscribeRemoteAudio(tag);
        }else{
            liveStat.put(mLocalUserId, mutableLiveData);
        }

    }

    private void joinChannel() {
        doJoinChannel(channelName, mLocalUserId, "");
    }

    @Override
    public void doChangeView() {

    }
}
