#pragma once
#include "AGVideoWnd.h"
#include <map>



/*
if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_REMOTE_AUDIO_STATS), (WPARAM)new RemoteAudioStats(stats), 0);

if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LOCAL_VIDEO_STATS), (WPARAM)new LocalVideoStats(stats), 0);

if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_REMOTE_VIDEO_STATS), (WPARAM)new RemoteVideoStats(stats), 0);
*/


class CAgoraReportInCallDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAgoraReportInCallDlg)

public:
	CAgoraReportInCallDlg(CWnd* pParent = nullptr);   
	virtual ~CAgoraReportInCallDlg();

	enum { IDD = IDD_DIALOG_PEPORT_IN_CALL };
public:
	//Initialize the Ctrl Text.
	void InitCtrlText();
	//Initialize the Agora SDK
	bool InitAgora();
	//UnInitialize the Agora SDK
	void UnInitAgora();
	//render local video from SDK local capture.
	void RenderLocalVideo();
	//resume window status
	void ResumeStatus();

private:
	bool m_joinChannel = false;
	bool m_initialize = false;
	bool m_setEncrypt = false;
	
	CAGVideoWnd m_localVideoWnd;
	CAGVideoWnd m_remoteVideoWnd;

	

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	DECLARE_MESSAGE_MAP()
	// agora sdk message window handler
	LRESULT OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDUserJoined(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDUserOffline(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDRemoteVideoStateChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDRemoteVideoStats(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDRemoteAudioStats(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDRtcStats(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDLocalVideoStats(WPARAM wParam, LPARAM lParam);



public:
	CStatic m_staChannel;
	CEdit m_edtChannel;
	CButton m_btnJoinChannel;
	CStatic m_staVideoArea;
	CStatic m_gopNetWorkTotal;
	CStatic m_gopAudioRemote;
	CStatic m_gopVideoRemote;
	CStatic m_staUpDownLinkVal;
	CStatic m_staTotalBytes;
	CStatic m_staTotalBytesVal;
	CStatic m_staTotalBitrate;
	CStatic m_staTotalBitrateVal;
	CStatic m_staAudioNetWorkDelay;
	CStatic m_staAudioNetWorkDelayVal;
	CStatic m_staAudioRecvBitrate;
	CStatic m_staAudioRecvBitrateVal;
	CStatic m_staVideoNetWorkDelay;
	CStatic m_staVideoNetWorkDelayVal;
	CStatic m_staVideoRecvBitrate;
	CStatic m_staVideoRecvBitrateVal;
	CStatic m_staLocalVideoResoultion;
	CStatic m_staLocalVideoResoultionVal;
	CStatic m_staLocalVideoFPS;
	CStatic m_staLocalVideoFPSVal;
	CStatic m_staDetails;
	CListBox m_lstInfo;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonJoinchannel();
	afx_msg void OnSelchangeListInfoBroadcasting();

};
