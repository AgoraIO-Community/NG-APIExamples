package io.agora.ng_api.base;

import android.os.Bundle;
import android.view.TextureView;

import androidx.annotation.IntRange;
import androidx.annotation.Nullable;
import androidx.viewbinding.ViewBinding;

import java.util.Random;

import io.agora.extension.ExtensionManager;
import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.ui.fragment.DescriptionFragment;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.ng_api.view.DynamicView;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.AgoraRteSdkConfig;
import io.agora.rte.base.AgoraRteLogConfig;
import io.agora.rte.media.track.AgoraRteCameraVideoTrack;
import io.agora.rte.media.track.AgoraRteMicrophoneAudioTrack;
import io.agora.rte.media.video.AgoraRteVideoCanvas;
import io.agora.rte.scene.AgoraRteScene;
import io.agora.rte.scene.AgoraRteSceneConfig;
import io.agora.rte.scene.AgoraRteSceneEventHandler;
import io.agora.rte.scene.AgoraRteSceneJoinOptions;

/**
 * On Jetpack navigation
 * Fragments enter/exit represent onCreateView/onDestroyView
 * Thus we should detach all reference to the VIEW on onDestroyView
 */
public abstract class BaseDemoFragment<B extends ViewBinding> extends BaseFragment<B> {

    // COMMON FILED
    public final String mLocalUserId = String.valueOf(new Random().nextInt(Integer.MAX_VALUE/2));
    public final String mLocalStreamId = String.valueOf(new Random().nextInt(Integer.MAX_VALUE/2));
    public final String mLocalMediaStreamId = "media-" + new Random().nextInt(Integer.MAX_VALUE/2) + Integer.MAX_VALUE/2;
    public String sceneName;
    public AgoraRteScene mScene;
    public AgoraRteSceneEventHandler mAgoraHandler;
    @Nullable
    public AgoraRteCameraVideoTrack mLocalVideoTrack;
    @Nullable
    public AgoraRteMicrophoneAudioTrack mLocalAudioTrack;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // ensure sceneName
        if (getArguments() == null || getArguments().get(DescriptionFragment.sceneName) == null) {
            MyApp.getInstance().shortToast(R.string.scene_name_required);
            getNavController().popBackStack();
            return;
        }
        sceneName = getArguments().getString(DescriptionFragment.sceneName);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (!MyApp.justDebugUIPart) {
            doExitScene();
        }
    }

    public void initAgoraRteSDK() {
        initAgoraRteSDK(false);
    }
    /**
     * app id 优先级
     * 用户主动输入 > strings 内置
     */
    public void initAgoraRteSDK(boolean enableExtension) {
        AgoraRteSdkConfig profile = new AgoraRteSdkConfig();
        // check SP
        String appId = ExampleUtil.getSp(requireContext()).getString(ExampleUtil.APPID, "");
        // check strings
        if (appId.isEmpty()) appId = getString(R.string.agora_app_id);
        profile.appId = appId;
        profile.context = requireContext();

        if(enableExtension)
            profile.addExtension(ExtensionManager.EXTENSION_NAME);
        profile.logConfig = new AgoraRteLogConfig(requireContext().getFilesDir().getAbsolutePath());
        AgoraRteSDK.init(profile);
    }

    /**
     * @param sceneName scene name
     * @param userId      userId
     * @param token       access token
     */
    public void doJoinScene(String sceneName, String userId, String token) {
        doJoinScene(sceneName, userId, token, new AgoraRteSceneConfig());
    }

    public void doJoinScene(String sceneName, String userId, String token, AgoraRteSceneConfig config) {
        mScene = AgoraRteSDK.createRteScene(sceneName, config);
        mScene.registerSceneEventHandler(mAgoraHandler);
        mScene.join(userId, token, new AgoraRteSceneJoinOptions());
    }

    public void doExitScene() {
        if(mLocalAudioTrack != null)
            mLocalAudioTrack.destroy();
        if(mLocalVideoTrack != null)
            mLocalVideoTrack.destroy();
        if (mScene != null) {
            mScene.leave();
            mScene.destroy();
        }
        AgoraRteSDK.deInit();
    }

    public void initLocalAudioTrack(){
        mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
        if(mLocalAudioTrack != null) {
            mLocalAudioTrack.startRecording();
            mScene.publishLocalAudioTrack(mLocalStreamId, mLocalAudioTrack);
        }
    }

    public void initLocalVideoTrack(DynamicView dynamicView){
        mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
        // 必须先添加setPreviewCanvas，然后才能 startCapture
        // Must first setPreviewCanvas, then we can startCapture
        addLocalView(dynamicView);
        if (mLocalVideoTrack != null) {
            mLocalVideoTrack.startCapture(null);
            mScene.publishLocalVideoTrack(mLocalStreamId, mLocalVideoTrack);
        }
    }

    public void addLocalView(DynamicView dynamicView) {
        addLocalView(dynamicView,AgoraRteVideoCanvas.RENDER_MODE_FIT);
    }

    public void addLocalView(DynamicView dynamicView, @IntRange(from = 1,to = 3) int renderMode) {
        TextureView textureView = new TextureView(requireContext());
        dynamicView.dynamicAddView(textureView);

        AgoraRteVideoCanvas videoCanvas = new AgoraRteVideoCanvas(textureView);
        videoCanvas.renderMode = renderMode;
        if (mLocalVideoTrack != null)
            mLocalVideoTrack.setPreviewCanvas(videoCanvas);
    }

}
