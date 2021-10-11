//
//  JoinChannelVC.swift
//  APIExample
//
//  Created by 张乾泽 on 2020/4/17.
//  Copyright © 2020 Agora Corp. All rights reserved.
//
import UIKit
import AGEVideoLayout
import AgoraRTE

class JoinChannelVideoEntry : UIViewController
{
    @IBOutlet weak var joinButton: UIButton!
    @IBOutlet weak var channelTextField: UITextField!
    let identifier = "JoinChannelVideo"
    @IBOutlet var resolutionBtn: UIButton!
    @IBOutlet var fpsBtn: UIButton!
    @IBOutlet var orientationBtn: UIButton!
    var width:Int = 640, height:Int = 360, orientation:AgoraVideoOutputOrientationMode = .adaptative, fps = 30
    
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    
    func getResolutionAction(width:Int, height:Int) -> UIAlertAction{
        return UIAlertAction(title: "\(width)x\(height)", style: .default, handler: {[unowned self] action in
            self.width = width
            self.height = height
            self.resolutionBtn.setTitle("\(width)x\(height)", for: .normal)
        })
    }
    
    func getFpsAction(_ fps:Int) -> UIAlertAction{
        return UIAlertAction(title: "\(fps)fps", style: .default, handler: {[unowned self] action in
            self.fps = fps
            self.fpsBtn.setTitle("\(fps)fps", for: .normal)
        })
    }
    
    func getOrientationAction(_ orientation:AgoraVideoOutputOrientationMode) -> UIAlertAction{
        return UIAlertAction(title: "\(orientation.description())", style: .default, handler: {[unowned self] action in
            self.orientation = orientation
            self.orientationBtn.setTitle("\(orientation.description())", for: .normal)
        })
    }
    
    @IBAction func setResolution(){
        let alert = UIAlertController(title: "Set Resolution".localized, message: nil, preferredStyle: .actionSheet)
        alert.addAction(getResolutionAction(width: 90, height: 90))
        alert.addAction(getResolutionAction(width: 160, height: 120))
        alert.addAction(getResolutionAction(width: 320, height: 240))
        alert.addAction(getResolutionAction(width: 640, height: 360))
        alert.addAction(getResolutionAction(width: 1280, height: 720))
        alert.addCancelAction()
        present(alert, animated: true, completion: nil)
    }
    
    @IBAction func setFps(){
        let alert = UIAlertController(title: "Set Fps".localized, message: nil, preferredStyle: .actionSheet)
        alert.addAction(getFpsAction(10))
        alert.addAction(getFpsAction(15))
        alert.addAction(getFpsAction(24))
        alert.addAction(getFpsAction(30))
        alert.addAction(getFpsAction(60))
        alert.addCancelAction()
        present(alert, animated: true, completion: nil)
    }
    
    @IBAction func setOrientation(){
        let alert = UIAlertController(title: "Set Orientation".localized, message: nil, preferredStyle: .actionSheet)
        alert.addAction(getOrientationAction(.adaptative))
        alert.addAction(getOrientationAction(.fixedLandscape))
        alert.addAction(getOrientationAction(.fixedPortrait))
        alert.addCancelAction()
        present(alert, animated: true, completion: nil)
    }
    
    @IBAction func doJoinPressed(sender: UIButton) {
        guard let channelName = channelTextField.text else {return}
        //resign channel text field
        channelTextField.resignFirstResponder()
        
        let storyBoard: UIStoryboard = UIStoryboard(name: identifier, bundle: nil)
        // create new view controller every time to ensure we get a clean vc
        guard let newViewController = storyBoard.instantiateViewController(withIdentifier: identifier) as? BaseViewController else {return}
        newViewController.title = channelName
        newViewController.configs = ["channelName":channelName, "resolution":CGSize(width: width, height: height), "fps": fps, "orientation": orientation]
        self.navigationController?.pushViewController(newViewController, animated: true)
    }
}

class JoinChannelVideoMain: BaseViewController {
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
            let resolution = configs["resolution"] as? CGSize,
            let fps = configs["fps"] as? Int,
            let orientation = configs["orientation"] as? AgoraVideoOutputOrientationMode else {return}
        
        // layout render view
        localVideo.setPlaceholder(text: "Local Host".localized)
        remoteVideo.setPlaceholder(text: "Remote Host".localized)
        container.layoutStream(views: [localVideo, remoteVideo])
        
        let profile = AgoraRteSdkProfile()
        profile.appid = KeyCenter.AppId
        agoraKit = AgoraRteSdk.sharedEngine(with: profile)
        let config = AgoraRteSceneConfg()
        config.enableAudioRecordingOrPlayout = true
        scene = agoraKit.createRteScene(withSceneId: channelName, sceneConfig: config)
        
        scene?.setSceneDelegate(self)
        let videoConfig = AgoraVideoEncoderConfiguration(size: resolution, frameRate: AgoraVideoFrameRate(rawValue: fps) ?? .fps15, bitrate: AgoraVideoBitrateStandard, orientationMode: orientation, mirrorMode: .auto)
        scene?.setVideoEncoderConfiguration(LOCAL_STREAM_ID, videoEncoderConfiguration: videoConfig)
        
        let mediaControl = agoraKit.rteMediaFactory()
        cameraTrack = mediaControl?.createCameraVideoTrack()
        microphoneTrack = mediaControl?.createMicrophoneAudioTrack()
        let joinOption = AgoraRteJoinOptions()
        joinOption.isUserVisibleToRemote = true
        
        scene.joinScene(withUserId: LOCAL_USER_ID, token: "", joinOptions: joinOption)
        microphoneTrack?.startRecording()
        
        let videoCanvas = AgoraRtcVideoCanvas()
        videoCanvas.uid = 0
        videoCanvas.view = localVideo.videoView
        videoCanvas.renderMode = .hidden
        cameraTrack?.setPreviewCanvas(videoCanvas)
        
        let streamOption = AgoraRteRtcStreamOptions()
        streamOption.token = ""
        cameraTrack.startCapture();
        scene?.createOrUpdateRTCStream(LOCAL_STREAM_ID, rtcStreamOptions: streamOption)
        scene?.publishLocalAudioTrack(LOCAL_STREAM_ID, rteAudioTrack: microphoneTrack!)
        scene.publishLocalVideoTrack(LOCAL_STREAM_ID, rteVideoTrack: cameraTrack!)
        scene?.createOrUpdateRTCStream(PLAYER_STREAM_ID, rtcStreamOptions: streamOption)
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

var users:Int = 0
extension JoinChannelVideoMain: AgoraRteSceneDelegate {
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
            users = (users + 1)%2

        }
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didConnectionStateChanged oldState: AgoraConnectionState, newState state: AgoraConnectionState, changedReason reason: AgoraConnectionChangedReason) {
        print("didConnectionStateChanged state:\(state.rawValue) reason:\(reason.rawValue)")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didLocalStreamStateChanged streams: AgoraRteStreamInfo?, mediaType: AgoraRteMediaType, steamMediaState oldState: AgoraRteStreamState, newState: AgoraRteStreamState, stateChangedReason reason: AgoraRteStreamStateChangedReason) {
        print("didLocalStreamStateChanged \(String(describing: streams?.streamId)), audio sentBitrate: \(String(describing: newState))")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, localStreamDidStats streamId: String?, stats: AgoraRteLocalStreamStats?) {
        print("didLocalStreamStats \(String(describing: streamId)), audio sentBitrate: \(String(describing: stats?.audioStats?.sentBitrate))")
        remoteVideo.statsInfo?.updateLocalVideoStats(stats!)
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, sceneStats stats: AgoraRteSceneStats?) {
        print("didSceneStats")
        guard stats != nil else {
            return
        }
        localVideo.statsInfo?.updateChannelStats(stats!)
    }
}
