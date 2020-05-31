#pragma once

#include "pch.h"

// From sysvad.h
#define VALUE_NORMALIZE_P(v, step) \
    ((((v)+(step) / 2) / (step)) * (step))

#define VALUE_NORMALIZE(v, step) \
    ((v) > 0 ? VALUE_NORMALIZE_P((v), (step)) : -(VALUE_NORMALIZE_P(-(v), (step))))

#define VALUE_NORMALIZE_IN_RANGE_EX(v, min, max, step) \
    ((v) > (max) ? (max) : \
    (v) < (min) ? (min) : \
    VALUE_NORMALIZE((v), (step)))

#define MAX_PROVIDERIDS 4

// From sysvad.h
#define VALUE_NORMALIZE_P(v, step) \
    ((((v)+(step) / 2) / (step)) * (step))

#define VALUE_NORMALIZE(v, step) \
    ((v) > 0 ? VALUE_NORMALIZE_P((v), (step)) : -(VALUE_NORMALIZE_P(-(v), (step))))

#define VALUE_NORMALIZE_IN_RANGE_EX(v, min, max, step) \
    ((v) > (max) ? (max) : \
    (v) < (min) ? (min) : \
    VALUE_NORMALIZE((v), (step)))

class MMEvent : public IMMNotificationClient
{
public:

	using StatusEvent = std::function<void()>;

	MMEvent(const LPWSTR & SpeakersId, const StatusEvent & AntennaStatusHandler) :
		CommunicationSpeakerId(SpeakersId),
		OnAntennaStatusChange(AntennaStatusHandler)
	{
	}

private:

	[[ nodiscard ]]
	HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID riid,
		_COM_Outptr_ void ** ppvObject) final override
	{
		return E_ILLEGAL_METHOD_CALL;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) final override
	{
		return ReferenceCounter.fetch_add(1);
	}

	ULONG STDMETHODCALLTYPE Release(void) final override
	{
		return ReferenceCounter.fetch_sub(1);
	}

	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR DeviceId, DWORD) override
	{
		if (
			(CommunicationSpeakerId != nullptr) &&
			(wcscmp(DeviceId, CommunicationSpeakerId) == 0) &&
			OnAntennaStatusChange
		)
		{
			OnAntennaStatusChange();
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, PROPERTYKEY) override
	{
		return S_OK;
	}

	std::atomic_ulong ReferenceCounter;
	const LPWSTR & CommunicationSpeakerId;
	const StatusEvent & OnAntennaStatusChange;

};

class RadioTopology
{
public:
	RadioTopology();

	HRESULT SetFmVolume(_In_ LONG volume);
	HRESULT GetFmVolume(_Out_ LONG* plVolume);
	HRESULT SetFmEndpointId(_In_ LONG index);
	HRESULT GetFmEndpointId(_Out_ PKSTOPOLOGY_ENDPOINTID pEndpointId);
	HRESULT GetFmAntennaEndpointId(_Out_ PKSTOPOLOGY_ENDPOINTID pEndpointId);
	HRESULT SetFmState(_In_ BOOL state);
	HRESULT GetFmState(_Out_ BOOL* pbState);
	bool IsAntennaPresent();

	MMEvent::StatusEvent OnAntennaStatusChange;

private:

	LPWSTR m_CommunicationSpeakers;
	MMEvent Event;
	CComPtr<IPart> CommunicationSpeakersPart;
	CComPtr<IKsControl>     m_spWaveKsControl;      // KsControl for wave filter
	CComPtr<IKsControl>     m_spTopologyKsControl;  // KsControl for topology filter
};