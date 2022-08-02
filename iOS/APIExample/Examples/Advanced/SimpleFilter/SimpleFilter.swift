//
//  SimpleFilter.swift
//  APIExample
//
//  Created by ADMIN on 2020/5/18.
//  Copyright © 2020 Agora Corp. All rights reserved.
//

import UIKit
import AgoraRTE
import AGEVideoLayout
import SimpleFilter

class SimpleFilterEntry : UIViewController
{
    @IBOutlet weak var joinButton: AGButton!
    @IBOutlet weak var channelTextField: AGTextField!
    let identifier = "SimpleFilter"
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    @IBAction func doJoinPressed(sender: AGButton) {
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

class SimpleFilterMain: BaseViewController {
    
    @IBOutlet weak var container: AGEVideoContainer!
    var localVideo = Bundle.loadVideoView(type: .local, audioOnly: false)
    var remoteVideo = Bundle.loadVideoView(type: .remote, audioOnly: false)
    let AUDIO_FILTER_NAME = "VolumeChange"
    let VIDEO_FILTER_NAME = "Watermark"
    
    var agoraKit: AgoraRteSdk!
    
    var scene: AgoraRteSceneProtocol!
    var microphoneTrack: AgoraRteMicrophoneAudioTrackProtocol!
    var cameraTrack: AgoraRteCameraVideoTrackProtocol!
    let LOCAL_USER_ID = String(UInt.random(in: 100...999))
    let LOCAL_STREAM_ID = String(UInt.random(in: 1000...2000))
    
    // indicate if current instance has joined channel
    var isJoined: Bool = false
    
    override func viewDidLoad(){
        super.viewDidLoad()
        
        // get channel name from configs
        guard let channelName = configs["channelName"] as? String
            else { return }
        
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
        microphoneTrack?.enableExtension(withProviderName: SimpleFilterManager.vendorName(), extensionName: AUDIO_FILTER_NAME)
        microphoneTrack?.startRecording()
        
        // video
        let videoCanvas = AgoraRtcVideoCanvas()
        videoCanvas.uid = 0
        videoCanvas.view = localVideo.videoView
        videoCanvas.renderMode = .hidden
        cameraTrack?.setPreviewCanvas(videoCanvas)
        cameraTrack?.enableExtension(withProviderName: SimpleFilterManager.vendorName(), extensionName: VIDEO_FILTER_NAME)
        cameraTrack?.startCapture();
        
        //initilize streaming control
        scene = agoraKit.createRteScene(withSceneId: channelName, sceneConfig: AgoraRteSceneConfg())
        scene?.setSceneDelegate(self)
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
                scene?.leave()
            }
        }
    }
    
    @IBAction func onChangeRecordingVolume(_ sender:UISlider){
        let value:Int = Int(sender.value)
        print("adjustRecordingSignalVolume \(value)")
        microphoneTrack.setExtensionPropertyWithProviderName(SimpleFilterManager.vendorName(), extensionName: AUDIO_FILTER_NAME, key: "volume", jsonValue: String(value))
    }
}

extension SimpleFilterMain: AgoraRteSceneDelegate {
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

extension SimpleFilterMain: AgoraMediaFilterEventDelegate{
    func onEvent(_ vendor: String?, extension: String?, key: String?, json_value: String?) {
        LogUtils.log(message: "onEvent: \(String(describing: key)) \(String(describing: json_value))", level: .info)
    }
}
