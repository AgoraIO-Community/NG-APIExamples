#pragma once
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <propsys.h>
#include <vector>
#include <functiondiscoverykeys_devpkey.h>
struct AudioDeviceInfo {
	std::string name;
	std::string id;
};

using AudioCaptureFunc = std::function<void(LPBYTE data, int size)>;

class WASAPISource
{
public:
	WASAPISource(std::string id, bool input, bool default = true);
	~WASAPISource();

	static void GetAudioDevices(std::vector<AudioDeviceInfo>& deviceInfo, bool input);
	void SetDefaultDevice(EDataFlow flow, ERole role, LPCWSTR id);
	void UpdateDevice(std::string id, bool default);
	void SetRecordCaptureFunc(AudioCaptureFunc func) { recordFunc = func; }
	int GetAudioChannel() {
		return channel;
	} 
	int GetAudioSampleRate() {
		return sampleRate;
	}
private:
	CComPtr<IMMDevice> device;
	CComPtr<IAudioClient> client;
	CComPtr<IAudioCaptureClient> capture;
	CComPtr<IAudioRenderClient> render;
	CComPtr<IMMDeviceEnumerator> enumerator;
	CComPtr<IMMNotificationClient> notify;

	std::wstring default_id;
	std::string device_id;
	std::string device_name;
	std::string device_sample = "-";

	bool isInputDevice;
	bool useDeviceTiming = false;
	bool isDefaultDevice = false;

	bool reconnecting = false;
	bool previouslyFailed = false;
	HANDLE reconnectThread;

	bool active = false;
	HANDLE captureThread;
	HANDLE stopSignal;
	HANDLE receiveSignal;

	//AUDIO_FORMAT_16BIT_PLANAR
	int channel = 2;
	uint32_t sampleRate = 48000;
	int bytesPerSample = 2;// 16bit int
	DWORD layout = KSAUDIO_SPEAKER_2POINT1;
	WAVEFORMATEX* wefex = nullptr;
	LPBYTE audio_buffer = nullptr;

	uint64_t lastNotifyTime = 0;
	LARGE_INTEGER clock_freq;
	AudioCaptureFunc recordFunc = nullptr;
	bool ProcessCaptureData();
	
	inline void Start();
	inline void Stop();
	void Reconnect();

	bool InitDevice();
	void InitClient();
	void InitRender();
	void InitFormat(WAVEFORMATEX *wfex);
	void InitCapture();
	void Initialize();
	bool TryInitialize();

	static DWORD WINAPI ReconnectThread(LPVOID param);
	static DWORD WINAPI CaptureThread(LPVOID param);

};

class WASAPINotify : public IMMNotificationClient {
	std::atomic<long> refs = 0; /* auto-incremented to 1 by ComPtr */
	WASAPISource *source;

public:
	WASAPINotify(WASAPISource *source_) : source(source_) {}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return (ULONG)++refs;//os_atomic_inc_long(&refs);
	}

	STDMETHODIMP_(ULONG) STDMETHODCALLTYPE Release()
	{
		long val = --refs;//os_atomic_dec_long(&refs);
		if (val == 0)
			delete this;
		return (ULONG)val;
	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ptr)
	{
		if (riid == IID_IUnknown) {
			*ptr = (IUnknown *)this;
		}
		else if (riid == __uuidof(IMMNotificationClient)) {
			*ptr = (IMMNotificationClient *)this;
		}
		else {
			*ptr = nullptr;
			return E_NOINTERFACE;
		}

		++refs;
		//os_atomic_inc_long(&refs);
		return S_OK;
	}

	STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role,
		LPCWSTR id)
	{
		source->SetDefaultDevice(flow, role, id);
		return S_OK;
	}

	STDMETHODIMP OnDeviceAdded(LPCWSTR) { return S_OK; }
	STDMETHODIMP OnDeviceRemoved(LPCWSTR) { return S_OK; }
	STDMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) { return S_OK; }
	STDMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY)
	{
		return S_OK;
	}
};
