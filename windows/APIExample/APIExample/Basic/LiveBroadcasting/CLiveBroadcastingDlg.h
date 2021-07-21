#pragma once
#include "AGVideoWnd.h"
#include <unordered_set>

// CLiveBroadcastingDlg dialog

#define VIDEO_COUNT                     36
#define IDC_BASEWND_VIDEO               113

class CLiveBroadcastingRtcEngineEventHandler
    : public agora::rte::IAgoraRteSceneEventHandler
{
public:
    virtual ~CLiveBroadcastingRtcEngineEventHandler() = default;

    // Inherited via IAgoraRteSceneEventHandler
	virtual void OnConnectionStateChanged(agora::rte::ConnectionState old_state, agora::rte::ConnectionState new_state,
		agora::rte::ConnectionChangedReason reason) override;

    virtual void OnRemoteUserJoined(const std::vector<agora::rte::UserInfo>& users) override {
        // RTE_LOG_INFO << "Existing user: ";

        // for (auto& user : remote_users) {
             //RTE_LOG_INFO << user << " , ";
         //}

        // RTE_LOG_INFO << ". Adding user: " << users[0].user_id;

        // remote_users.emplace(users[0].user_id);

        // remote_user_joined_semaphore.Notify();
		if (m_hMsgHanlder) {
			
			agora::rte::UserInfo* userInfos = new  agora::rte::UserInfo[users.size()];
			for (int i = 0; i < users.size(); ++i) {
				userInfos[i].user_id = users[i].user_id;
				userInfos[i].user_name = users[i].user_name;
			}
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_USER_JOINED), (WPARAM)userInfos, users.size());
		}
    }
    virtual void OnRemoteUserLeft(const std::vector<agora::rte::UserInfo>& users) override {
        /* RTE_LOG_INFO << "Existing user: ";
         for (auto& user : remote_users) {
             RTE_LOG_INFO << user << " , ";
         }
         RTE_LOG_INFO << ". Removing user: " << users[0].user_id;

         remote_users.erase(users[0].user_id);
         remote_user_left_semaphore.Notify();*/
    }
    //set the message notify window handler
    void SetMsgReceiver(HWND hWnd) { m_hMsgHanlder = hWnd; }
    virtual void OnRemoteStreamAdded(const std::vector<agora::rte::StreamInfo>& streams)  override {
		if (m_hMsgHanlder) {

			agora::rte::StreamInfo* infos = new  agora::rte::StreamInfo[streams.size()];
			for (int i = 0; i < streams.size(); ++i) {
				infos[i].user_id = streams[i].user_id;
				infos[i].stream_id = streams[i].stream_id;
			}
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_REMOTE_VIDEOSTREAM_ADD), (WPARAM)infos, streams.size());
		}
	}

    /**
     * Occurs when remote streams are removed.
     *
     * @param streams Removed remote streams.
     */
    virtual void OnRemoteStreamRemoved(const std::vector<agora::rte::StreamInfo>& streams)  override {
		if (m_hMsgHanlder) {

			agora::rte::StreamInfo* infos = new  agora::rte::StreamInfo[streams.size()];
			for (int i = 0; i < streams.size(); ++i) {
				infos[i].user_id = streams[i].user_id;
				infos[i].stream_id = streams[i].stream_id;
			}
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_REMOTE_VIDEOSTREAM_REMOVE), (WPARAM)infos, streams.size());
		}
	}

    /**
     * Occurs when the media state of the local stream changes.
     *
     * @param streams Information of the local stream.
     * @param media_type Media type of the local stream.
     * @param old_state Old state of the local stream.
     * @param new_state New state of the local stream.
     * @param reason The reason of the state change.
     */
    virtual void OnLocalStreamStateChanged(const agora::rte::StreamInfo& streams, agora::rte::MediaType media_type,
        agora::rte::StreamMediaState old_state, agora::rte::StreamMediaState new_state,
        agora::rte::StreamStateChangedReason reason) override {
	
	}

    /**
     * Occurs when the media state of the remote stream changes.
     *
     * @param streams Information of the remote stream.
     * @param media_type Media type of the remote stream.
     * @param old_state Old state of the remote stream.
     * @param new_state New state of the remote stream.
     * @param reason The reason of the state change.
     */
    virtual void OnRemoteStreamStateChanged(const agora::rte::StreamInfo& streams, agora::rte::MediaType media_type,
        agora::rte::StreamMediaState old_state, agora::rte::StreamMediaState new_state,
        agora::rte::StreamStateChangedReason reason) override {
	
		if (m_hMsgHanlder ) {
			
			RemoteVideoStateChanged* videoState = new RemoteVideoStateChanged(streams, media_type, old_state, new_state, reason);
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_REMOTE_VIDEO_STATE_CHANGED), (WPARAM)videoState, 0);
		}
	}

    /**
     * Reports the volume information of users.
     *
     * @param speakers The volume information of users.
     * @param totalVolume Total volume after audio mixing. The value ranges between 0 (lowest volume) and 255 (highest volume).
     */
    virtual void OnAudioVolumeIndication(const std::vector<agora::rte::AudioVolumeInfo>& speakers,
        int totalVolume) override {}

    /**
     * Occurs when the token will expire in 30 seconds for the user.
     *
     * //TODO (yejun): expose APIto renew scene token
     * @param token The token that will expire in 30 seconds.
     */
    virtual void OnSceneTokenWillExpired(const std::string& scene_id, const std::string& token) override {}

    /**
     * Occurs when the token has expired for a user.
     *
     * //TODO (yejun): expose APIto renew scene token
     *
     * @param stream_id The ID of the scene.
     */
    virtual void OnSceneTokenExpired(const std::string& scene_id)override {}

    /**
     * Occurs when the token of a stream expires in 30 seconds.
     * If the token you specified when calling 'CreateOrUpdateRTCStream' expires,
     * the user will drop offline. This callback is triggered 30 seconds before the token expires, to
     * remind you to renew the token by calling 'CreateOrUpdateRTCStream' again with new token.
     * //TODO (yejun): Need to tell how to generate new token, ETA for new token facility
     * Upon receiving this callback, generate a new token at your app server
     *
     * @param stream_id
     * @param token
     */

    virtual void OnStreamTokenWillExpire(const std::string& stream_id, const std::string& token) override {}

    /**
     * Occurs when the token has expired for a stream.
     *
     * Upon receiving this callback, you must generate a new token on your server and call
     * "CreateOrUpdateRTCStream" to pass the new token to the SDK.
     *
     * @param stream_id The ID of the scene.
     */
    virtual void OnStreamTokenExpired(const std::string& stream_id) override{}

	virtual void OnBypassCdnStateChanged(
		const std::string& stream_id, const std::string& target_cdn_url,
		agora::rte::CDNBYPASS_STREAM_PUBLISH_STATE state,
		agora::rte::CDNBYPASS_STREAM_PUBLISH_ERROR err_code) override {}

	virtual void OnBypassCdnPublished(
		const std::string& stream_id, const std::string& target_cdn_url,
		agora::rte::CDNBYPASS_STREAM_PUBLISH_ERROR err_code) override {}

	virtual void OnBypassCdnUnpublished(const std::string& stream_id,
		const std::string& target_cdn_url) override {}

	virtual void OnBypassTranscodingUpdated(const std::string& stream_id) override {}

	virtual void OnSceneStats(const agora::rte::SceneStats& stats) override {}

	virtual void OnLocalStreamStats(const std::string& stream_id, const agora::rte::LocalStreamStats& stats) override;

	virtual void OnRemoteStreamStats(const std::string& stream_id, const agora::rte::RemoteStreamStats& stats) override;

private:
    HWND m_hMsgHanlder;
};

class CLiveBroadcastingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CLiveBroadcastingDlg)

public:
	CLiveBroadcastingDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CLiveBroadcastingDlg();

    //Initialize the Agora SDK
    bool InitAgora();
    //UnInitialize the Agora SDK
    void UnInitAgora();
    
	//resume window status
	void ResumeStatus();

// Dialog Data
    enum { IDD = IDD_DIALOG_LIVEBROADCASTING };
    //The number of people supported within the channel
    enum PEOPLE_IN_CHANNEL_TYPE {
        PEOPLE_IN_CHANNEL_2 = 0,
        PEOPLE_IN_CHANNEL_4,
        PEOPLE_IN_CHANNEL_9,
        PEOPLE_IN_CHANNEL_16
    };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnSelchangeComboPersons();
    afx_msg void OnSelchangeComboRole();
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    //Agora Event handler
    afx_msg LRESULT OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnEIDUserJoined(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnEIDUserOffline(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEIDConnectionStateChanged(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT  OnEIDLocalStreamStats(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT  OnEIDRemoteStreamStats(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRemoteStreamAdd(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnRemoteStreamRemove(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnEIDRemoteVideoStateChanged(WPARAM wParam, LPARAM lParam);
private:
    //set control text from config.
    void InitCtrlText();
    //create all video window to save m_videoWnds.
    void CreateAllVideoWnds();
    //show all video window from m_videoWnds.
    void ShowVideoWnds();
    //render local video from SDK local capture.
    void RenderLocalVideo();


    //IRtcEngine* m_rtcEngine = nullptr;
    //CLiveBroadcastingRtcEngineEventHandler m_eventHandler;
  
    bool m_joinChannel = false;
    bool m_initialize = false;
    //video wnd
    CAGVideoWnd m_videoWnds[VIDEO_COUNT];
    int m_maxVideoCount = 4;
    std::unordered_set<std::string> m_strmIds;
    std::shared_ptr<CLiveBroadcastingRtcEngineEventHandler> m_sceneEventHandler;
    std::shared_ptr<agora::rte::IAgoraRteScene> m_scene = nullptr;

	std::shared_ptr<agora::rte::IAgoraRteMediaFactory> media_control_ = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteCameraVideoTrack> camera_track_ = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteMicrophoneAudioTrack> microphone_ = nullptr;
	std::string local_camera_id_ = "live_camera_stream_id";
	std::string local_user_id_ = "live_user_id";
	std::string local_microphone_id_ = "live_mic_id";

	std::vector<agora::rte::VideoDeviceInfo> cameraLists_;
	agora::rte::IAgoraRteAudioDeviceCollection* mics_;
	agora::rte::IAgoraRteAudioDeviceCollection* speakers_;
	std::shared_ptr<agora::rte::IAgoraRteAudioDeviceManager> audioManager_;
	std::vector<int> bitrates_;
	std::vector<std::string> connectionStates;
	agora::rte::VideoEncoderConfiguration videoEcnoder_;
	agora::rte::AudioEncoderConfiguration audioEncoder_;

	std::vector<agora::rtc::VIDEO_CODEC_TYPE> coedec_types_;
	
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonJoinchannel();
	afx_msg void OnSelchangeListInfoBroadcasting();
	afx_msg void OnStnClickedStaticVideo();

	CComboBox m_cmbRole;
    CStatic m_staRole;
    CComboBox m_cmbPersons;
    CEdit m_edtChannelName;
    CButton m_btnJoinChannel;
    CListBox m_lstInfo;
    CStatic m_videoArea;
    CStatic m_staPersons;
    CStatic m_staChannelName;
    CStatic m_staDetail;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CComboBox m_cmbVideoEncoder;
	CStatic m_staWidth;
	CEdit m_edtWidth;
	CStatic m_staHeight;
	CEdit m_edtHeight;
	CStatic m_staFPS;
	CEdit m_edtFPS;
	CStatic m_staBitrate;
	CComboBox m_cmbBitrate;
	CComboBox m_cmbAudioProfile;
	CStatic m_staAudioEncoder;
	CComboBox m_cmbSampleRate;
	CButton m_chkStereo;
	CStatic m_staSampleRate;
	CStatic m_staVideoDevice;
	CComboBox m_cmbCameraList;
	CComboBox m_cmbMicList;
	CComboBox m_cmbSpeakerList;
	CStatic m_staMicrophone;
	CStatic m_staSpk;
	CSliderCtrl m_sldMic;
	CSliderCtrl m_sldSpeaker;
	afx_msg void OnSelchangeComboCameraList();
	afx_msg void OnSelchangeComboMicList();
	afx_msg void OnSelchangeComboSpeakerList();
	afx_msg void OnCustomdrawSliderMic(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCustomdrawSliderSpeaker(NMHDR *pNMHDR, LRESULT *pResult);
	
	CButton m_chkReportInCall;
	afx_msg void OnClickedCheckReportIncall();
};
