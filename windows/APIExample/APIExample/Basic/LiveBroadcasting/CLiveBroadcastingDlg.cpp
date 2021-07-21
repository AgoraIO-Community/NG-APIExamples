// CLiveBroadcastingDlg.cpp : implementation file

#include "stdafx.h"
#include "APIExample.h"
#include "AgoraRteSdk.hpp"
#include "CLiveBroadcastingDlg.h"
//#include "CAgoraReportInCallDlg.h"
#include <iostream>
/*
note:
    Join the channel callback.This callback method indicates that the client
    successfully joined the specified channel.Channel ids are assigned based
    on the channel name specified in the joinChannel. If IRtcEngine::joinChannel
    is called without a user ID specified. The server will automatically assign one
parameters:
    channel:channel name.
    uid: user ID¡£If the UID is specified in the joinChannel, that ID is returned here;
    Otherwise, use the ID automatically assigned by the Agora server.
    elapsed: The Time from the joinChannel until this event occurred (ms).
*/
/*void CLiveBroadcastingRtcEngineEventHandler::onJoinChannelSuccess(const char* channel, uid_t uid, int elapsed)
{
    if (m_hMsgHanlder) {
        ::PostMessage(m_hMsgHanlder, WM_MSGID(EID_JOINCHANNEL_SUCCESS), (WPARAM)uid, (LPARAM)elapsed);
    }
}*/

/*
note:
    In the live broadcast scene, each anchor can receive the callback
    of the new anchor joining the channel, and can obtain the uID of the anchor.
    Viewers also receive a callback when a new anchor joins the channel and
    get the anchor's UID.When the Web side joins the live channel, the SDK will
    default to the Web side as long as there is a push stream on the
    Web side and trigger the callback.
parameters:
    uid: remote user/anchor ID for newly added channel.
    elapsed: The joinChannel is called from the local user to the delay triggered
    by the callback£¨ms).
*/
/*void CLiveBroadcastingRtcEngineEventHandler::onUserJoined(uid_t uid, int elapsed) {
    if (m_hMsgHanlder) {
        ::PostMessage(m_hMsgHanlder, WM_MSGID(EID_USER_JOINED), (WPARAM)uid, (LPARAM)elapsed);
    }
}*/

/*
note:
    Remote user (communication scenario)/anchor (live scenario) is called back from
    the current channel.A remote user/anchor has left the channel (or dropped the line).
    There are two reasons for users to leave the channel, namely normal departure and
    time-out:When leaving normally, the remote user/anchor will send a message like
    "goodbye". After receiving this message, determine if the user left the channel.
    The basis of timeout dropout is that within a certain period of time
    (live broadcast scene has a slight delay), if the user does not receive any
    packet from the other side, it will be judged as the other side dropout.
    False positives are possible when the network is poor. We recommend using the
    Agora Real-time messaging SDK for reliable drop detection.
parameters:
    uid: The user ID of an offline user or anchor.
    reason:Offline reason: USER_OFFLINE_REASON_TYPE.
*/
/*void CLiveBroadcastingRtcEngineEventHandler::onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason)
{
    if (m_hMsgHanlder) {
        ::PostMessage(m_hMsgHanlder, WM_MSGID(EID_USER_OFFLINE), (WPARAM)uid, (LPARAM)reason);
    }
}*/

/*
note:
    When the App calls the leaveChannel method, the SDK indicates that the App
    has successfully left the channel. In this callback method, the App can get
    the total call time, the data traffic sent and received by THE SDK and other
    information. The App obtains the call duration and data statistics received
    or sent by the SDK through this callback.
parametes:
    stats: Call statistics.
*/
/*void CLiveBroadcastingRtcEngineEventHandler::onLeaveChannel(const RtcStats& stats)
{
    if (m_hMsgHanlder) {
        ::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LEAVE_CHANNEL), 0, 0);
    }
}*/

// CLiveBroadcastingDlg dialog
IMPLEMENT_DYNAMIC(CLiveBroadcastingDlg, CDialogEx)

CLiveBroadcastingDlg::CLiveBroadcastingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LIVEBROADCASTING, pParent)
{

}

CLiveBroadcastingDlg::~CLiveBroadcastingDlg()
{
}

void CLiveBroadcastingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_ROLE, m_cmbRole);
	DDX_Control(pDX, IDC_STATIC_ROLE, m_staRole);
	DDX_Control(pDX, IDC_EDIT_CHANNELNAME, m_edtChannelName);
	DDX_Control(pDX, IDC_BUTTON_JOINCHANNEL, m_btnJoinChannel);
	DDX_Control(pDX, IDC_LIST_INFO_BROADCASTING, m_lstInfo);
	DDX_Control(pDX, IDC_STATIC_VIDEO, m_videoArea);
	DDX_Control(pDX, IDC_COMBO_PERSONS, m_cmbPersons);
	DDX_Control(pDX, IDC_STATIC_PERSONS, m_staPersons);
	DDX_Control(pDX, IDC_STATIC_CHANNELNAME, m_staChannelName);
	DDX_Control(pDX, IDC_STATIC_DETAIL, m_staDetail);
	DDX_Control(pDX, IDC_COMBO_ENCODER, m_cmbVideoEncoder);
	DDX_Control(pDX, IDC_STATIC_WIDTH, m_staWidth);
	DDX_Control(pDX, IDC_EDIT_WIDTH, m_edtWidth);
	DDX_Control(pDX, IDC_STATIC_HEIGHT, m_staHeight);
	DDX_Control(pDX, IDC_EDIT_HEIGHT, m_edtHeight);
	DDX_Control(pDX, IDC_STATIC_LIVE_FPS, m_staFPS);
	DDX_Control(pDX, IDC_EDIT_LIVE_FPS, m_edtFPS);
	DDX_Control(pDX, IDC_STATIC_BITRATE, m_staBitrate);
	DDX_Control(pDX, IDC_COMBO_BITRATE, m_cmbBitrate);
	DDX_Control(pDX, IDC_COMBO_AUDIO_PROFILE, m_cmbAudioProfile);
	DDX_Control(pDX, IDC_STATIC_AUDIO_ENCODER2, m_staAudioEncoder);
	DDX_Control(pDX, IDC_COMBO_AUDIO_SAMPLERATE, m_cmbSampleRate);
	DDX_Control(pDX, IDC_CHECK_STEREO, m_chkStereo);
	DDX_Control(pDX, IDC_STATIC_SAMPLERATE, m_staSampleRate);
	DDX_Control(pDX, IDC_STATIC_VIDEO_DEVICE, m_staVideoDevice);
	DDX_Control(pDX, IDC_COMBO_CAMERA_LIST, m_cmbCameraList);
	DDX_Control(pDX, IDC_COMBO_MIC_LIST, m_cmbMicList);
	DDX_Control(pDX, IDC_COMBO_SPEAKER_LIST, m_cmbSpeakerList);
	DDX_Control(pDX, IDC_STATIC_LIVE_MICROPHONE, m_staMicrophone);
	DDX_Control(pDX, IDC_STATIC_LIVE_SPK, m_staSpk);
	DDX_Control(pDX, IDC_SLIDER_MIC, m_sldMic);
	DDX_Control(pDX, IDC_SLIDER_SPEAKER, m_sldSpeaker);
	DDX_Control(pDX, IDC_CHECK_REPORT_INCALL, m_chkReportInCall);
}


BEGIN_MESSAGE_MAP(CLiveBroadcastingDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_JOINCHANNEL, &CLiveBroadcastingDlg::OnBnClickedButtonJoinchannel)
    ON_CBN_SELCHANGE(IDC_COMBO_PERSONS, &CLiveBroadcastingDlg::OnSelchangeComboPersons)
    ON_CBN_SELCHANGE(IDC_COMBO_ROLE, &CLiveBroadcastingDlg::OnSelchangeComboRole)
    ON_MESSAGE(WM_MSGID(EID_JOINCHANNEL_SUCCESS), &CLiveBroadcastingDlg::OnEIDJoinChannelSuccess)
    ON_MESSAGE(WM_MSGID(EID_LEAVE_CHANNEL), &CLiveBroadcastingDlg::OnEIDLeaveChannel)
    ON_MESSAGE(WM_MSGID(EID_USER_JOINED), &CLiveBroadcastingDlg::OnEIDUserJoined)
	ON_MESSAGE(WM_MSGID(EID_REMOTE_VIDEOSTREAM_ADD), &CLiveBroadcastingDlg::OnRemoteStreamAdd)
	
	ON_MESSAGE(WM_MSGID(EID_REMOTE_VIDEOSTREAM_REMOVE), &CLiveBroadcastingDlg::OnRemoteStreamRemove)
    ON_MESSAGE(WM_MSGID(EID_USER_OFFLINE), &CLiveBroadcastingDlg::OnEIDUserOffline)
	ON_MESSAGE(WM_MSGID(EID_CONNECTION_STATE), &CLiveBroadcastingDlg::OnEIDConnectionStateChanged)


	ON_MESSAGE(WM_MSGID(EID_LOCAL_STREAM_STATS), &CLiveBroadcastingDlg::OnEIDLocalStreamStats)
	ON_MESSAGE(WM_MSGID(EID_REMOTE_STREAM_STATS), &CLiveBroadcastingDlg::OnEIDRemoteStreamStats)

    ON_WM_SHOWWINDOW()
    ON_LBN_SELCHANGE(IDC_LIST_INFO_BROADCASTING, &CLiveBroadcastingDlg::OnSelchangeListInfoBroadcasting)
    ON_STN_CLICKED(IDC_STATIC_VIDEO, &CLiveBroadcastingDlg::OnStnClickedStaticVideo)
	ON_CBN_SELCHANGE(IDC_COMBO_CAMERA_LIST, &CLiveBroadcastingDlg::OnSelchangeComboCameraList)
	ON_CBN_SELCHANGE(IDC_COMBO_MIC_LIST, &CLiveBroadcastingDlg::OnSelchangeComboMicList)
	ON_CBN_SELCHANGE(IDC_COMBO_SPEAKER_LIST, &CLiveBroadcastingDlg::OnSelchangeComboSpeakerList)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_MIC, &CLiveBroadcastingDlg::OnCustomdrawSliderMic)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_SPEAKER, &CLiveBroadcastingDlg::OnCustomdrawSliderSpeaker)
	ON_BN_CLICKED(IDC_CHECK_REPORT_INCALL, &CLiveBroadcastingDlg::OnClickedCheckReportIncall)
	ON_MESSAGE(WM_MSGID(EID_REMOTE_VIDEO_STATE_CHANGED), &CLiveBroadcastingDlg::OnEIDRemoteVideoStateChanged)

END_MESSAGE_MAP()


// CLiveBroadcastingDlg message handlers
BOOL CLiveBroadcastingDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    CreateAllVideoWnds();
    // clientRole
    int i = 0;
    m_cmbRole.InsertString(i++, agoraRoleBroadcaster);
    m_cmbRole.InsertString(i++, agoraRoleAudience);
    //
    i = 0;
    m_cmbPersons.InsertString(i++, _T("1V1"));
    m_cmbPersons.InsertString(i++, _T("1V3"));
    m_cmbPersons.InsertString(i++, _T("1V8"));
    m_cmbPersons.InsertString(i++, _T("1V15"));

	i = 0;
	m_cmbVideoEncoder.InsertString(i++, _T("VP8"));
	m_cmbVideoEncoder.InsertString(i++, _T("h264"));
	m_cmbVideoEncoder.InsertString(i++, _T("h265"));
	m_cmbVideoEncoder.InsertString(i++, _T("VP9"));
	m_cmbVideoEncoder.InsertString(i++, _T("Generic"));
	m_cmbVideoEncoder.InsertString(i++, _T("Generic H264"));

	i = 0;
	m_cmbBitrate.InsertString(i++, _T("Standard"));
	m_cmbBitrate.InsertString(i++, _T("Compatible"));
	m_cmbBitrate.InsertString(i++, _T("Default"));
	m_cmbBitrate.SetCurSel(0);

	i = 0;
	m_cmbAudioProfile.InsertString(i++, _T("Default"));
	m_cmbAudioProfile.InsertString(i++, _T("SPEECH_STANDARD"));
	m_cmbAudioProfile.InsertString(i++, _T("STANDARD_STEREO"));
	m_cmbAudioProfile.InsertString(i++, _T("HIGH_QUALITY"));
	m_cmbAudioProfile.InsertString(i++, _T("HIGH_QUALITY_STEREO"));
	m_cmbAudioProfile.InsertString(i++, _T("IOT"));
	m_cmbAudioProfile.InsertString(i++, _T("NUM"));
	m_cmbAudioProfile.SetCurSel(0);

	bitrates_.push_back(1);
	bitrates_.push_back(-1);
	bitrates_.push_back(-1);

	ResumeStatus();
	m_cmbSampleRate.ShowWindow(SW_HIDE);
	m_chkStereo.ShowWindow(SW_HIDE);
	m_staSampleRate.ShowWindow(SW_HIDE);
	m_sldMic.SetRange(0, 255);
	m_sldSpeaker.SetRange(0, 255);

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

	m_edtWidth.SetWindowText(_T("640"));
	m_edtHeight.SetWindowText(_T("480"));
	m_edtFPS.SetWindowText(_T("15"));
	
	coedec_types_.push_back(agora::rtc::VIDEO_CODEC_VP8);
	coedec_types_.push_back(agora::rtc::VIDEO_CODEC_H264);
	coedec_types_.push_back(agora::rtc::VIDEO_CODEC_H265);
	coedec_types_.push_back(agora::rtc::VIDEO_CODEC_VP9);
	coedec_types_.push_back(agora::rtc::VIDEO_CODEC_GENERIC);
	coedec_types_.push_back(agora::rtc::VIDEO_CODEC_GENERIC_H264);
	coedec_types_.push_back(agora::rtc::VIDEO_CODEC_GENERIC_JPEG);

    return TRUE;
}


//set control text from config.
void CLiveBroadcastingDlg::InitCtrlText()
{
    m_staRole.SetWindowText(commonCtrlClientRole);
    m_staPersons.SetWindowText(liveCtrlPersons);
    m_staChannelName.SetWindowText(commonCtrlChannel);
    m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);
}

//create all video window to save m_videoWnds.
void CLiveBroadcastingDlg::CreateAllVideoWnds()
{
    for (int i = 0; i < VIDEO_COUNT; ++i) {
        m_videoWnds[i].Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CRect(0, 0, 1, 1), this, IDC_BASEWND_VIDEO + i);
        //set window background color.
        m_videoWnds[i].SetFaceColor(RGB(0x58, 0x58, 0x58));
		m_videoWnds[i].ShowVideoInfo(FALSE);
    }
}

//show all video window from m_videoWnds.
void CLiveBroadcastingDlg::ShowVideoWnds()
{
    m_videoArea.ShowWindow(SW_HIDE);
    int row = 2;
    int col = 2;
    m_maxVideoCount = 4;
    switch (m_cmbPersons.GetCurSel()) {
    case PEOPLE_IN_CHANNEL_2: {
        row = 1;
        col = 2;
        m_maxVideoCount = 2;
    }
        break;
    case PEOPLE_IN_CHANNEL_4: {
        int row = 2;
        int col = 2;
        m_maxVideoCount = 4;
    }
        break;
    case PEOPLE_IN_CHANNEL_9: {
        row = 3;
        col = 3;
        m_maxVideoCount = 9;
    }
        break;
    case PEOPLE_IN_CHANNEL_16: {
        row = 4;
        col = 4;
        m_maxVideoCount = 16;
    }
        break;
    }
    

    RECT rcArea;
    m_videoArea.GetClientRect( &rcArea);
    int space = 1;
    
    int w = (rcArea.right -rcArea.left - space * (col - 1)) / col;
    int h = (rcArea.bottom - rcArea.top - space * (row - 1)) / row;
    
    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            int x = rcArea.left + (w + space) * c;
            int y = rcArea.top + (h + space) * r;
            int nIndex = r * col + c;
            m_videoWnds[nIndex].MoveWindow(x, y, w, h, TRUE);
            m_videoWnds[nIndex].ShowWindow(SW_SHOW);
            m_videoWnds[nIndex].SetParent(this);

            if (!m_videoWnds[nIndex].IsWindowVisible()) {
                m_videoWnds[nIndex].ShowWindow(SW_SHOW);
            }
        }
    }

    for (int i = m_maxVideoCount; i < VIDEO_COUNT; i++) {
        m_videoWnds[i].ShowWindow(0);
        if (!m_videoWnds[i].GetUID().empty()) {
            agora::rte::VideoCanvas canvas;
            //canvas.uid = m_videoWnds[i].GetUID();
            canvas.view = m_videoWnds[i].GetSafeHwnd();
            //m_rtcEngine->setupRemoteVideo(canvas);
			m_scene->SetRemoteVideoCanvas(m_videoWnds[i].GetStrmId(), canvas);
        }
    }
}
//Initialize the Agora SDK
bool CLiveBroadcastingDlg::InitAgora()
{
	agora::rte::AgoraRteLogger::EnableLogging(true);
	m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("AgoraRteLogger::EnableLogging"));
	agora::rte::AgoraRteLogger::SetLevel(agora::rte::LogLevel::Verbose);
	m_lstInfo.InsertString(m_lstInfo.GetCount() , _T("AgoraRteLogger::SetLevel Verbose"));
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

	if (bCompatible)
		local_user_id_ = GenerateUserId();

	else
		local_user_id_ = GenerateRandomString(local_user_id_, id_random_len);

	local_microphone_id_ = local_user_id_;
	local_camera_id_ = local_user_id_;
	
	m_cmbCameraList.Clear();
	m_cmbMicList.Clear();
	m_cmbSpeakerList.Clear();

	auto cameraManager = agora::rte::AgoraRteSDK::GetVideoDeviceManager();
	cameraLists_ = cameraManager->EnumerateVideoDevices();
	for (int i = 0; i < cameraLists_.size(); ++i) {
		m_cmbCameraList.InsertString(i, utf82cs(cameraLists_[i].device_name));
	}
	m_cmbCameraList.SetCurSel(0);

	audioManager_ = agora::rte::AgoraRteSDK::GetAudioDeviceManager();
	mics_ = audioManager_->EnumerateRecordingDevices();
	for (int i = 0; i < mics_->GetCount(); ++i) {
		char szName[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 }, szId[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 };
		mics_->GetDevice(i, szName, szId);
		m_cmbMicList.InsertString(i, utf82cs(szName));
	}
	
	if (m_cmbMicList.GetCount() > 0) {
		m_cmbMicList.SetCurSel(0);
		int vol = 0;
		audioManager_->GetRecordingDeviceVolume(&vol);
		m_sldMic.SetPos(vol);
	}

	speakers_ = audioManager_->EnumeratePlaybackDevices();
	for (int i = 0; i < speakers_->GetCount(); ++i) {
		char szName[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 }, szId[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 };
		speakers_->GetDevice(i, szName, szId);
		m_cmbSpeakerList.InsertString(i, utf82cs(szName));
		
	}

	if(m_cmbSpeakerList.GetCount() > 0) {
		m_cmbSpeakerList.SetCurSel(0);
		int vol = 0;
		audioManager_->GetPlaybackDeviceVolume(&vol);
		m_sldSpeaker.SetPos(vol);
	}
    return true;
}

//UnInitialize the Agora SDK
void CLiveBroadcastingDlg::UnInitAgora()
{
	if (m_scene) {
		
		agora::rte::AgoraRteSDK::Deinit();
	}
}

void CLiveBroadcastingDlg::ResumeStatus()
{
	m_lstInfo.ResetContent();
	m_cmbRole.SetCurSel(0);
	m_cmbPersons.SetCurSel(0);
	m_cmbVideoEncoder.SetCurSel(1);
	ShowVideoWnds();
	InitCtrlText();
	
	m_btnJoinChannel.EnableWindow(TRUE);
	m_cmbRole.EnableWindow(TRUE);
	m_edtChannelName.SetWindowText(_T(""));
	m_edtWidth.SetWindowText(_T(""));
	m_edtHeight.SetWindowText(_T(""));
	m_edtFPS.SetWindowText(_T(""));
	m_cmbBitrate.SetCurSel(0);
	m_cmbVideoEncoder.SetCurSel(1);
	
	m_joinChannel = false;
	m_initialize = false;
}

//render local video from SDK local capture.
void CLiveBroadcastingDlg::RenderLocalVideo()
{
	if (camera_track_) {
		agora::rte::VideoCanvas canvas;
		canvas.renderMode = agora::media::base::RENDER_MODE_FIT;
		canvas.uid = 0;
		canvas.view = m_videoWnds[0].GetSafeHwnd();
		
		camera_track_->SetPreviewCanvas(canvas);
		m_lstInfo.InsertString(m_lstInfo.GetCount() - 1, _T("CameraTrack SetPreviewCanvas"));
		
		agora::rte::CameraCallbackFun cameraCallback = [this](agora::rte::CameraState state, agora::rte::CameraSource) {
			//::PostMessage(this->m_hWnd, 0, 0, 0);
		};
		camera_track_->StartCapture(cameraCallback);
		m_lstInfo.InsertString(m_lstInfo.GetCount() - 1, _T("CameraTrack StartCapture"));
	}
}


void CLiveBroadcastingDlg::OnSelchangeComboPersons()
{
    int index = m_cmbPersons.GetCurSel();
    ShowVideoWnds();
}


void CLiveBroadcastingDlg::OnSelchangeComboRole()
{
    /*if (m_rtcEngine) {
        m_rtcEngine->setClientRole(CLIENT_ROLE_TYPE(m_cmbRole.GetCurSel() + 1));

        m_lstInfo.InsertString(m_lstInfo.GetCount(), m_cmbRole.GetCurSel() == 0 ? _T("setClientRole broadcaster"): _T("setClientRole Audience"));
    }*/
}

void CLiveBroadcastingDlg::OnBnClickedButtonJoinchannel()
{
    if ( !m_initialize)
        return;
    CString strInfo;
    if (!m_joinChannel) {
        CString strSceneId;
        m_edtChannelName.GetWindowText(strSceneId);
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
		m_sceneEventHandler = std::make_shared<CLiveBroadcastingRtcEngineEventHandler>();
		m_sceneEventHandler->SetMsgReceiver(m_hWnd);
		m_scene->RegisterEventHandler(m_sceneEventHandler);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("Scene RegisterEventHandler"));
		agora::rte::JoinOptions option;
		option.is_user_visible_to_remote = m_cmbRole.GetCurSel() == 0 ? true : false;
		//option.autoSubscribeAudio = true;
		//option.autoSubscribeVideo = true;

		CString str;
		m_edtWidth.GetWindowText(str);
		int w = _ttoi(str.GetBuffer(0));
		if (w == 0) {
			w = 640;
			m_edtWidth.SetWindowText(_T("640"));
		}

		m_edtHeight.GetWindowText(str);
		int h = _ttoi(str.GetBuffer(0));
		if (h == 0) {
			h = 480;
			m_edtHeight.SetWindowText(_T("480"));
		}

		m_edtFPS.GetWindowText(str);
		int fps = _ttoi(str.GetBuffer(0));
		if (fps == 0) {
			h = 15;
			m_edtFPS.SetWindowText(_T("15"));
		}

		videoEcnoder_.dimensions.width = w;
		videoEcnoder_.dimensions.height = h;
		videoEcnoder_.frameRate = fps;
		videoEcnoder_.bitrate = bitrates_[m_cmbBitrate.GetCurSel()];
		videoEcnoder_.codecType = coedec_types_[m_cmbVideoEncoder.GetCurSel()];

		audioEncoder_.audioProfile = (agora::rtc::AUDIO_PROFILE_TYPE)m_cmbAudioProfile.GetCurSel();
        //join channel in the engine.
		int ret = 0;
		if (0 ==  (ret = m_scene->Join(local_user_id_, APP_TOKEN, option))) {
			strInfo.Format(_T("uid %S"), local_user_id_.c_str());
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
            strInfo.Format(_T("join scene %s, use JoinOptions, %s")
				, strSceneId, option.is_user_visible_to_remote? _T("Broadcaster") : _T("Audience"));
			m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
			
			microphone_->StartRecording();
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("StartRecording"));
			agora::rte::RtcStreamOptions options = { STREAM_TOKEN };
			m_scene->CreateOrUpdateRTCStream(local_microphone_id_, options);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateOrUpdateRTCStream microphone"));

			m_scene->SetAudioEncoderConfiguration(local_microphone_id_, audioEncoder_);

			m_scene->PublishLocalAudioTrack(local_microphone_id_, microphone_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("PublishLocalAudioTrack microphone"));

			if (!bCompatible) {//if compatible use same stream with microphone
				m_scene->CreateOrUpdateRTCStream(local_camera_id_, options);
				m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("CreateOrUpdateRTCStream camera"));
			}
			m_scene->SetVideoEncoderConfiguration(local_camera_id_, videoEcnoder_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("SetVideoEncoderConfiguration"));
			
			m_scene->PublishLocalVideoTrack(local_camera_id_, camera_track_);
			m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("PublishLocalVideoTrack camera"));
			
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

		m_scene->UnpublishLocalAudioTrack(microphone_);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("UnpublishLocalAudioTrack microphone"));

		m_scene->Leave();
		strInfo.Format(_T("leave scene"));
		m_lstInfo.InsertString(m_lstInfo.GetCount() - 1, strInfo);
		m_btnJoinChannel.EnableWindow(FALSE);
	}
}

void CLiveBroadcastingDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CDialogEx::OnShowWindow(bShow, nStatus);

    if (bShow) {
        RenderLocalVideo();
	}
	else
	{
		ResumeStatus();
	}
}

LRESULT CLiveBroadcastingDlg::OnEIDRemoteVideoStateChanged(WPARAM wParam, LPARAM lParam)
{

	RemoteVideoStateChanged* videoState = (RemoteVideoStateChanged*)wParam;

	if (videoState->media_type == agora::rte::MediaType::kVideo
		&&videoState->reason == agora::rte::StreamStateChangedReason::kSubscribed) {
		m_strmIds.insert(videoState->streams.stream_id);
		auto localStreams = m_scene->GetLocalStreams();

		int streamCount = 0;
		for (int i = 1; i < m_maxVideoCount; i++) {
			if (m_videoWnds[i].GetUID().empty()) {


				agora::rte::VideoCanvas canvas;

				canvas.view = m_videoWnds[i].GetSafeHwnd();
				canvas.renderMode = agora::media::base::RENDER_MODE_FIT;
				//setup remote video in engine to the canvas.
				m_scene->SetRemoteVideoCanvas(videoState->streams.stream_id, canvas);

				m_lstInfo.InsertString(m_lstInfo.GetCount(), _T("SetRemoteVideoCanvas "));
				m_lstInfo.InsertString(m_lstInfo.GetCount(), utf82cs(videoState->streams.stream_id));
				m_videoWnds[i].SetUID(videoState->streams.stream_id);
				m_videoWnds[i].SetLocalFlag(false);
				streamCount++;
				break;
			}
		}

	}


	if (videoState) {
		delete videoState;
		videoState = nullptr;
	}
	return 0;
}

LRESULT CLiveBroadcastingDlg::OnEIDConnectionStateChanged(WPARAM wParam, LPARAM lParam)
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

LRESULT CLiveBroadcastingDlg::OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam)
{
    m_btnJoinChannel.EnableWindow(TRUE);
	m_joinChannel = true;
    m_btnJoinChannel.SetWindowText(commonCtrlLeaveChannel);
    CString strInfo;
    strInfo.Format(_T("%s:connected"), getCurrentTime());
    m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	std::vector<agora::rte::StreamInfo>  strmInfos = m_scene->GetLocalStreams();
	for (int i = 0; i < strmInfos.size(); ++i) {
		m_videoWnds[i].SetLocalFlag(true);
		m_videoWnds[i].SetUID(strmInfos[i].user_id);
		m_videoWnds[i].SetStrmId(strmInfos[i].stream_id);
		m_strmIds.insert(strmInfos[i].stream_id);
	}
   
    //notify parent window
    ::PostMessage(GetParent()->GetSafeHwnd(), WM_MSGID(EID_JOINCHANNEL_SUCCESS), TRUE, 0);
    return 0;
}

LRESULT CLiveBroadcastingDlg::OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam)
{
    m_btnJoinChannel.EnableWindow(TRUE);
	m_joinChannel = false;
    m_btnJoinChannel.SetWindowText(commonCtrlJoinChannel);

    CString strInfo;
    strInfo.Format(_T("disconnected:%s"), getCurrentTime());
    m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	m_strmIds.clear();
    for (int i = 0; i < m_maxVideoCount; i++) {
        m_videoWnds[i].SetUID("");
		m_videoWnds[i].SetStrmId("");
		m_videoWnds[i].SetLocalFlag(true);
    }

    //notify parent window
    ::PostMessage(GetParent()->GetSafeHwnd(), WM_MSGID(EID_JOINCHANNEL_SUCCESS), FALSE, 0);
    return 0;
}

LRESULT CLiveBroadcastingDlg::OnEIDUserJoined(WPARAM wParam, LPARAM lParam)
{
    if (m_strmIds.size() == m_maxVideoCount)
        return 0;
	agora::rte::UserInfo* userInfos = (agora::rte::UserInfo*)wParam;
	int count = lParam;
	for (int i = 0; i < count; ++i) {
		CString strInfo;
		strInfo.Format(_T("%u joined"), userInfos[0].user_id);
		m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);
	}
    //m_lstUids.push_back(wParam);
    /*for (int i = 1; i < m_maxVideoCount; i++) {
        if (m_videoWnds[i].GetUID() == 0) {

			agora::rte::VideoCanvas canvas;
			canvas.renderMode = agora::media::base::RENDER_MODE_FIT;
			canvas.uid = 0;
			canvas.view = m_videoWnds[0].GetSafeHwnd();
			camera_track_->SetPreviewCanvas(canvas);
			m_lstInfo.InsertString(m_lstInfo.GetCount() - 1, _T("CameraTrack SetPreviewCanvas"));
			
		   VideoCanvas canvas;
            canvas.uid  = wParam;
            canvas.view = m_videoWnds[i].GetSafeHwnd();
            canvas.renderMode = media::base::RENDER_MODE_FIT;
            //setup remote video in engine to the canvas.
            m_rtcEngine->setupRemoteVideo(canvas);
			m_videoWnds[i].SetUID(wParam);
            break;
        }
    }*/

	if (userInfos) {
		delete[] userInfos;
		userInfos = nullptr;
	}
    return 0;
}

LRESULT CLiveBroadcastingDlg::OnRemoteStreamAdd(WPARAM wParam, LPARAM lParam)
{
	agora::rte::StreamInfo* infos = (agora::rte::StreamInfo*)wParam;
	int count = lParam;
	for (int i = 0; i < count; ++i) {
		m_scene->SubscribeRemoteAudio(infos[i].stream_id);
		agora::rte::VideoSubscribeOptions options;
		m_scene->SubscribeRemoteVideo(infos[i].stream_id, options);
		
	}

	delete[] infos;
	infos = nullptr;
	return 0;
}

LRESULT CLiveBroadcastingDlg::OnRemoteStreamRemove(WPARAM wParam, LPARAM lParam)
{
	agora::rte::StreamInfo* infos = (agora::rte::StreamInfo*)wParam;
	int count = lParam;
	std::unordered_set<std::string> strmids;
	for (int i = 0; i < count; ++i) {
		strmids.insert(infos[i].stream_id);
	}
	for (int i = 1; i < m_maxVideoCount; i++) {
		std::string strmid = m_videoWnds[i].GetUID();

		if (strmids.find(strmid) != strmids.end()) {
			m_videoWnds[i].SetUID("");
			m_videoWnds[i].SetLocalFlag(true);
			m_videoWnds[i].SetStrmId("");
			m_videoWnds[i].Invalidate();
			agora::rte::VideoCanvas canvas;
			canvas.view = nullptr;
			m_scene->SetRemoteVideoCanvas(strmid, canvas);
			break;
		}
	}

	for (int i = 0; i < count; ++i) {
		m_strmIds.erase(infos[i].stream_id);
	}

	delete[] infos;
	infos = nullptr;
	return 0;
}

LRESULT CLiveBroadcastingDlg::OnEIDUserOffline(WPARAM wParam, LPARAM lParam)
{
   /* uid_t remoteUid = (uid_t)wParam;
    VideoCanvas canvas;
    canvas.uid = remoteUid;
    canvas.view = NULL;
    m_rtcEngine->setupRemoteVideo(canvas);
    CString strInfo;
    strInfo.Format(_T("%u offline, reason:%d"), remoteUid, lParam);
    m_lstInfo.InsertString(m_lstInfo.GetCount(), strInfo);

    for (int i = 1; i < m_maxVideoCount; i++){
        if (m_videoWnds[i].GetUID() == remoteUid) {
            m_videoWnds[i].SetUID(0);
            m_videoWnds[i].Invalidate();
            break;
        }
    }

    for (auto iter = m_lstUids.begin();
        iter != m_lstUids.end(); iter++){
        if (*iter == remoteUid) {
            m_lstUids.erase(iter);
            break;
        }
    }*/
    return 0;
}


void CLiveBroadcastingDlg::OnSelchangeListInfoBroadcasting()
{
    int sel = m_lstInfo.GetCurSel();
	if (sel < 0)return;
    CString strDetail;
    m_lstInfo.GetText(sel, strDetail);
    m_staDetail.SetWindowText(strDetail);
}


void CLiveBroadcastingDlg::OnStnClickedStaticVideo()
{
}


BOOL CLiveBroadcastingDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CLiveBroadcastingRtcEngineEventHandler::OnLocalStreamStats(const std::string& stream_id, const agora::rte::LocalStreamStats& stats) 
{
	if (m_hMsgHanlder) {
		PLocalStats stat = new LocalStats;
		stat->strmid = stream_id;
		stat->stats = stats;
		if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LOCAL_STREAM_STATS), (WPARAM)stat, 0);

	}
}

void CLiveBroadcastingRtcEngineEventHandler::OnRemoteStreamStats(const std::string& stream_id, const agora::rte::RemoteStreamStats& stats) 
{
	if (m_hMsgHanlder) {
		PRemoteStats stat = new RemoteStats;
		stat->strmid = stream_id;
		stat->stats = stats;
		if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_REMOTE_STREAM_STATS), (WPARAM)stat, 0);

	}
}

void CLiveBroadcastingRtcEngineEventHandler::OnConnectionStateChanged(agora::rte::ConnectionState old_state, agora::rte::ConnectionState new_state,
	agora::rte::ConnectionChangedReason reason)
{

	if (m_hMsgHanlder)
		::PostMessage(m_hMsgHanlder, WM_MSGID(EID_CONNECTION_STATE), (int)old_state, (int)reason);
	/*switch (new_state) {
	case agora::rte::ConnectionState::kDisconnecting:
		
		break;
	case agora::rte::ConnectionState::kConnecting:
		break;
	case agora::rte::ConnectionState::kConnected:
		//connect_success_sempahore.Notify();
		break;
	case agora::rte::ConnectionState::kReconnecting:
		break;
	case agora::rte::ConnectionState::kFailed:
	case agora::rte::ConnectionState::kDisconnected:
		//connect_gone_sempahore.Notify();
		break;
	default:
		break;
	}*/
}


void CLiveBroadcastingDlg::OnSelchangeComboCameraList()
{
	int sel = m_cmbAudioProfile.GetCurSel();
	
	camera_track_->StopCapture();
	camera_track_.reset();
	camera_track_ = media_control_->CreateCameraVideoTrack();
	OutputDebugStringA(cameraLists_[sel].device_id.c_str());
	OutputDebugStringA("\n");
	camera_track_->SetCameraDevice(cameraLists_[sel].device_id);
	RenderLocalVideo();
}


void CLiveBroadcastingDlg::OnSelchangeComboMicList()
{
	// TODO: Add your control notification handler code here
	int sel = m_cmbMicList.GetCurSel();
	auto audioManager = agora::rte::AgoraRteSDK::GetAudioDeviceManager();
	char szName[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 };
	char szId[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 };
	mics_->GetDevice(sel, szName, szId);

	m_lstInfo.InsertString(m_lstInfo.GetCount() , _T("SetRecordingDevice"));
	m_lstInfo.InsertString(m_lstInfo.GetCount(), utf82cs(szName));
	audioManager->SetRecordingDevice(szId);

	int vol = 0;
	audioManager->GetRecordingDeviceVolume(&vol);
	m_sldMic.SetPos(vol);
	CString strVol;
	strVol.Format(_T("%d"), vol);
	m_sldMic.GetToolTips()->SetWindowText(strVol);
	
}

void CLiveBroadcastingDlg::OnSelchangeComboSpeakerList()
{
	// TODO: Add your control notification handler code here
	//auto audioManager = agora::rte::AgoraRteSDK::GetAudioDeviceManager();
	int sel = m_cmbSpeakerList.GetCurSel();
	char szName[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 };
	char szId[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 };

	speakers_->GetDevice(sel, szName, szId);
	audioManager_->SetPlaybackDevice(szId);
	OutputDebugStringA(szId);
	char sz[agora::rte::MAX_DEVICE_ID_LENGTH] = { 0 };
	OutputDebugStringA(sz);
	int vol = 0;
	audioManager_->GetPlaybackDeviceVolume(&vol);
	m_sldSpeaker.SetPos(vol);
}


void CLiveBroadcastingDlg::OnCustomdrawSliderMic(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	audioManager_->SetRecordingDeviceVolume(m_sldMic.GetPos());
	std::string s;
	s.reserve();
}


void CLiveBroadcastingDlg::OnCustomdrawSliderSpeaker(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	audioManager_->SetPlaybackDeviceVolume(m_sldSpeaker.GetPos());
}



LRESULT  CLiveBroadcastingDlg::OnEIDLocalStreamStats(WPARAM wParam, LPARAM lParam)
{
	PLocalStats stats = (PLocalStats)wParam;
	if (!m_chkReportInCall.GetCheck()) {
		for (int i = 0; i < m_strmIds.size(); ++i) {
			if (m_videoWnds[i].GetStrmId().compare(stats->strmid) == 0) {
				m_videoWnds[i].SetLocalStats(*stats);
				break;
			}
		}

	}
	delete stats;
	stats = nullptr;
	return 0;
}

LRESULT  CLiveBroadcastingDlg::OnEIDRemoteStreamStats(WPARAM wParam, LPARAM lParam)
{
	PRemoteStats stats = (PRemoteStats)wParam;
	if (!m_chkReportInCall.GetCheck()) {
		for (int i = 0; i < m_strmIds.size(); ++i) {
			if (m_videoWnds[i].GetStrmId().compare(stats->strmid) == 0) {
				m_videoWnds[i].SetRemoteStats(*stats);
				break;
			}
		}
	}
	delete stats;
	stats = nullptr;
	return 0;
}

void CLiveBroadcastingDlg::OnClickedCheckReportIncall()
{
	for (int i = 0; i < m_strmIds.size(); ++i) {
		m_videoWnds[i].ShowVideoInfo(m_chkReportInCall.GetCheck());
	}
}
