#include "stdafx.h"
#include "WASAPISource.h"

#define BUFFER_TIME_100NS (5 * 10000000)
std::string GetDeviceName(IMMDevice* device) {
	CComPtr<IPropertyStore> store;
	HRESULT hr;
	std::string device_name;
	if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &store))) {
		PROPVARIANT nameVar;
		PropVariantInit(&nameVar);
		hr = store->GetValue(PKEY_Device_FriendlyName, &nameVar);

		if (SUCCEEDED(hr) && nameVar.pwszVal && *nameVar.pwszVal) {
			size_t len = wcslen(nameVar.pwszVal);
			size_t size;
			char szName[MAX_PATH] = { 0 };
			WideCharToMultiByte(CP_UTF8, 0, nameVar.pwszVal, len, szName, MAX_PATH, nullptr, nullptr);
			device_name = szName;
		}
	}

	return device_name;
}

FILE* fp = nullptr;
WASAPISource::WASAPISource(std::string id, bool input, bool default)
	: isInputDevice(input)
	, isDefaultDevice(default)
	, device_id(id)
{ 
	stopSignal = CreateEvent(nullptr, true, false, nullptr);
	receiveSignal = CreateEvent(nullptr, false, false, nullptr);
	QueryPerformanceFrequency(&clock_freq);
	fp = fopen("D:\\1.pcm", "wb+");
	Start();
}

WASAPISource::~WASAPISource()
{
	enumerator->UnregisterEndpointNotificationCallback(notify);
	Stop();
	fclose(fp);
}

void WASAPISource::UpdateDevice(std::string id, bool default)
{
	bool restart = id.compare(device_id) != 0;

	device_id = id;
	isDefaultDevice = default;

	if (restart) {
		Stop();
		Start();
	}
}

void WASAPISource::Start()
{
	if (!TryInitialize()) {
		
		Reconnect();
	}
}

void WASAPISource::Stop()
{
	SetEvent(stopSignal);

	if (active) {
		WaitForSingleObject(captureThread, INFINITE);
	}

	if(reconnecting)
		WaitForSingleObject(reconnectThread, INFINITE);
	ResetEvent(stopSignal);
}

bool WASAPISource::TryInitialize()
{
	try {
		Initialize();
	}
	catch (const char *error) {
		if (previouslyFailed)
			return active;
	}

	previouslyFailed = !active;
	return active;
}

void WASAPISource::Initialize()
{
	HRESULT hr;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
	if (FAILED(hr))
		throw "Failed to create enumerator";

	if (!InitDevice())
		return;

	device_name = GetDeviceName(device);

	if (!notify) {
		notify = new WASAPINotify(this);
		enumerator->RegisterEndpointNotificationCallback(notify);
	}

	IPropertyStore *store = nullptr;
	PWAVEFORMATEX deviceFormatProperties;
	PROPVARIANT prop;
	hr = device->OpenPropertyStore(STGM_READ, &store);

	PropVariantInit(&prop);
	if (!FAILED(hr)) {
		hr = store->GetValue(PKEY_AudioEngine_DeviceFormat, &prop);
		if (!FAILED(hr)) {
			if (!prop.vt != VT_EMPTY && prop.blob.pBlobData) {
				deviceFormatProperties = (PWAVEFORMATEX)prop.blob.pBlobData;
				device_sample = std::to_string(
					deviceFormatProperties->nSamplesPerSec);
			}
		}
		store->Release();
	}

	InitClient();
	if (!isInputDevice)
		InitRender();
	InitCapture();
}

void WASAPISource::InitFormat(WAVEFORMATEX *wfex)
{	
	if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE*)wfex;
		layout = ext->dwChannelMask;
	}
	sampleRate = wfex->nSamplesPerSec;
	channel = wfex->nChannels;

	if (audio_buffer) {
		delete[] audio_buffer;
		audio_buffer = nullptr;
	}
	audio_buffer = new BYTE[sampleRate / 100  * channel * bytesPerSample];
}

void WASAPISource::InitClient()
{
	
	HRESULT hr;
	hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&client);

	if (FAILED(hr))
		throw ("Failed to activate client context");

	hr = client->GetMixFormat(&wefex);
	if (FAILED(hr))
		throw ("Failed to get mix format");

	DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	if (!isInputDevice)
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

	InitFormat(wefex);

	hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, BUFFER_TIME_100NS, 0, wefex, nullptr);

	if (FAILED(hr))
		throw ("Failed to get initialize audio client");
}

void WASAPISource::InitRender()
{
	HRESULT hr;
	UINT32 frames;
	LPBYTE buffer;

	hr = client->GetBufferSize(&frames);
	if (FAILED(hr))
		throw "Failed to get buffer size";

	hr = client->GetService(__uuidof(IAudioRenderClient), (void**)&render);
	if (FAILED(hr))
		throw "Failed to get render client";

	hr = render->GetBuffer(frames, &buffer);
	if (FAILED(hr))
		throw ("Failed to get buffer");

	memset(buffer, 0, frames * wefex->nBlockAlign);
	render->ReleaseBuffer(frames, 0);
}

void WASAPISource::InitCapture()
{
	HRESULT res = client->GetService(__uuidof(IAudioCaptureClient),
		(void **)&capture);
	if (FAILED(res))
		throw ("Failed to create capture context");

	res = client->SetEventHandle(receiveSignal);
	if (FAILED(res))
		throw ("Failed to set event handle");

	captureThread = CreateThread(nullptr, 0, WASAPISource::CaptureThread,
		this, 0, nullptr);

	if(captureThread == nullptr)
		throw "Failed to create capture thread";

	client->Start();
	active = true;
}

DWORD WINAPI WASAPISource::CaptureThread(LPVOID param)
{
	WASAPISource *source = (WASAPISource*)param;
	bool reconnect = false;

	/* Output devices don't signal, so just make it check every 10 ms */
	DWORD dur = source->isInputDevice ? 3000 : 10;

	HANDLE sigs[2] = { source->receiveSignal, source->stopSignal };
	DWORD ret = 0;
	while ((ret = WaitForMultipleObjects(2, sigs, false, dur)) == WAIT_OBJECT_0 || ret == WAIT_TIMEOUT) {
		if (!source->ProcessCaptureData()) {
			reconnect = true;
			break;
		}
	}

	source->client->Stop();
	source->captureThread = nullptr;
	source->active = false;

	if (reconnect)
		source->Reconnect();

	return 0;
}


int ConvertFloatTo16Bit(char* pData, UINT nDataSize)
{
	if (nDataSize <= 0)
	{
		return -1;
	}
	float* pSrc = (float*)pData;
	short* pDst = (short*)pData;
	union { float f; DWORD i; } u;
	UINT processCount = nDataSize / 4;
	for (UINT i = 0; i < processCount; i++)
	{
		u.f = float(*pSrc + 384.0);
		if (u.i > 0x43c07fff)
		{
			*pDst = 32767;
		}
		else if (u.i < 0x43bf8000)
		{
			*pDst = -32768;
		}
		else
		{
			*pDst = short(u.i - 0x43c00000);
		}
		pSrc++;
		pDst++;
	}
	return nDataSize / 2;
}

bool WASAPISource::ProcessCaptureData()
{
	HRESULT hr;
	UINT captureSize = 0;
	LPBYTE buffer;
	UINT32 frames;
	DWORD flags;
	UINT64 pos, ts;

	int buffer_size = 0;
	while (true) {
		hr = capture->GetNextPacketSize(&captureSize);
		if (FAILED(hr)) {
			if (hr != AUDCLNT_E_DEVICE_INVALIDATED) {
			
			}
			return false;
		}

		if (!captureSize)
			break;

		hr = capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
		if (FAILED(hr)) {
			if (hr != AUDCLNT_E_DEVICE_INVALIDATED) {

			}
			return false;
		}
		
		if (recordFunc) {
			int size = ConvertFloatTo16Bit((char*)buffer, frames * 2 * 4);
			recordFunc(buffer, size);
		}
		
		capture->ReleaseBuffer(frames);
	}

	return true;
}

bool WASAPISource::InitDevice()
{
	HRESULT hr;

	if (isDefaultDevice) {
		hr = enumerator->GetDefaultAudioEndpoint(
			isInputDevice ? eCapture : eRender,
			isInputDevice ? eCommunications : eConsole,
			&device);

		if (FAILED(hr))
			return false;

		wchar_t* id = nullptr;
		hr = device->GetId(&id);
		default_id = id;
	}
	else {
		wchar_t szId[MAX_PATH] = { 0 };
		//WideCharToMultiByte(CP_UTF8, 0, w_id, wcslen(w_id), szId, MAX_PATH, nullptr, nullptr);
		MultiByteToWideChar(CP_UTF8, 0, device_id.c_str(), device_id.length(), szId, MAX_PATH);
		hr = enumerator->GetDevice(szId, &device);
	}

	return hr == S_OK;
}

void WASAPISource::SetDefaultDevice(EDataFlow flow, ERole role, LPCWSTR id)
{
	if (!isDefaultDevice)
		return;

	EDataFlow expectedFlow = isInputDevice ? eCapture : eRender;
	ERole expectedRole = isInputDevice ? eCommunications : eConsole;

	if (flow != expectedFlow || role != expectedRole)
		return;

	if (id && default_id.compare(id) == 0)
		return;

	

	LARGE_INTEGER current_time;
	double time_val;

	QueryPerformanceCounter(&current_time);
	time_val = (double)current_time.QuadPart;
	time_val *= 1000000000.0;
	time_val /= (double)clock_freq.QuadPart;

	uint64_t t = time_val;
	if (t - lastNotifyTime < 300000000)
		return;

	std::thread([this]() {
		Stop();
		Start();
		}).detach();

		lastNotifyTime = t;
}

void WASAPISource::Reconnect()
{
	reconnecting = true;
	reconnectThread = CreateThread(
		nullptr, 0, WASAPISource::ReconnectThread, this, 0, nullptr);

	if (reconnectThread == nullptr) {

	}
}


DWORD WINAPI WASAPISource::ReconnectThread(LPVOID param)
{
	WASAPISource *source = (WASAPISource *)param;

	const HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	const bool com_initialized = SUCCEEDED(hr);
	if (!com_initialized) {

	}

	while (WaitForSingleObject(source->stopSignal, 3000) != WAIT_TIMEOUT) {
		if (source->TryInitialize())
			break;
	}

	if (com_initialized)
		CoUninitialize();

	source->reconnectThread = nullptr;
	source->reconnecting = false;
	return 0;
}

void WASAPISource::GetAudioDevices(std::vector<AudioDeviceInfo>& deviceInfo, bool input)
{
	CComPtr<IMMDeviceEnumerator> deviceEnumerator;
	CComPtr<IMMDeviceCollection> collection;
	HRESULT hr;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
	if (FAILED(hr))
		return;
	
	deviceEnumerator->EnumAudioEndpoints(input ? eCapture : eRender, DEVICE_STATE_ACTIVE, &collection);
	if (FAILED(hr))
		return;

	UINT count = 0;
	collection->GetCount(&count);

	for (int i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;
		WCHAR* w_id = nullptr;
		AudioDeviceInfo info;
		hr = collection->Item(i, &device);
		if (FAILED(hr))
			continue;

		hr = device->GetId(&w_id);
		if (FAILED(hr) || !w_id || !*w_id)
			continue;
		info.name = GetDeviceName(device);

		char szId[MAX_PATH] = { 0 };
		WideCharToMultiByte(CP_UTF8, 0, w_id, wcslen(w_id), szId, MAX_PATH, nullptr, nullptr);
		info.id = szId;

		deviceInfo.push_back(info);
	}
}