#pragma once

#include "pch.h"
#include "FM Receive Port Device.h"
#include "Interfaces & Controllers/Event Notifier.h"

class FakePortTuner : public IPortTunerDevice
{
public:
	FakePortTuner(FakePortFmRx & ReceiveDevice, EventNotifier & Notifier) :
		ReceiveDevice(ReceiveDevice),
		Notifier(Notifier)
	{
	}

	HRESULT STDMETHODCALLTYPE SendTunerEvent(
		[[ maybe_unused ]] /* [in] */ REFGUID rguidEventId,
		[[ maybe_unused ]] /* [in] */ AV_VARIANT * pavvParam) final override
	{
		return S_OK;
	}

	[[ nodiscard ]]
	HRESULT STDMETHODCALLTYPE AsyncComplete(
		/* [in] */ HRESULT hrStatus,
		/* [in] */ HTUNER_ASYNCCTXT hAsyncCtxt) final override
	{
		if (FAILED(hrStatus))
		{
			// Event was "ignored successfully", I guess
			return S_OK;
		}

		try
		{
			Notifier.OnRadioEvent(hAsyncCtxt);
			return S_OK;
		}
		catch (...)
		{
			/* TODO:
			Granular catch statements: exists
			Descriptive error codes: exists
			Me, an intellectual: */
			return E_FAIL;
		}
	}

	[[ nodiscard ]]
	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) final override
	{
		if (riid != IID_IPortFmRxDevice)
		{
			return E_INVALIDARG;
		}

		*ppvObject = &ReceiveDevice;
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) final override
	{
		return ReferenceCounter.fetch_add(1);
	}

	ULONG STDMETHODCALLTYPE Release(void) final override
	{
		return ReferenceCounter.fetch_sub(1);
	}

private:
	std::atomic_ulong ReferenceCounter;
	FakePortFmRx & ReceiveDevice;
	EventNotifier & Notifier;
};