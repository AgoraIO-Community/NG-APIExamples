//
//  BasicVideoVC.swift
//  APIExample
//
//  Created by 张乾泽 on 2020/4/17.
//  Copyright © 2020 Agora Corp. All rights reserved.
//
import UIKit
import AGEVideoLayout
import AgoraRTE

class BasicVideoEntry : UIViewController
{
    @IBOutlet weak var joinButton: UIButton!
    @IBOutlet weak var channelTextField: UITextField!
    let identifier = "BasicVideo"
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    @IBAction func doJoinPressed(sender: UIButton) {
        guard let channelName = channelTextField.text else {return}
        //resign channel text field
        channelTextField.resignFirstResponder()
        
        let storyBoard: UIStoryboard = UIStoryboard(name: identifier, bundle: nil)
        // create new view controller every time to ensure we get a clean vc
        guard let newViewController = storyBoard.instantiateViewController(withIdentifier: identifier) as? BaseViewController else {return}
        newViewController.title = channelName
        newViewController.configs = ["channelName":channelName]
        self.navigationController?.pushViewController(newViewController, animated: true)
    }
}

class BasicVideoMain: BaseViewController {
    var localVideo = Bundle.loadVideoView(type: .local, audioOnly: false)
    var remoteVideo = Bundle.loadVideoView(type: .remote, audioOnly: false)
    
    @IBOutlet weak var container: AGEVideoContainer!
    
    var agoraKit: AgoraRteSdk!
    
    var scene: AgoraRteSceneProtocol!
    var microphoneTrack: AgoraRteMicrophoneAudioTrackProtocol!
    var cameraTrack: AgoraRteCameraVideoTrackProtocol!
    let LOCAL_USER_ID = String(UInt.random(in: 100...999))
    let LOCAL_STREAM_ID = String(UInt.random(in: 1000...2000))
    // indicate if current instance has joined channel
    var isJoined: Bool = false
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // get channel name from configs
        guard let channelName = configs["channelName"] as? String,
            let resolution = GlobalSettings.shared.getSetting(key: "resolution")?.selectedOption().value as? CGSize,
            let fps = GlobalSettings.shared.getSetting(key: "fps")?.selectedOption().value as? AgoraVideoFrameRate,
            let orientation = GlobalSettings.shared.getSetting(key: "orientation")?.selectedOption().value as? AgoraVideoOutputOrientationMode else {return}
        
        // layout render view
        localVideo.setPlaceholder(text: "Local Host".localized)
        remoteVideo.setPlaceholder(text: "Remote Host".localized)
        container.layoutStream(views: [localVideo, remoteVideo])
        
        // initialize sdk
        let profile = AgoraRteSdkProfile()
        profile.appid = KeyCenter.AppId
        agoraKit = AgoraRteSdk.sharedEngine(with: profile)
        
        // initialize media control
        let mediaControl = agoraKit.rteMediaFactory()
        cameraTrack = mediaControl?.createCameraVideoTrack()
        microphoneTrack = mediaControl?.createMicrophoneAudioTrack()
        
        // audio
        microphoneTrack?.startRecording()
        
        // video
        let videoCanvas = AgoraRtcVideoCanvas()
        videoCanvas.uid = 0
        videoCanvas.view = localVideo.videoView
        videoCanvas.renderMode = .hidden
        cameraTrack?.setPreviewCanvas(videoCanvas)
        cameraTrack.startCapture();
        
        //initilize streaming control
        scene = agoraKit.createRteScene(withSceneId: channelName, sceneConfig: AgoraRteSceneConfg())
        scene?.setSceneDelegate(self)
        let videoConfig = AgoraVideoEncoderConfiguration(size: resolution, frameRate: fps, bitrate: AgoraVideoBitrateStandard, orientationMode: orientation, mirrorMode: .auto)
        scene?.setVideoEncoderConfiguration(LOCAL_STREAM_ID, videoEncoderConfiguration: videoConfig)
        scene.joinScene(withUserId: LOCAL_USER_ID, token: "", joinOptions: AgoraRteJoinOptions())
        let streamOption = AgoraRteRtcStreamOptions()
        streamOption.token = ""
        scene?.createOrUpdateRTCStream(LOCAL_STREAM_ID, rtcStreamOptions: streamOption)
        scene?.publishLocalAudioTrack(LOCAL_STREAM_ID, rteAudioTrack: microphoneTrack!)
        scene?.publishLocalVideoTrack(LOCAL_STREAM_ID, rteVideoTrack: cameraTrack!)
    }
    
    override func willMove(toParent parent: UIViewController?) {
        if parent == nil {
            // leave channel when exiting the view
            if isJoined {
                scene.leave()
            }
        }
    }
}

extension BasicVideoMain: AgoraRteSceneDelegate {
    //
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, remoteUserDidJoin userInfos: [AgoraRteUserInfo]?) {
        print("didRemoteUserDidJoin")
    }
    // one user --> more streams so subscribe user by streamId
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, remoteStreamesDidAddWith streamInfos: [AgoraRteStreamInfo]?) {
        
        guard let infos = streamInfos else { return }
        for info in infos {
            // Only one remote video view is available for this
            // tutorial. Here we check if there exists a surface
            // view tagged as this uid.
            let videoCanvas = AgoraRtcVideoCanvas()
            videoCanvas.uid = UInt(info.streamId!)!
            videoCanvas.userId = info.userId
            // the view to be binded
            videoCanvas.view = remoteVideo.videoView
            videoCanvas.renderMode = .hidden
            rteScene.setRemoteVideoCanvas(info.streamId!, videoCanvas: videoCanvas)
            let option = AgoraRteVideoSubscribeOptions()
            rteScene.subscribeRemoteAudio(info.streamId!)
            rteScene.subscribeRemoteVideo(info.streamId!, videoSubscribeOptions: option)
            print("didRemoteStreamAdded" + "stream_id == \(String(describing: info.streamId))")

        }
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didConnectionStateChanged oldState: AgoraConnectionState, newState state: AgoraConnectionState, changedReason reason: AgoraConnectionChangedReason) {
        print("didConnectionStateChanged state:\(state.rawValue) reason:\(reason.rawValue)")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didLocalStreamStateChanged streams: AgoraRteStreamInfo?, mediaType: AgoraRteMediaType, steamMediaState oldState: AgoraRteStreamState, newState: AgoraRteStreamState, stateChangedReason reason: AgoraRteStreamStateChangedReason) {
        print("didLocalStreamStateChanged \(String(describing: streams?.streamId)), audio sentBitrate: \(String(describing: newState))")
    }
}
