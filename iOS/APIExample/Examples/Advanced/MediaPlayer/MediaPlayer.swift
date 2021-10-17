//
//  MediaPlayer.swift
//  APIExample
//
//  Created by 张乾泽 on 2020/4/17.
//  Copyright © 2020 Agora Corp. All rights reserved.
//
import UIKit
import AGEVideoLayout
import AgoraRTE

let LOCAL_USER_ID = String(UInt.random(in: 100...999))
let LOCAL_STREAM_ID = String(UInt.random(in: 1000...2000))
let PLAYER_STREAM_ID =  String(UInt.random(in: 2000...3000))

class MediaPlayerEntry : UIViewController
{
    @IBOutlet weak var joinButton: UIButton!
    @IBOutlet weak var channelTextField: UITextField!
    let identifier = "MediaPlayer"
    
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

class MediaPlayerMain: BaseViewController, UITextFieldDelegate {
    var localVideo = Bundle.loadView(fromNib: "VideoView", withType: VideoView.self)
    var remoteVideo = Bundle.loadView(fromNib: "VideoView", withType: VideoView.self)
    
    @IBOutlet weak var container: AGEVideoContainer!
    @IBOutlet weak var mediaUrlField: UITextField!
    @IBOutlet weak var playerControlStack: UIStackView!
    @IBOutlet weak var playerProgressSlider: UISlider!
    @IBOutlet weak var playoutVolume: UISlider!
    @IBOutlet weak var publishVolume: UISlider!
    @IBOutlet weak var playerDurationLabel: UILabel!
    @IBAction func stop(_ sender: Any) {
        
        player.stop();
    }
    @IBAction func play(_ sender: Any) {
        player.play()
    }
    
    @IBAction func open(_ sender: Any) {
        guard let url = mediaUrlField.text else { return }
        player.openUrl(url, startPos: 0)
    }
    
    @IBAction func pause(sender: UIButton) {
        player.pause()
    }
    
    @IBAction func doAdjustPlayoutVolume(sender: UISlider) {
        player.adjustPlayoutVolume(Int(sender.value))
    }
    
    var agoraKit: AgoraRteSdk!
    var player : AgoraRteMediaPlayerProtocol!
    
    var scene: AgoraRteSceneProtocol!
    var microphoneTrack: AgoraRteMicrophoneAudioTrackProtocol!
    var cameraTrack: AgoraRteCameraVideoTrackProtocol!
    private var originY: CGFloat = 0
    
    // indicate if current instance has joined channel
    var isJoined: Bool = false
    
    @objc func keyboardWillAppear(notification: NSNotification) {
        let keyboardinfo = notification.userInfo![UIResponder.keyboardFrameEndUserInfoKey]
        let keyboardheight:CGFloat = (keyboardinfo as AnyObject).cgRectValue.size.height
   
        if self.originY == 0 {
//            self.originY = self.view.centerY_CS
        }
        let rect = self.mediaUrlField.convert(self.mediaUrlField.bounds, to: self.view)
        let y = self.view.bounds.height - rect.origin.y - self.mediaUrlField.bounds.height - keyboardheight

        if y < 0 {
            let animator = UIViewPropertyAnimator(duration: 0.2, curve: .easeOut) {
//                self.view.centerY_CS = y + self.originY
            }
            animator.startAnimation()
        }
    }
    
    @objc func keyboardWillDisappear(notification:NSNotification){
        let animator = UIViewPropertyAnimator(duration: 0.2, curve: .easeOut) {
//            self.view.centerY_CS = self.originY
            self.originY = 0
        }
        animator.startAnimation()
    }
    
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        textField.resignFirstResponder()
        return true
    }
    
    func textView(_ textView: UITextView, shouldChangeTextIn range: NSRange, replacementText text: String) -> Bool {
        if text == "\n" {
            textView.resignFirstResponder()
            return true
        }
        return false
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // get channel name from configs
        guard let channelName = configs["channelName"] as? String else {return}
        
        // setup view
        mediaUrlField.delegate = self
        NotificationCenter.default.addObserver(self, selector: #selector(keyboardWillAppear), name: UIResponder.keyboardWillShowNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(keyboardWillDisappear), name: UIResponder.keyboardWillHideNotification, object: nil)
        // layout render view
        localVideo.setPlaceholder(text: "No Player Loaded")
        remoteVideo.setPlaceholder(text: "Remote Host".localized)
        container.layoutStream1x2(views: [localVideo, remoteVideo])
        
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
        
        // camera
        let videoCanvas = AgoraRtcVideoCanvas()
        videoCanvas.uid = 0
        videoCanvas.view = localVideo.videoView
        videoCanvas.renderMode = .hidden
        cameraTrack?.setPreviewCanvas(videoCanvas)
        cameraTrack?.startCapture();
        
        // media player
        player = mediaControl?.createMediaPlayer()
        player.setView(localVideo.videoView)
        player.setAgoraRtePlayerDelegate(self)
        
        //initilize streaming control
        scene = agoraKit.createRteScene(withSceneId: channelName, sceneConfig: AgoraRteSceneConfg())
        scene?.setSceneDelegate(self)
        scene.joinScene(withUserId: LOCAL_USER_ID, token: "", joinOptions: AgoraRteJoinOptions())
        let streamOption = AgoraRteRtcStreamOptions()
        streamOption.token = ""
        scene?.createOrUpdateRTCStream(LOCAL_STREAM_ID, rtcStreamOptions: streamOption)
        scene?.publishLocalAudioTrack(LOCAL_STREAM_ID, rteAudioTrack: microphoneTrack!)
        scene?.publishLocalVideoTrack(LOCAL_STREAM_ID, rteVideoTrack: cameraTrack!)
        scene?.createOrUpdateRTCStream(PLAYER_STREAM_ID, rtcStreamOptions: streamOption)
        scene.publishMediaPlayer(PLAYER_STREAM_ID, mediaPlayer: player)
        
    }
    
    override func willMove(toParent parent: UIViewController?) {
        if parent == nil {
            // leave channel when exiting the view
            if isJoined {
                player.stop()
                scene.leave()
            }
        }
    }
}

extension MediaPlayerMain: AgoraRteSceneDelegate {
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

extension MediaPlayerMain:AgoraRteMediaPlayerDelegate {
    func agoraRteMediaPlayer(_ playerKit: AgoraRteMediaPlayerProtocol, stateDidChangeTo state: AgoraMediaPlayerState, error: AgoraMediaPlayerError, rteFileInfo fileInfo: AgoraRteFileInfo?) {
            DispatchQueue.main.async {[weak self] in
                guard let weakself = self else { return }
                switch state {
                case .failed:
                    weakself.showAlert(message: "media player error: \(error.rawValue)")
                    break
                case .openCompleted:
                    let duration = weakself.player.duration()
                    weakself.playerControlStack.isHidden = false
                    weakself.playerDurationLabel.text = "\(String(format: "%02d", duration / 60000)) : \(String(format: "%02d", duration % 60000 / 1000))"
                    weakself.playerProgressSlider.setValue(0, animated: true)
                    break
                case .stopped:
                    weakself.playerControlStack.isHidden = true
                    weakself.playerProgressSlider.setValue(0, animated: true)
                    weakself.playerDurationLabel.text = "00 : 00"
                    break
                default: break
                
            }
        }
    }
}
