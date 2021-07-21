#pragma once

#define WM_SHOWMODECHANGED	WM_USER+300
#define WM_SHOWBIG			WM_USER+301

#define WND_VIDEO_WIDTH     192
#define WND_VIDEO_HEIGHT    144

#define WND_INFO_WIDTH      256
#define WND_INFO_HEIGHT     20*4

class CAGInfoWnd : public CWnd
{
	DECLARE_DYNAMIC(CAGInfoWnd)

public:
	CAGInfoWnd();
	virtual ~CAGInfoWnd();
	void SetLocalStats(const LocalStats stat);
	void SetRemoteStats(const RemoteStats stat);
	void ShowTips(BOOL bShow = TRUE);
	void SetVideoResolution(int nWidth, int nHeight);
	void SetFrameRateInfo(int nFPS);
	void SetBitrateInfo(int nBitrate);

    void SetUID(std::string dwUID) { m_nUID = dwUID;
	    uid = utf82cs(dwUID);
	}
	void SetStrmId(std::string strmId) { m_strmId = strmId; 
	this->strmId = utf82cs(strmId);
	}
	BOOL		m_bLocal;
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
	
private:
	BOOL		m_bShowTip;
	
	COLORREF	m_crBackColor;

	int		m_nWidth;
	int		m_nHeight;
	int		m_nFps;
	int		m_nBitrate;

	CBrush	m_brBack;
    std::string	m_nUID = "";
	std::string     m_strmId = "";
	CString strmId, uid;
	LocalStats localstat;
	RemoteStats remotestat;
	
};


class CAGVideoWnd : public CWnd
{
	DECLARE_DYNAMIC(CAGVideoWnd)

public:
	CAGVideoWnd();
	virtual ~CAGVideoWnd();

	void SetUID(std::string dwUID);
	void SetStrmId(std::string strmId);
	std::string GetUID();
	std::string GetStrmId() { return m_strmId; }
	BOOL IsWndFree();

	void SetFaceColor(COLORREF crBackColor);
	BOOL SetBackImage(UINT nID, UINT nWidth, UINT nHeight, COLORREF crMask = RGB(0xFF, 0xff, 0xFF));

	void SetVideoResolution(UINT nWidth, UINT nHeight);
	void GetVideoResolution(UINT *nWidth, UINT *nHeight);
	
	void SetBitrateInfo(int nReceivedBitrate);
	int	GetBitrateInfo() { return m_nBitRate; };
	
	void SetFrameRateInfo(int nReceiveFrameRate);
	int GetFrameRateInfo() { return m_nFrameRate; };

	void ShowVideoInfo(BOOL bShow);
	BOOL IsVideoInfoShowed() { return m_bShowVideoInfo; };

	void SetLocalFlag(BOOL bLocal);
	BOOL IsLocal() { return m_wndInfo.m_bLocal ; };
	

	void SetLocalStats(const LocalStats stat) { m_wndInfo.SetLocalStats(stat); }
	void SetRemoteStats(const RemoteStats stat) { m_wndInfo.SetRemoteStats(stat); }
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

private:
	CImageList		m_imgBackGround;
	COLORREF		m_crBackColor;

	CAGInfoWnd		m_wndInfo;

private:
	std::string		m_nUID = "";
	std::string     m_strmId = "";
	UINT		m_nWidth;
	UINT		m_nHeight;
	int			m_nFrameRate;
	int			m_nBitRate;
	BOOL		m_bShowVideoInfo = TRUE;
	
};


