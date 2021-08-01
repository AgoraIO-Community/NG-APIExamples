#pragma once
#include "AGVideoWnd.h"
#include "AgVideoBuffer.h"
#include "dshowcapture.hpp"
#include "WASAPISource.h"
class CAgoraAVCaptureDlgEngineEventHandler : public agora::rte::IAgoraRteSceneEventHandler
{
public:
	virtual ~CAgoraAVCaptureDlgEngineEventHandler() = default;

	// Inherited via IAgoraRteSceneEventHandler
	virtual void OnConnectionStateChanged(agora::rte::ConnectionState old_state, agora::rte::ConnectionState new_state,
		agora::rte::ConnectionChangedReason reason) override;

	virtual void OnRemoteUserJoined(const std::vector<agora::rte::UserInfo>& users) override {
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

		if (m_hMsgHanlder) {

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

	virtual void OnLocalStreamStats(const std::string& stream_id, const agora::rte::LocalStreamStats& stats) override;

	virtual void OnRemoteStreamStats(const std::string& stream_id, const agora::rte::RemoteStreamStats& stats) override;

private:
	HWND m_hMsgHanlder;
};


class CAgoraAVCaptureDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAgoraAVCaptureDlg)

public:
	// agora sdk message window handler
	LRESULT OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDUserJoined(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDUserOffline(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDRemoteVideoStateChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEIDConnectionStateChanged(WPARAM wParam, LPARAM lParam);
	
	CAgoraAVCaptureDlg(CWnd* pParent = nullptr); 
	virtual ~CAgoraAVCaptureDlg();
	//Initialize the Agora SDK
	bool InitAgora();
	//UnInitialize the Agora SDK
	void UnInitAgora();
	//initialize libdshowcapture
	void InitExternalDevice();

	//set control text from config.
	void InitCtrlText();
	//render local video from SDK local capture.
	void RenderLocalVideo();
	//register or unregister agora video frame observer.
	
	// update window view and control.
	void UpdateViews();
	// enumerate device and show device in combobox.
	void UpdateDevice();
	// resume window status.
	void ResumeStatus();
	// start or stop capture.
	
	//static void ThreadRun(CAgoraAVCaptureDlg*self);
	void InitVideoConfig();
enum { IDD = IDD_DIALOG_CUSTOM_CAPTURE_VIDEO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX); 

private:
	void StartAudioFrame();
	void StopAudioFrame();
	CAgoraAVCaptureDlgEngineEventHandler m_eventHandler;
	
	CAGVideoWnd m_localVideoWnd;
	bool m_joinChannel = false;
	bool m_initialize = false;
	bool m_remoteJoined = false;
	bool m_extenalCaptureVideo = false;

	FILE* fp = nullptr;
	std::shared_ptr<CAgoraAVCaptureDlgEngineEventHandler> m_sceneEventHandler;
	std::shared_ptr<agora::rte::IAgoraRteCustomVideoTrack>  customVideoTrack_;
	std::shared_ptr<agora::rte::IAgoraRteCustomAudioTrack>  customAudioTrack_;
	std::shared_ptr<agora::rte::IAgoraRteMediaFactory> media_control_ = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteScene> m_scene = nullptr;
	std::string custom_video_id_ = "custom_video_id";
	std::string local_user_id_ = "custom_user_id";
	std::string custom_audio_id = "custom_audio_id";

	DShow::VideoConfig videoConfig_;
	DShow::Device videoDevice_;
	std::vector<DShow::VideoDevice> videoDevices_;
	agora::rte::ExternalVideoFrame videoFrame_;
	agora::rte::AudioFrame audioFrame_;
	std::map<DShow::VideoFormat, CString> mapFormat;
	std::vector<std::string> connectionStates;

	std::vector<AudioDeviceInfo> audioDevicesInfo_;
	std::unique_ptr<WASAPISource> wasapiInput;
	DECLARE_MESSAGE_MAP()
public:
	CStatic m_staVideoArea;
	CStatic m_staChannelName;
	CStatic m_staCaputreVideo;
	CEdit m_edtChannel;
	CButton m_btnJoinChannel;
	CButton m_btnSetExtCapture;
	CComboBox m_cmbVideoDevice;
	CComboBox m_cmbVideoType;
	CListBox m_lstInfo;
	virtual BOOL OnInitDialog();
	afx_msg	void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnClickedButtonStartCaputre();
	afx_msg void OnClickedButtonJoinchannel();
	afx_msg void OnSelchangeComboCaptureVideoDevice();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CComboBox m_cmbAudioDevice;
};
