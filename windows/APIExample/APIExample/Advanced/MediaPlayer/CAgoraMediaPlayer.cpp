#include "stdafx.h"
#include "APIExample.h"
#include "CAgoraMediaPlayer.h"
#include <iostream>

IMPLEMENT_DYNAMIC(CAgoraMediaPlayer, CDialogEx)

CAgoraMediaPlayer::CAgoraMediaPlayer(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MEDIA_PLAYER, pParent)
{

}

CAgoraMediaPlayer::~CAgoraMediaPlayer()
{
}

void CAgoraMediaPlayer::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_VIDEO, m_staVideoArea);
	DDX_Control(pDX, IDC_LIST_INFO_BROADCASTING, m_lstInfo);
	DDX_Control(pDX, IDC_STATIC_DETAIL, m_staDetail);
	DDX_Control(pDX, IDC_STATIC_CHANNELNAME, m_staChannel);
	DDX_Control(pDX, IDC_EDIT_CHANNELNAME, m_edtChannel);
	DDX_Control(pDX, IDC_BUTTON_JOINCHANNEL, m_btnJoinChannel);
	DDX_Control(pDX, IDC_STATIC_VIDEO_SOURCE, m_staVideoSource);
	DDX_Control(pDX, IDC_EDIT_VIDEO_SOURCE, m_edtVideoSource);
	DDX_Control(pDX, IDC_BUTTON_OPEN, m_btnOpen);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_btnStop);
	DDX_Control(pDX, IDC_BUTTON_PLAY, m_btnPlay);

	DDX_Control(pDX, IDC_BUTTON_PUBLISH_VIDEO, m_btnPublishVideo);
	
	DDX_Control(pDX, IDC_SLIDER_VIDEO, m_sldVideo);
}


//Initialize the Ctrl Text.
void CAgoraMediaPlayer::InitCtrlText()
{
	m_staVideoSource.SetWindowText(mediaPlayerCtrlVideoSource);
	m_btnPlay.SetWindowText(mediaPlayerCtrlPlay);
	m_btnOpen.SetWindowText(mediaPlayerCtrlOpen);
	m_btnStop.SetWindowText(mediaPlayerCtrlClose);
	m_btnPublishVideo.SetWindowText(mediaPlayerCtrlPublishVideo);
	m_staChannel.SetWindowText(commonCtrlChannel);
	m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);
}

//Initialize media player.
void CAgoraMediaPlayer::InitMediaPlayerKit()
{
	//create agora media player.
	//m_mediaPlayer = m_rtcEngine->createMediaPlayer().get();//createAgoraMediaPlayer();
	//m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("createAgoraMediaPlayer"));
	//agora::rtc::MediaPlayerContext context;
	//initialize media player context.
	//agora::base::IAgoraService* agoraService;
	//int ret = m_mediaPlayer->initialize(agoraService);
	//set message notify receiver window
	//m_mediaPlayerEnvet.SetMsgReceiver(m_hWnd);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("mediaplayer initialize"));
	//set message notify receiver window
	
	//register player event observer.
	//ret = m_mediaPlayer->registerPlayerSourceObserver(&m_mediaPlayerEnvet);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("registerPlayerSourceObserver"));
}


//Uninitialized media player .
void CAgoraMediaPlayer::UnInitMediaPlayerKit()
{
	/*if (m_mediaPlayer)
	{
		//call media player release function.
		//m_mediaPlayer->release();
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("release mediaPlayer"));
		m_mediaPlayer = nullptr;
	}*/
}

//Initialize the Agora SDK
bool CAgoraMediaPlayer::InitAgora()
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
	media_player_ = media_control_->CreateMediaPlayer();
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateMediaPlayer"));
	
	player_observer_ = std::make_shared<MediaPlayerObserver>(media_player_);
	player_observer_->SetMsgReceiver(m_hWnd);
	media_player_->RegisterMediaPlayerObserver(player_observer_);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("RegisterMediaPlayerObserver"));

	m_initialize = true;

	//set show window handle.
	RECT rc = { 0 };
	m_localVideoWnd.GetWindowRect(&rc);
	int ret = media_player_->SetView((agora::media::base::view_t)m_localVideoWnd.GetSafeHwnd());


	if (bCompatible) {
		//local_user_id_ = GenerateUserId();
		//local_screen_id_ = GenerateUserId();
		//local_camera_id_ = GenerateUserId();

	}

	else {
		//local_user_id_ = GenerateRandomString(local_user_id_, id_random_len);
		//local_screen_id_ = GenerateRandomString(local_screen_id_, id_random_len);
		//local_camera_id_ = GenerateRandomString(local_camera_id_, id_random_len);
	}
	//local_microphone_id_ = local_user_id_;


	return true;
}


//UnInitialize the Agora SDK
void CAgoraMediaPlayer::UnInitAgora()
{
	if (m_joinChannel){
		//leave channel
		m_joinChannel = !m_joinChannel;
		if (m_scene) {
			m_scene->UnpublishMediaPlayer(media_player_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("UnpublishMediaPlayer"));

			m_scene->Leave();
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("Leave Scene"));
		}
		
		agora::rte::AgoraRteSDK::Deinit();
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("Deinit rte SDK"));
	}

}


//resume window status
void CAgoraMediaPlayer::ResumeStatus()
{
	InitCtrlText();
	m_staDetail.SetWindowText(_T(""));
	m_edtChannel.SetWindowText(_T(""));
	m_edtVideoSource.SetWindowText(_T(""));
	m_lstInfo.ResetContent();
	m_sldVideo.SetPos(0);

	m_btnPublishVideo.EnableWindow(FALSE);
	
	m_btnPlay.EnableWindow(FALSE);
	m_mediaPlayerState = mediaPLAYER_READY;
	m_joinChannel = false;
	m_initialize = false;
	m_attach = false;
}

BEGIN_MESSAGE_MAP(CAgoraMediaPlayer, CDialogEx)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_BUTTON_JOINCHANNEL, &CAgoraMediaPlayer::OnBnClickedButtonJoinchannel)
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CAgoraMediaPlayer::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CAgoraMediaPlayer::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CAgoraMediaPlayer::OnBnClickedButtonPlay)

	ON_BN_CLICKED(IDC_BUTTON_PUBLISH_VIDEO, &CAgoraMediaPlayer::OnBnClickedButtonPublishVideo)
	
	ON_LBN_SELCHANGE(IDC_LIST_INFO_BROADCASTING, &CAgoraMediaPlayer::OnSelchangeListInfoBroadcasting)
	ON_MESSAGE(WM_MSGID(EID_CONNECTION_STATE), &CAgoraMediaPlayer::OnEIDConnectionStateChanged)

	ON_MESSAGE(WM_MSGID(mediaPLAYER_STATE_CHANGED), &CAgoraMediaPlayer::OnmediaPlayerStateChanged)
	ON_MESSAGE(WM_MSGID(mediaPLAYER_POSTION_CHANGED), &CAgoraMediaPlayer::OnmediaPlayerPositionChanged)

	ON_MESSAGE(WM_MSGID(EID_JOINCHANNEL_SUCCESS), &CAgoraMediaPlayer::OnEIDJoinChannelSuccess)
	ON_MESSAGE(WM_MSGID(EID_LEAVE_CHANNEL), &CAgoraMediaPlayer::OnEIDLeaveChannel)
	ON_MESSAGE(WM_MSGID(EID_USER_JOINED), &CAgoraMediaPlayer::OnEIDUserJoined)
	ON_MESSAGE(WM_MSGID(EID_USER_OFFLINE), &CAgoraMediaPlayer::OnEIDUserOffline)

	ON_WM_DESTROY()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_VIDEO, &CAgoraMediaPlayer::OnReleasedcaptureSliderVideo)
END_MESSAGE_MAP()


//WM_SHOWWINDOW	message handler.
void CAgoraMediaPlayer::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);
	if (bShow)//bShwo is true ,show window 
	{
		InitCtrlText();
	}
	else {
		ResumeStatus();
	}

}

//InitDialog handler.
BOOL CAgoraMediaPlayer::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_localVideoWnd.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CRect(0, 0, 1, 1), this, ID_BASEWND_VIDEO + 100);
	RECT rcArea;
	m_staVideoArea.GetClientRect(&rcArea);
	m_localVideoWnd.MoveWindow(&rcArea);
	m_localVideoWnd.ShowWindow(SW_SHOW);
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

	if (bCompatible) {
		local_user_id_ = GenerateUserId();
	
	}
	else {
		local_user_id_ = GenerateRandomString(local_user_id_, id_random_len);
	}
	return TRUE;
}

//join channel handler.
void CAgoraMediaPlayer::OnBnClickedButtonJoinchannel()
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
		m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
		m_sceneEventHandler = std::make_shared<CAgoraMediaPlayerHandler>();
		m_sceneEventHandler->SetMsgReceiver(m_hWnd);
		m_scene->RegisterEventHandler(m_sceneEventHandler);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("Scene RegisterEventHandler"));
		agora::rte::JoinOptions option;
		option.is_user_visible_to_remote = true;

		//join channel in the engine.
		int ret = 0;
		if (0 == (ret = m_scene->Join(local_user_id_, APP_TOKEN, option))) {
			agora::rte::RtcStreamOptions pub_option;
			pub_option.type = agora::rte::StreamType::kRtcStream;
			m_scene->CreateOrUpdateRTCStream(local_user_id_, pub_option);
			
			m_scene->PublishMediaPlayer(local_user_id_, media_player_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("UnpublishMediaPlayer"));

			strInfo.Format(_T("uid %S"), local_user_id_.c_str());
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
			strInfo.Format(_T("join scene %s, use JoinOptions, %s")
				, strSceneId, option.is_user_visible_to_remote ? _T("Broadcaster") : _T("Audience"));
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
			m_btnJoinChannel.EnableWindow(FALSE);
		}
		else {
			strInfo.Format(_T("join scene failed:, %d"), ret);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
		}
	}
	else {
		
		m_scene->Leave();
		strInfo.Format(_T("leave scene"));
		m_lstInfo.InsertString(m_lstInfo.GetCount() - 1, strInfo);
		m_btnJoinChannel.EnableWindow(FALSE);
	}
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
}

//open button click handler.
void CAgoraMediaPlayer::OnBnClickedButtonOpen()
{
	CString strUrl;
	CString strInfo;
	m_edtVideoSource.GetWindowText(strUrl);
	std::string tmp = cs2utf8(strUrl);
	switch (m_mediaPlayerState)
	{
	case mediaPLAYER_READY:
	case mediaPLAYER_STOP:
	{
		if (tmp.empty())
		{
			AfxMessageBox(_T("you can fill video source."));
			return;
		}
		//call media player open function
		int ret = media_player_->Open(tmp.c_str(), 0);
		CString strInfo;
		strInfo.Format(_T("media player: %d"), ret);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	}
		break;
	default:
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("can not open player."));
		break;
	}
}

//stop button click handler. 
void CAgoraMediaPlayer::OnBnClickedButtonStop()
{
	if (m_mediaPlayerState == mediaPLAYER_OPEN ||
		m_mediaPlayerState == mediaPLAYER_PLAYING ||
		m_mediaPlayerState == mediaPLAYER_PAUSE)
	{
		//call media player stop function
		media_player_->Stop();
		m_mediaPlayerState = mediaPLAYER_STOP;
		m_btnPlay.SetWindowText(mediaPlayerCtrlPlay);
		m_btnPlay.EnableWindow(FALSE);
		//set slider current position.
		m_sldVideo.SetPos(0);
	}
	else {
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("can not stop player"));
	}
}

// play button click handler.
void CAgoraMediaPlayer::OnBnClickedButtonPlay()
{
	int ret = -1;
	switch (m_mediaPlayerState)
	{
	case mediaPLAYER_PAUSE:
	case mediaPLAYER_OPEN:
		//call media player play function
		ret = media_player_->Play();
		if (ret == 0)
		{
			m_mediaPlayerState = mediaPLAYER_PLAYING;
			m_btnPlay.SetWindowText(mediaPlayerCtrlPause);
		}
		break;
	case mediaPLAYER_PLAYING:
		//call media player pause function
		ret = media_player_->Pause();
		if (ret == 0)
		{
			m_mediaPlayerState = mediaPLAYER_PAUSE;
			m_btnPlay.SetWindowText(mediaPlayerCtrlPlay);
		}
		break;
	default:
		break;
	}
}

//push video button click handler.
void CAgoraMediaPlayer::OnBnClickedButtonPublishVideo()
{
	if (m_publishMeidaplayer) {
		
		m_publishMeidaplayer = false;
	}
	else {
		
		m_publishMeidaplayer = true;
	}
}


//show notify information
void CAgoraMediaPlayer::OnSelchangeListInfoBroadcasting()
{
	int sel = m_lstInfo.GetCurSel();
	if (sel < 0)return;
	CString strDetail;
	m_lstInfo.GetText(sel, strDetail);
	m_staDetail.SetWindowText(strDetail);
}

// intercept enter key
BOOL CAgoraMediaPlayer::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


//media player state changed handler
LRESULT CAgoraMediaPlayer::OnmediaPlayerStateChanged(WPARAM wParam, LPARAM lParam)
{
	CString strState;
	CString strError;
	switch ((agora::media::base::MEDIA_PLAYER_STATE)wParam)
	{
	case  agora::media::base::PLAYER_STATE_OPEN_COMPLETED:
		strState = _T("PLAYER_STATE_OPEN_COMPLETED");
		m_mediaPlayerState = mediaPLAYER_OPEN;
		m_btnPlay.EnableWindow(TRUE);
		int64_t duration;
		media_player_->GetDuration(duration);
		m_sldVideo.SetRangeMax((int)duration);

		break;
	case  agora::media::base::PLAYER_STATE_OPENING:
		strState = _T("PLAYER_STATE_OPENING");
		break;
	case  agora::media::base::PLAYER_STATE_IDLE:
		strState = _T("PLAYER_STATE_IDLE");
		break;
	case  agora::media::base::PLAYER_STATE_PLAYING:
		strState = _T("PLAYER_STATE_PLAYING");
		break;
	case agora::media::base::PLAYER_STATE_PLAYBACK_COMPLETED:
		strState = _T("PLAYER_STATE_PLAYBACK_COMPLETED");
		break;
	case agora::media::base::PLAYER_STATE_PAUSED:
		strState = _T("PLAYER_STATE_PAUSED");
		break;
	case agora::media::base::PLAYER_STATE_STOPPED:
		strState = _T("PLAYER_STATE_PAUSED");
		break;
	case agora::media::base::PLAYER_STATE_FAILED:
		strState = _T("PLAYER_STATE_FAILED");
		//call media player stop function
		media_player_->Stop();
		break;
	default:
		strState = _T("PLAYER_STATE_UNKNOWN");
		break;
	}

	switch ((agora::media::base::MEDIA_PLAYER_ERROR)lParam)
	{
	case agora::media::base::PLAYER_ERROR_URL_NOT_FOUND:
		strError = _T("PLAYER_ERROR_URL_NOT_FOUND");
		break;
	case agora::media::base::PLAYER_ERROR_NONE:
		strError = _T("PLAYER_ERROR_NONE");
		break;
	case agora::media::base::PLAYER_ERROR_CODEC_NOT_SUPPORTED:
		strError = _T("PLAYER_ERROR_NONE");
		break;
	case agora::media::base::PLAYER_ERROR_INVALID_ARGUMENTS:
		strError = _T("PLAYER_ERROR_INVALID_ARGUMENTS");
		break;
	case agora::media::base::PLAYER_ERROR_SRC_BUFFER_UNDERFLOW:
		strError = _T("PLAY_ERROR_SRC_BUFFER_UNDERFLOW");
		break;
	case agora::media::base::PLAYER_ERROR_INTERNAL:
		strError = _T("PLAYER_ERROR_INTERNAL");
		break;
	case agora::media::base::PLAYER_ERROR_INVALID_STATE:
		strError = _T("PLAYER_ERROR_INVALID_STATE");
		break;
	case agora::media::base::PLAYER_ERROR_NO_RESOURCE:
		strError = _T("PLAYER_ERROR_NO_RESOURCE");
		break;
	case agora::media::base::PLAYER_ERROR_OBJ_NOT_INITIALIZED:
		strError = _T("PLAYER_ERROR_OBJ_NOT_INITIALIZED");
		break;
	case agora::media::base::PLAYER_ERROR_INVALID_CONNECTION_STATE:
		strError = _T("PLAYER_ERROR_INVALID_CONNECTION_STATE");
		break;
	case agora::media::base::PLAYER_ERROR_UNKNOWN_STREAM_TYPE:
		strError = _T("PLAYER_ERROR_UNKNOWN_STREAM_TYPE");
		break;
	case agora::media::base::PLAYER_ERROR_VIDEO_RENDER_FAILED:
		strError = _T("PLAYER_ERROR_VIDEO_RENDER_FAILED");
		break;
	}
	CString strInfo;
	strInfo.Format(_T("sta:%s,\nerr:%s"), strState, strError);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	return TRUE;
}

LRESULT CAgoraMediaPlayer::OnmediaPlayerPositionChanged(WPARAM wParam, LPARAM lParam)
{
	int64_t * p = (int64_t*)wParam;
	m_sldVideo.SetPos((int)*p);
	delete p;
	return TRUE;
}

//EID_JOINCHANNEL_SUCCESS message window handler
LRESULT CAgoraMediaPlayer::OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam)
{
	m_joinChannel = true;
	m_btnJoinChannel.SetWindowText(commonCtrlLeaveChannel);
	m_btnJoinChannel.EnableWindow(TRUE);
	CString strInfo;
	strInfo.Format(_T("%s:join success, uid=%u"), getCurrentTime(), wParam);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);

    m_btnPublishVideo.EnableWindow(TRUE);
	//notify parent window
	return 0;
}

//EID_LEAVEHANNEL_SUCCESS message window handler
LRESULT CAgoraMediaPlayer::OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam)
{
	m_joinChannel = false;
	m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);
	CString strInfo;
	strInfo.Format(_T("leave channel success %s"), getCurrentTime());
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	::PostMessage(GetParent()->GetSafeHwnd(), WM_MSGID(EID_JOINCHANNEL_SUCCESS), FALSE, 0);
	return 0;
}

//EID_USER_JOINED message window handler
LRESULT CAgoraMediaPlayer::OnEIDUserJoined(WPARAM wParam, LPARAM lParam)
{
	CString strInfo;
	strInfo.Format(_T("%u joined"), wParam);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	return 0;
}

//EID_USER_OFFLINE message handler.
LRESULT CAgoraMediaPlayer::OnEIDUserOffline(WPARAM wParam, LPARAM lParam)
{

	return 0;
}


LRESULT CAgoraMediaPlayer::OnEIDConnectionStateChanged(WPARAM wParam, LPARAM lParam)
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

void CAgoraMediaPlayer::OnDestroy()
{
	CDialogEx::OnDestroy();
	if (media_player_) {
		media_player_->UnregisterMediaPlayerObserver(player_observer_);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("UnregisterMediaPlayerObserver"));
		media_player_.reset();
	}
}


//drag events
void CAgoraMediaPlayer::OnReleasedcaptureSliderVideo(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	int pos = m_sldVideo.GetPos();
	media_player_->Seek(pos);
	*pResult = 0;
}
