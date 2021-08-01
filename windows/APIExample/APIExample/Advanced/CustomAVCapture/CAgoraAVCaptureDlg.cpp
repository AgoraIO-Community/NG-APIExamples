#include "stdafx.h"
#include "APIExample.h"
#include "CAgoraAVCaptureDlg.h"
#include "AgVideoBuffer.h"

#define HAVE_JPEG
#include "libyuv.h"

#ifdef DEBUG
#pragma comment(lib, "yuv.lib")
#pragma comment(lib, "jpeg-static.lib")
#else
#pragma comment(lib, "yuv.lib")
#pragma comment(lib, "jpeg-static.lib")
#endif
#include <set>


IMPLEMENT_DYNAMIC(CAgoraAVCaptureDlg, CDialogEx)

CAgoraAVCaptureDlg::CAgoraAVCaptureDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CUSTOM_CAPTURE_VIDEO, pParent)
{

}

CAgoraAVCaptureDlg::~CAgoraAVCaptureDlg()
{
	if (videoFrame_.buffer) {
		delete[] videoFrame_.buffer;
		videoFrame_.buffer = nullptr;
	}
}

void CAgoraAVCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_VIDEO, m_staVideoArea);
	DDX_Control(pDX, IDC_STATIC_CHANNELNAME, m_staChannelName);
	DDX_Control(pDX, IDC_STATIC_CAPTUREDEVICE, m_staCaputreVideo);
	DDX_Control(pDX, IDC_EDIT_CHANNELNAME, m_edtChannel);
	DDX_Control(pDX, IDC_BUTTON_JOINCHANNEL, m_btnJoinChannel);
	DDX_Control(pDX, IDC_BUTTON_START_CAPUTRE, m_btnSetExtCapture);
	DDX_Control(pDX, IDC_COMBO_CAPTURE_VIDEO_DEVICE, m_cmbVideoDevice);
	DDX_Control(pDX, IDC_COMBO_CAPTURE_VIDEO_TYPE, m_cmbVideoType);
	DDX_Control(pDX, IDC_LIST_INFO_BROADCASTING, m_lstInfo);
	DDX_Control(pDX, IDC_COMBO_CAPTURE_AUDIO_DEVICE2, m_cmbAudioDevice);
}

BEGIN_MESSAGE_MAP(CAgoraAVCaptureDlg, CDialogEx)
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_MSGID(EID_CONNECTION_STATE), &CAgoraAVCaptureDlg::OnEIDConnectionStateChanged)
	ON_MESSAGE(WM_MSGID(EID_JOINCHANNEL_SUCCESS), &CAgoraAVCaptureDlg::OnEIDJoinChannelSuccess)
	ON_MESSAGE(WM_MSGID(EID_LEAVE_CHANNEL), &CAgoraAVCaptureDlg::OnEIDLeaveChannel)
	ON_MESSAGE(WM_MSGID(EID_USER_JOINED), &CAgoraAVCaptureDlg::OnEIDUserJoined)
	ON_MESSAGE(WM_MSGID(EID_USER_OFFLINE), &CAgoraAVCaptureDlg::OnEIDUserOffline)
	ON_MESSAGE(WM_MSGID(EID_REMOTE_VIDEO_STATE_CHANED), &CAgoraAVCaptureDlg::OnEIDRemoteVideoStateChanged)
	ON_BN_CLICKED(IDC_BUTTON_START_CAPUTRE, &CAgoraAVCaptureDlg::OnClickedButtonStartCaputre)
	ON_BN_CLICKED(IDC_BUTTON_JOINCHANNEL, &CAgoraAVCaptureDlg::OnClickedButtonJoinchannel)
	ON_CBN_SELCHANGE(IDC_COMBO_CAPTURE_VIDEO_DEVICE, &CAgoraAVCaptureDlg::OnSelchangeComboCaptureVideoDevice)
	
END_MESSAGE_MAP()

//set control text from config.
void CAgoraAVCaptureDlg::InitCtrlText()
{
	m_staChannelName.SetWindowText(commonCtrlChannel);
	m_staCaputreVideo.SetWindowText(customVideoCaptureCtrlCaptureVideoDevice);
	m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);
	m_btnSetExtCapture.SetWindowText(customVideoCaptureCtrlSetExternlCapture);
}

/*
	create Agora RTC Engine and initialize context.set channel property.
*/
bool CAgoraAVCaptureDlg::InitAgora()
{
	agora::rte::AgoraRteLogger::EnableLogging(true);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteLogger::EnableLogging"));
	agora::rte::AgoraRteLogger::SetLevel(agora::rte::LogLevel::Verbose);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteLogger::SetLevel Verbose"));
	agora::rte::AgoraRteLogger::SetListener([](const std::string& message) { /*std::cout << message;*/ });

	agora::rte::SdkProfile profile;
	profile.appid = GET_APP_ID;
	agora::rte::AgoraRteSDK::Init(profile, bCompatible);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteSDK::Init"));
	media_control_ = agora::rte::AgoraRteSDK::GetRteMediaFactory();
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteSDK::GetRteMediaFactory"));
	customVideoTrack_ = media_control_->CreateCustomVideoTrack();
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateCustomVideoTrack"));
	customAudioTrack_ = media_control_->CreateCustomAudioTrack();
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateCustomAudioTrack"));
	m_initialize = true;

	if (bCompatible)
		local_user_id_ = GenerateUserId();

	else
		local_user_id_ = GenerateRandomString(local_user_id_, id_random_len);

	custom_audio_id = local_user_id_;
	custom_video_id_ = local_user_id_;

	InitExternalDevice();
	return true;
}

void CAgoraAVCaptureDlg::InitExternalDevice()
{
	videoDevice_.ResetGraph();
	videoDevice_.EnumVideoDevices(videoDevices_);
	WASAPISource::GetAudioDevices(audioDevicesInfo_, true);//reocrding device
}

/*
	stop and release agora rtc engine.
*/
void CAgoraAVCaptureDlg::UnInitAgora()
{
	if (m_joinChannel) {
		//leave channel
		m_joinChannel = !m_joinChannel;
		if (m_scene) {
			m_scene->UnpublishLocalVideoTrack(customVideoTrack_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("UnpublishLocalVideoTrack custom"));

			m_scene->Leave();
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("Leave Scene"));
		}

		agora::rte::AgoraRteSDK::Deinit();
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("Deinit rte SDK"));
	}
}


/*
	initialize dialog, and set control property.
*/
BOOL CAgoraAVCaptureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_localVideoWnd.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CRect(0, 0, 1, 1), this, ID_BASEWND_VIDEO + 100);
	RECT rcArea;
	m_staVideoArea.GetClientRect(&rcArea);
	m_localVideoWnd.MoveWindow(&rcArea);
	ResumeStatus();

	connectionStates.push_back("CONNECTING");
	connectionStates.push_back("JOIN SUCCESS");
	connectionStates.push_back("INTERRUPTED");
	connectionStates.push_back("JOIN BANNED_BY_SERVER");

	connectionStates.push_back("JOIN FAILED");
	connectionStates.push_back("LEAVE CHANNEL");

	connectionStates.push_back("Invalid APPID");
	connectionStates.push_back("Invalid Channel Name");
	connectionStates.push_back("Invalid Token");
	connectionStates.push_back("Token Expired");
	connectionStates.push_back("Rejected By Server");

	connectionStates.push_back("Setting Proxy Server");
	connectionStates.push_back("Renew Token");
	connectionStates.push_back("Client IP Address Changed");
	connectionStates.push_back("Keep Alive Timeout");
	connectionStates.push_back("Rejoin Success");
	connectionStates.push_back("Lost");
	connectionStates.push_back("echo test");
	connectionStates.push_back("Client IP Address Changed By User");

	m_btnSetExtCapture.ShowWindow(SW_HIDE);
	m_cmbVideoType.ShowWindow(SW_HIDE);
	return TRUE;
}

/*
	register or unregister agora video Frame Observer.
*/


// update window view and control.
void CAgoraAVCaptureDlg::UpdateViews()
{
	for (int i = 0; i < videoDevices_.size(); ++i) {
		DShow::VideoConfig videoConfig;
		videoConfig.path = videoDevices_[i].path;
		videoDevice_.GetVideoConfig(videoConfig);
		m_cmbVideoDevice.InsertString(i, videoDevices_[i].name.c_str());
	}

	if (videoDevices_.size() > 0) {
		m_cmbVideoType.SetCurSel(0);
	}

	for (int i = 0; i < audioDevicesInfo_.size(); ++i) {
		m_cmbAudioDevice.InsertString(i, utf82cs(audioDevicesInfo_[i].name));
	}
	if (audioDevicesInfo_.size() > 0) {
		m_cmbAudioDevice.SetCurSel(0);
		wasapiInput = std::make_unique<WASAPISource>(audioDevicesInfo_[0].id, true, false);
	}
	
	// render local video
	RenderLocalVideo();
	// enumerate device and show.
	UpdateDevice();
}

// enumerate device and show device in combobox.
void CAgoraAVCaptureDlg::UpdateDevice()
{
	
	m_cmbVideoDevice.SetCurSel(0);
	OnSelchangeComboCaptureVideoDevice();
}
// resume window status.
void CAgoraAVCaptureDlg::ResumeStatus()
{
	m_lstInfo.ResetContent();
	InitCtrlText();
	
	m_joinChannel = false;
	m_initialize = false;
	m_remoteJoined = false;
	m_extenalCaptureVideo = false;
	m_edtChannel.SetWindowText(_T(""));
}

/*
	set up canvas and local video view.
*/
void CAgoraAVCaptureDlg::RenderLocalVideo()
{
}

/*
	Enumerate all the video capture devices and add to the combo box.
*/
void CAgoraAVCaptureDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);
	if (bShow) {
		//init control text.
		InitCtrlText();
		//update window.
		UpdateViews();
	}
	else {
		//resume window status.
		ResumeStatus();
	}
}

/*
	start or stop capture,register or unregister video frame observer. 
*/
void CAgoraAVCaptureDlg::InitVideoConfig()
{
	CString strInfo;
	m_cmbVideoType.GetWindowText(strInfo);

	int pos = strInfo.Find(_T("x"));
	videoConfig_.cx = _ttoi(strInfo.Mid(0, pos));
	int posH = strInfo.Find(_T(" "));
	videoConfig_.cy_abs = _ttoi(strInfo.Mid(pos + 1, posH - pos - 1));
	int posFPS = strInfo.Find(_T("fps"));
	videoConfig_.frameInterval = _ttoi(strInfo.Mid(posH+1, posFPS - posH - 1));
	videoConfig_.useDefaultConfig = false;
	videoConfig_.internalFormat = DShow::VideoFormat::MJPEG;
	videoFrame_.stride = videoConfig_.cx;
	videoFrame_.height = videoConfig_.cy_abs;
	videoFrame_.timestamp = 0;
	videoFrame_.format = agora::media::base::VIDEO_PIXEL_I420;
	if (videoFrame_.buffer) {
		delete[] videoFrame_.buffer;
		videoFrame_.buffer = nullptr;
	}
	videoFrame_.buffer = new uint8_t[videoFrame_.stride * videoFrame_.height * 3 / 2];
    
}

void CAgoraAVCaptureDlg::StartAudioFrame()
{
	if (audioFrame_.buffer) {
		delete[] audioFrame_.buffer;
		audioFrame_.buffer = nullptr;
	}
	audioFrame_.bytesPerSample    = agora::rtc::TWO_BYTES_PER_SAMPLE;
	audioFrame_.channels          = wasapiInput->GetAudioChannel();
	audioFrame_.samplesPerSec     = wasapiInput->GetAudioSampleRate();
	audioFrame_.samplesPerChannel = audioFrame_.samplesPerSec / 100 ;
	audioFrame_.buffer            = new BYTE[audioFrame_.samplesPerChannel * audioFrame_.channels * 2];//sizeof(16bit PCM)
	wasapiInput->SetRecordCaptureFunc(
		[this](LPBYTE data, int size) {
			memcpy(this->audioFrame_.buffer, data, size);
			this->customAudioTrack_->PushAudioFrame(this->audioFrame_);
		});
}

void CAgoraAVCaptureDlg::StopAudioFrame()
{
	wasapiInput->SetRecordCaptureFunc(nullptr);
}

void CAgoraAVCaptureDlg::OnClickedButtonStartCaputre()
{
	m_extenalCaptureVideo = !m_extenalCaptureVideo;
	if (m_extenalCaptureVideo)
	{
		if (m_cmbVideoType.GetCurSel() == -1)
		{
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("can not set vitrual video capture"));
			return;
		}
	
		m_btnSetExtCapture.SetWindowText(customVideoCaptureCtrlCancelExternlCapture);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("use extenal video frame observer sucess!"));
		InitVideoConfig();
		videoConfig_.callback =
			[this](const DShow::VideoConfig &config, unsigned char *data,
			size_t size, long long startTime, long long stopTime,
			long rotation) {
				libyuv::MJPGToI420(data, size,
					static_cast<uint8_t*>(videoFrame_.buffer), config.cx,
					static_cast<uint8_t*>(videoFrame_.buffer) + config.cx * config.cy_abs,
					config.cx / 2,
					static_cast<uint8_t*>(videoFrame_.buffer) + config.cx * config.cy_abs * 5 / 4,
					config.cx / 2,
					config.cx, config.cy_abs,
					config.cx, config.cy_abs);
			if(m_btnJoinChannel)
				customVideoTrack_->PushVideoFrame(this->videoFrame_);
		};
		DShow::VideoConfig config = videoConfig_;
		int size = 1024;
		uint8_t* data = nullptr;
		/**/
		int sel = m_cmbVideoDevice.GetCurSel();
		if (sel == -1) {
			AfxMessageBox(_T("no cameras"));
			return;
		}
		//videoConfig.name
		videoConfig_.name = videoDevices_[sel].name;
		videoConfig_.path = videoDevices_[sel].path;
		videoDevice_.SetVideoConfig(&videoConfig_);
		videoDevice_.ConnectFilters();
		videoDevice_.Start();
	}
	else {
		videoDevice_.Stop();
		m_btnSetExtCapture.SetWindowText(customVideoCaptureCtrlSetExternlCapture);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("restore video frame observer sucess!"));
	}
}

//The JoinChannel button's click handler.
//This function either joins or leaves the channel
void CAgoraAVCaptureDlg::OnClickedButtonJoinchannel()
{
	if (!m_initialize)
		return;
	CString strInfo;
	if (!m_joinChannel) {
		CString strSceneId;
		m_edtChannel.GetWindowText(strSceneId);
		if (strSceneId.IsEmpty()) {
			AfxMessageBox(_T("Fill scene id first"));
			return;
		}
		std::string szSceneId = cs2utf8(strSceneId);

		agora::rte::SceneConfig config;

		config.scene_type = bCompatible ? agora::rte::SceneType::kCompatible : agora::rte::SceneType::kAdhoc;
		m_scene = agora::rte::AgoraRteSDK::CreateRteScene(szSceneId, config);
		strInfo.Format(_T("AgoraRteSDK::CreateRteScene: %s"), strSceneId);

		m_lstInfo.InsertString(m_lstInfo.GetCount() - 1, strInfo);
		m_sceneEventHandler = std::make_shared<CAgoraAVCaptureDlgEngineEventHandler>();
		m_sceneEventHandler->SetMsgReceiver(m_hWnd);
		m_scene->RegisterEventHandler(m_sceneEventHandler);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("Scene RegisterEventHandler"));
		agora::rte::JoinOptions option;
		option.is_user_visible_to_remote = true ;

		OnClickedButtonStartCaputre();
		StartAudioFrame();
		//join channel in the engine.
		int ret = 0;
		if (0 == (ret = m_scene->Join(local_user_id_, APP_TOKEN, option))) {
			strInfo.Format(_T("uid %S"), local_user_id_.c_str());
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
			strInfo.Format(_T("join scene %s, use JoinOptions, %s")
				, strSceneId, option.is_user_visible_to_remote ? _T("Broadcaster") : _T("Audience"));
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
			
			agora::rte::RtcStreamOptions options = { STREAM_TOKEN };
			m_scene->CreateOrUpdateRTCStream(custom_audio_id, options);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateOrUpdateRTCStream custom audio"));

			m_scene->PublishLocalAudioTrack(custom_audio_id, customAudioTrack_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("PublishLocalAudioTrack custom audio track"));

			if (!bCompatible) {
				m_scene->CreateOrUpdateRTCStream(custom_video_id_, options);
				m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateOrUpdateRTCStream for custom Video"));
			}
		
			m_scene->PublishLocalVideoTrack(custom_video_id_, customVideoTrack_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("PublishLocalVideoTrack camera"));

			m_btnJoinChannel.EnableWindow(FALSE);
		}
		else {
			strInfo.Format(_T("join scene failed:, %d"), ret);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
		}
	}
	else {
		StopAudioFrame();
		OnClickedButtonStartCaputre();
		m_scene->UnpublishLocalAudioTrack(customAudioTrack_);
		m_scene->UnpublishLocalVideoTrack(customVideoTrack_);
		m_scene->Leave();
		strInfo.Format(_T("leave scene"));
		m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
		m_btnJoinChannel.EnableWindow(FALSE);
	}
}


//EID_JOINCHANNEL_SUCCESS message window handler.
LRESULT CAgoraAVCaptureDlg::OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam)
{
	m_joinChannel = true;
	m_btnJoinChannel.EnableWindow(TRUE);
	
	m_btnJoinChannel.SetWindowText(commonCtrlLeaveChannel);
	CString strInfo;
	strInfo.Format(_T("%s:join success, uid=%u"), getCurrentTime(), wParam);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	//m_localVideoWnd.SetUID(wParam);
	
	//notify parent window
	::PostMessage(GetParent()->GetSafeHwnd(), WM_MSGID(EID_JOINCHANNEL_SUCCESS), TRUE, 0);
	return 0;
}

//EID_LEAVE_CHANNEL message window handler.
LRESULT CAgoraAVCaptureDlg::OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam)
{
	m_joinChannel = false;
	m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);
	m_btnJoinChannel.EnableWindow(TRUE);
	CString strInfo;
	strInfo.Format(_T("leave channel success %s"), getCurrentTime());
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	::PostMessage(GetParent()->GetSafeHwnd(), WM_MSGID(EID_JOINCHANNEL_SUCCESS), FALSE, 0);
	return 0;
}

//EID_USER_JOINED message window handler.
LRESULT CAgoraAVCaptureDlg::OnEIDUserJoined(WPARAM wParam, LPARAM lParam)
{
	CString strInfo;
	strInfo.Format(_T("%u joined"), wParam);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	return 0;
}


//EID_USER_OFFLINE message window handler.
LRESULT CAgoraAVCaptureDlg::OnEIDUserOffline(WPARAM wParam, LPARAM lParam)
{
	
	return 0;
}

//EID_REMOTE_VIDEO_STATE_CHANED message window handler.
LRESULT CAgoraAVCaptureDlg::OnEIDRemoteVideoStateChanged(WPARAM wParam, LPARAM lParam)
{
	
	return 0;
}


LRESULT CAgoraAVCaptureDlg::OnEIDConnectionStateChanged(WPARAM wParam, LPARAM lParam)
{
	agora::rte::ConnectionState old_state = (agora::rte::ConnectionState)wParam;
	agora::rte::ConnectionState new_state = (agora::rte::ConnectionState)lParam;
	agora::rte::ConnectionChangedReason reason = (agora::rte::ConnectionChangedReason)lParam;
	if (reason == agora::rtc::CONNECTION_CHANGED_JOIN_SUCCESS) {
		if (!m_btnJoinChannel.IsWindowEnabled())
			OnEIDJoinChannelSuccess(wParam, lParam);
	}
	else if (reason == agora::rtc::CONNECTION_CHANGED_LEAVE_CHANNEL) {
		if (!m_btnJoinChannel.IsWindowEnabled())
			OnEIDLeaveChannel(wParam, lParam);
	}
	else {
		//if(reason != agora::rtc::CONNECTION_STATE_CONNECTING)
		//	m_btnJoinChannel.EnableWindow(TRUE);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), utf82cs(connectionStates[reason]));
	}

	return 0;
}

//Enumerates the video capture devices and types, 
//and inserts them into the ComboBox
void CAgoraAVCaptureDlg::OnSelchangeComboCaptureVideoDevice()
{
	m_cmbVideoType.Clear();
	auto caps = videoDevices_[0].caps;
	std::set<CString> capsSet;
	CString strInfo;

	for (int i = 0; i < caps.size(); ++i) {
		
		if(DShow::VideoFormat::MJPEG != caps[i].format)
			continue;
			
		int fps[2] = { 15, 30 };
		for (int index = 0; index < 2; index++) {
			strInfo.Format(_T("%dx%d %dfps")
				, caps[i].maxCX, caps[i].minCY, fps[index]);
			if (capsSet.find(strInfo) == capsSet.end()) {
				capsSet.insert(strInfo);
				m_cmbVideoType.InsertString(m_cmbVideoType.GetCount(), strInfo);
			}
		}
		capsSet.insert(strInfo);
		m_cmbVideoType.SetCurSel(0);
	}
}

BOOL CAgoraAVCaptureDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN&&pMsg->wParam==VK_RETURN) {
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CAgoraAVCaptureDlgEngineEventHandler::OnLocalStreamStats(const std::string& stream_id, const agora::rte::LocalStreamStats& stats)
{
	if (m_hMsgHanlder) {
		PLocalStats stat = new LocalStats;
		stat->strmid = stream_id;
		stat->stats = stats;
		if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LOCAL_STREAM_STATS), (WPARAM)stat, 0);

	}
}

void CAgoraAVCaptureDlgEngineEventHandler::OnRemoteStreamStats(const std::string& stream_id, const agora::rte::RemoteStreamStats& stats)
{
	if (m_hMsgHanlder) {
		PRemoteStats stat = new RemoteStats;
		stat->strmid = stream_id;
		stat->stats = stats;
		if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_REMOTE_STREAM_STATS), (WPARAM)stat, 0);

	}
}

void CAgoraAVCaptureDlgEngineEventHandler::OnConnectionStateChanged(agora::rte::ConnectionState old_state, agora::rte::ConnectionState new_state,
	agora::rte::ConnectionChangedReason reason)
{

	if (m_hMsgHanlder)
		::PostMessage(m_hMsgHanlder, WM_MSGID(EID_CONNECTION_STATE), (int)old_state, (int)reason);

}