#include "stdafx.h"
#include "APIExample.h"
#include "CAgoraMutilVideoSourceDlg.h"
#include <iostream>


IMPLEMENT_DYNAMIC(CAgoraMutilVideoSourceDlg, CDialogEx)

CAgoraMutilVideoSourceDlg::CAgoraMutilVideoSourceDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MUTI_SOURCE, pParent)
{

}

CAgoraMutilVideoSourceDlg::~CAgoraMutilVideoSourceDlg()
{
}

void CAgoraMutilVideoSourceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_VIDEO, m_staVideoArea);
	DDX_Control(pDX, IDC_LIST_INFO_BROADCASTING, m_lstInfo);
	DDX_Control(pDX, IDC_STATIC_CHANNELNAME, m_staChannel);
	DDX_Control(pDX, IDC_EDIT_CHANNELNAME, m_edtChannel);
	DDX_Control(pDX, IDC_BUTTON_JOINCHANNEL, m_btnJoinChannel);
	DDX_Control(pDX, IDC_BUTTON_PUBLISH, m_btnPublish);
	DDX_Control(pDX, IDC_STATIC_DETAIL, m_staDetail);
}


BEGIN_MESSAGE_MAP(CAgoraMutilVideoSourceDlg, CDialogEx)
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_MSGID(EID_CONNECTION_STATE), &CAgoraMutilVideoSourceDlg::OnEIDConnectionStateChanged)

	ON_LBN_SELCHANGE(IDC_LIST_INFO_BROADCASTING, &CAgoraMutilVideoSourceDlg::OnSelchangeListInfoBroadcasting)
	ON_MESSAGE(WM_MSGID(EID_JOINCHANNEL_SUCCESS), &CAgoraMutilVideoSourceDlg::OnEIDJoinChannelSuccess)
	ON_MESSAGE(WM_MSGID(EID_LEAVE_CHANNEL), &CAgoraMutilVideoSourceDlg::OnEIDLeaveChannel)
	ON_MESSAGE(WM_MSGID(EID_USER_JOINED), &CAgoraMutilVideoSourceDlg::OnEIDUserJoined)
	ON_MESSAGE(WM_MSGID(EID_USER_OFFLINE), &CAgoraMutilVideoSourceDlg::OnEIDUserOffline)
	ON_MESSAGE(WM_MSGID(EID_LOCAL_VIDEO_STATE_CHANGED), &CAgoraMutilVideoSourceDlg::OnEIDLocalVideoStateChanged)
	ON_BN_CLICKED(IDC_BUTTON_JOINCHANNEL, &CAgoraMutilVideoSourceDlg::OnBnClickedButtonJoinchannel)
	ON_BN_CLICKED(IDC_BUTTON_PUBLISH, &CAgoraMutilVideoSourceDlg::OnBnClickedButtonPublish)
END_MESSAGE_MAP()


//Initialize the Ctrl Text.
void CAgoraMutilVideoSourceDlg::InitCtrlText()
{

	m_btnPublish.SetWindowText(MultiVideoSourceCtrlPublish);//MultiVideoSourceCtrlUnPublish
	m_staChannel.SetWindowText(commonCtrlChannel);
	m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);
}


//Initialize the Agora SDK
bool CAgoraMutilVideoSourceDlg::InitAgora()
{
	agora::rte::AgoraRteLogger::EnableLogging(true);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteLogger::EnableLogging"));
	agora::rte::AgoraRteLogger::SetLevel(agora::rte::LogLevel::Verbose);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteLogger::SetLevel Verbose"));
	agora::rte::AgoraRteLogger::SetListener([](const std::string& message) { std::cout << message; });

	agora::rte::SdkProfile profile;
	profile.appid = GET_APP_ID;
	agora::rte::AgoraRteSDK::Init(profile, bCompatible);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteSDK::Init"));
	media_control_ = agora::rte::AgoraRteSDK::GetRteMediaFactory();
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteSDK::GetRteMediaFactory"));
	camera_track_ = media_control_->CreateCameraVideoTrack();
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateCameraVideoTrack"));
	microphone_ = media_control_->CreateMicrophoneAudioTrack();
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateMicrophoneAudioTrack"));
	m_initialize = true;

	if (bCompatible) {
		local_user_id_ = GenerateUserId();
		local_screen_id_ = GenerateUserId();
		local_camera_id_ = GenerateUserId();

	}
	else {
		local_user_id_ = GenerateRandomString(local_user_id_, id_random_len);
		local_screen_id_ = GenerateRandomString(local_screen_id_, id_random_len);
		local_camera_id_ = GenerateRandomString(local_camera_id_, id_random_len);
	}
	local_microphone_id_ = local_user_id_;
	
	return true;
}


//UnInitialize the Agora SDK
void CAgoraMutilVideoSourceDlg::UnInitAgora()
{
	if (m_scene) {
		camera_track_->StopCapture();
		microphone_->StopRecording();
		screen_track_->StopCapture();
		agora::rte::AgoraRteSDK::Deinit();
	}
}

//render local video from SDK local capture.
void CAgoraMutilVideoSourceDlg::RenderLocalVideo()
{
	if (camera_track_) {
		agora::rte::VideoCanvas canvas;
		canvas.renderMode = agora::media::base::RENDER_MODE_FIT;
		canvas.uid = 0;
		canvas.view = m_videoWnds[0].GetSafeHwnd();
		
		camera_track_->SetPreviewCanvas(canvas);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CameraTrack SetPreviewCanvas"));

		agora::rte::CameraCallbackFun cameraCallback = [this](agora::rte::CameraState state, agora::rte::CameraSource source) {
			::PostMessage(this->m_hWnd, WM_MSGID(EID_CAMERA_STATE_CHANED), (WPARAM)state, (LPARAM)source);
		};

		screen_track_ = media_control_->CreateScreenVideoTrack();
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateScreenVideoTrack"));

		camera_track_->StartCapture(cameraCallback);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CameraTrack StartCapture"));

		agora::rte::VideoCanvas canvasScreen;
		canvasScreen.renderMode = agora::media::base::RENDER_MODE_FIT;
		canvasScreen.uid = 0;
		canvasScreen.sourceType = agora::rtc::VIDEO_SOURCE_SCREEN_PRIMARY;
		canvasScreen.view = m_videoWnds[1].GetSafeHwnd();
		canvasScreen.mirrorMode = agora::rtc::VIDEO_MIRROR_MODE_DISABLED;
		screen_track_->SetPreviewCanvas(canvasScreen);

		agora::rte::Rectangle rc;

		HWND hWnd = ::GetDesktopWindow();
		RECT destop_rc;
		::GetWindowRect(hWnd, &destop_rc);
		
		screen_track_->StartCaptureScreen(rc, rc);
	}
}


//resume window status
void CAgoraMutilVideoSourceDlg::ResumeStatus()
{
	InitCtrlText();
	m_joinChannel = false;
	m_initialize = false;
	m_bPublishScreen = false;
	m_btnJoinChannel.EnableWindow(TRUE);
}


void CAgoraMutilVideoSourceDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);
	if (bShow) {
		//init control text.
		InitCtrlText();
		//update window.
		RenderLocalVideo();
	}
	else {
		//resume window status.
		ResumeStatus();
	}
}


BOOL CAgoraMutilVideoSourceDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

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

	RECT rcArea;
	m_staVideoArea.GetClientRect(&rcArea);
	RECT leftArea = rcArea;
	leftArea.right = (rcArea.right - rcArea.left) / 2;
	RECT rightArea = rcArea;
	rightArea.left = (rcArea.right - rcArea.left) / 2;
	
	for (int i = 0; i < this->VIDOE_COUNT; ++i) {
		m_videoWnds[i].Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CRect(0, 0, 1, 1), this, i);
		//set window background color.
		m_videoWnds[i].SetFaceColor(RGB(0x58, 0x58, 0x58));
	}
	m_videoWnds[0].MoveWindow(&leftArea);
	m_videoWnds[1].MoveWindow(&rightArea);
	m_btnPublish.ShowWindow(SW_HIDE);
	//camera screen
	ResumeStatus();
	return TRUE;
}


BOOL CAgoraMutilVideoSourceDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CAgoraMutilVideoSourceDlg::StartDesktopShare()
{
	/*agora::rtc::Rectangle rc;
	ScreenCaptureParameters scp;
	scp.frameRate = 15;
	scp.bitrate = 0;
	HWND hWnd = ::GetDesktopWindow();
	RECT destop_rc;
	::GetWindowRect(hWnd, &destop_rc);
	scp.dimensions.width = destop_rc.right - destop_rc.left;
	scp.dimensions.height = destop_rc.bottom - destop_rc.top;
	m_rtcEngine->startScreenCaptureByScreenRect(rc, rc, scp);*/
}

void CAgoraMutilVideoSourceDlg::OnBnClickedButtonJoinchannel()
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
		m_sceneEventHandler = std::make_shared<CMultiVideoSourceRteSceneEventHandler>();
		m_sceneEventHandler->SetMsgReceiver(m_hWnd);
		m_scene->RegisterEventHandler(m_sceneEventHandler);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("Scene RegisterEventHandler"));
		agora::rte::JoinOptions option;
		option.is_user_visible_to_remote = true;	
		
		//join channel in the engine.
		int ret = 0;
		if (0 == (ret = m_scene->Join(local_user_id_, APP_TOKEN, option))) {
			strInfo.Format(_T("uid %S"), local_user_id_.c_str());
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
			strInfo.Format(_T("join scene %s, use JoinOptions, %s")
				, strSceneId, option.is_user_visible_to_remote ? _T("Broadcaster") : _T("Audience"));
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);

			microphone_->StartRecording();
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("StartRecording"));
			agora::rte::RtcStreamOptions options = { STREAM_TOKEN };
			m_scene->CreateOrUpdateRTCStream(local_microphone_id_, options);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateOrUpdateRTCStream microphone"));

			m_scene->PublishLocalAudioTrack(local_microphone_id_, microphone_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("PublishLocalAudioTrack microphone"));

			if (!bCompatible) {//if compatible use same stream with microphone
				m_scene->CreateOrUpdateRTCStream(local_camera_id_, options);
				m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateOrUpdateRTCStream camera"));
			}
			m_scene->PublishLocalVideoTrack(local_camera_id_, camera_track_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("PublishLocalVideoTrack camera"));
			m_lstInfo.InsertString(m_lstInfo.GetCount(), utf82cs(local_camera_id_));
			if (!bCompatible) {
				m_scene->CreateOrUpdateRTCStream(local_screen_id_, options);
				m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateOrUpdateRTCStream screen"));
			}
			m_scene->PublishLocalVideoTrack(local_screen_id_, screen_track_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("PublishLocalVideoTrack screen"));
			m_lstInfo.InsertString(m_lstInfo.GetCount(), utf82cs(local_screen_id_));

			m_btnJoinChannel.EnableWindow(FALSE);
		}
		else {
			strInfo.Format(_T("join scene failed:, %d"), ret);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
		}
	}
	else {
		m_scene->UnpublishLocalVideoTrack(camera_track_);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("UnpublishLocalVideoTrack camera"));

		m_scene->UnpublishLocalVideoTrack(screen_track_);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("UnpublishLocalVideoTrack screen"));

		m_scene->UnpublishLocalAudioTrack(microphone_);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("UnpublishLocalAudioTrack microphone"));

		m_scene->Leave();
		strInfo.Format(_T("leave scene"));
		m_lstInfo.InsertString(m_lstInfo.GetCount() - 1, strInfo);
		m_btnJoinChannel.EnableWindow(FALSE);
	}
}


void CAgoraMutilVideoSourceDlg::OnBnClickedButtonPublish()
{
	m_bPublishScreen = !m_bPublishScreen;
}

//EID_JOINCHANNEL_SUCCESS message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam)
{
	m_btnJoinChannel.EnableWindow(TRUE);
	m_joinChannel = true;
	m_btnJoinChannel.SetWindowText(commonCtrlLeaveChannel);
	CString strInfo;
	strInfo.Format(_T("%s:connected"), getCurrentTime());
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	
	//notify parent window
	::PostMessage(GetParent()->GetSafeHwnd(), WM_MSGID(EID_JOINCHANNEL_SUCCESS), TRUE, 0);
	return 0;
}

//EID_LEAVE_CHANNEL message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam)
{
	 m_btnJoinChannel.EnableWindow(TRUE);
	m_joinChannel = false;
    m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);

    CString strInfo;
    strInfo.Format(_T("disconnected:%s"), getCurrentTime());
    m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);

	return 0;
}

//EID_USER_JOINED message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDUserJoined(WPARAM wParam, LPARAM lParam)
{
	//CString strInfo;
	//strInfo.Format(_T("%u joined %s"), wParam, strChannelName);
	///m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	return 0;
}


//EID_USER_OFFLINE message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDUserOffline(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

//EID_REMOTE_VIDEO_STATE_CHANED message window handler.
LRESULT CAgoraMutilVideoSourceDlg::OnEIDLocalVideoStateChanged(WPARAM wParam, LPARAM lParam)
{
	RemoteVideoStateChanged* videoState = (RemoteVideoStateChanged*)wParam;

	CString strInfo;
	strInfo.Format(_T("%s %s %s"), utf82cs(videoState->streams.stream_id), videoState->media_type == agora::rte::MediaType::kAudio ? _T("Audio") : _T("Video")
	, videoState->reason == agora::rte::StreamStateChangedReason::kPublished ? _T("published") : _T("unplublished"));

	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	if (videoState) {
		delete videoState;
		videoState = nullptr;
	}
	return 0;
}


LRESULT CAgoraMutilVideoSourceDlg::OnEIDConnectionStateChanged(WPARAM wParam, LPARAM lParam)
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
		if (reason != agora::rtc::CONNECTION_CHANGED_CONNECTING)
			m_btnJoinChannel.EnableWindow(TRUE);
	   
		m_lstInfo.InsertString(m_lstInfo.GetCount(), utf82cs(connectionStates[reason]));
	}

	return 0;
}

void CAgoraMutilVideoSourceDlg::OnSelchangeListInfoBroadcasting()
{
	int sel = m_lstInfo.GetCurSel();
	if (sel < 0)return;
	CString strDetail;
	m_lstInfo.GetText(sel, strDetail);
	m_staDetail.SetWindowText(strDetail);
}
