#pragma once
#include "AGVideoWnd.h"
class CMultiVideoSourceRteSceneEventHandler
	: public agora::rte::IAgoraRteSceneEventHandler
{
public:
	virtual ~CMultiVideoSourceRteSceneEventHandler() = default;

	// Inherited via IAgoraRteSceneEventHandler
	virtual void OnConnectionStateChanged(agora::rte::ConnectionState old_state, agora::rte::ConnectionState new_state,
		agora::rte::ConnectionChangedReason reason) override {
		if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_CONNECTION_STATE), (int)old_state, (int)reason);

	}

	virtual void OnRemoteUserJoined(const std::vector<agora::rte::UserInfo>& users) override {
		
	}
	virtual void OnRemoteUserLeft(const std::vector<agora::rte::UserInfo>& users) override {
		
	}
	//set the message notify window handler
	void SetMsgReceiver(HWND hWnd) { m_hMsgHanlder = hWnd; }
	virtual void OnRemoteStreamAdded(const std::vector<agora::rte::StreamInfo>& streams)  override {
		
	}

	/**
	 * Occurs when remote streams are removed.
	 *
	 * @param streams Removed remote streams.
	 */
	virtual void OnRemoteStreamRemoved(const std::vector<agora::rte::StreamInfo>& streams)  override {
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
		if (m_hMsgHanlder) {

			RemoteVideoStateChanged* videoState = new RemoteVideoStateChanged(streams, media_type, old_state, new_state, reason);
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LOCAL_VIDEO_STATE_CHANGED), (WPARAM)videoState, 0);
		}
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
		if (m_hMsgHanlder) {

			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LOCAL_VIDEO_STATE_CHANGED), (int)old_state, (int)reason);
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
	virtual void OnStreamTokenExpired(const std::string& stream_id) override {}

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

	virtual void OnLocalStreamStats(const std::string& stream_id, const agora::rte::LocalStreamStats& stats) override{}

	virtual void OnRemoteStreamStats(const std::string& stream_id, const agora::rte::RemoteStreamStats& stats) override{}

private:
	HWND m_hMsgHanlder;
};

class CAgoraMutilVideoSourceDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAgoraMutilVideoSourceDlg)

public:
	CAgoraMutilVideoSourceDlg(CWnd* pParent = nullptr);   
	virtual ~CAgoraMutilVideoSourceDlg();

	enum { IDD = IDD_DIALOG_MUTI_SOURCE };
	static const int VIDOE_COUNT = 2;
	//Initialize the Agora SDK
	bool InitAgora();
	//UnInitialize the Agora SDK
	void UnInitAgora();
	//set control text from config.
	void InitCtrlText();
	//render local video from SDK local capture.
	void RenderLocalVideo();
	// resume window status.
	void ResumeStatus();

	void StartDesktopShare();
private:
	bool m_joinChannel = false;
	bool m_initialize = false;

	std::string m_strChannel;

	agora::rtc::IRtcEngine* m_rtcEngine = nullptr;
	
	//conn_id_t m_conn_screen;
	//conn_id_t m_conn_camera;
	
	bool m_bPublishScreen = false;
	CAGVideoWnd m_videoWnds[VIDOE_COUNT];

	agora::rtc::uid_t m_screenUid = 0;

	std::shared_ptr<CMultiVideoSourceRteSceneEventHandler> m_sceneEventHandler;
	std::shared_ptr<agora::rte::IAgoraRteScene> m_scene = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteScreenVideoTrack> screen_track_ = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteMediaFactory> media_control_ = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteCameraVideoTrack> camera_track_ = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteMicrophoneAudioTrack> microphone_ = nullptr;
	std::string local_camera_id_ = "ms_camera_stream_id";
	std::string local_screen_id_ = "ms_screen_stream_id";
	std::string local_user_id_ = "ms_user_id";
	std::string local_microphone_id_ = "ms_mic_id";
	std::vector<std::string> connectionStates;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);  
	// agora sdk message window handler
	LRESULT OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDUserJoined(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDUserOffline(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDLocalVideoStateChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSelchangeListInfoBroadcasting();
	afx_msg LRESULT OnEIDConnectionStateChanged(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	CStatic m_staVideoArea;
	CListBox m_lstInfo;
	CStatic m_staChannel;
	CEdit m_edtChannel;
	CButton m_btnJoinChannel;
	CStatic m_staVideoSource;
	
	CButton m_btnPublish;
	CStatic m_staDetail;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonJoinchannel();
	afx_msg void OnBnClickedButtonPublish();
	
};
