#include "pch.h"
#include "Notifier Handles.h"
#include "Qualcomm Miniport Proxy.h"
#include "Checked Windows API Calls.h"

QualcommMiniportProxy::QualcommMiniportProxy(IMiniportFmRxDevice *& MiniportReceiveDevice) :
	MiniportReceiveDevice(MiniportReceiveDevice)
{
}

void QualcommMiniportProxy::SetFrequency(FrequencyType Frequency)
{
	std::lock_guard<decltype(LibraryLock)> Lock(LibraryLock);

	Windows::CheckedMemberAPICall(
		MiniportReceiveDevice,
		&IMiniportFmRxDevice::Tune,
		static_cast<FM_FREQUENCY>(Frequency),
		NotifierHandles::AsyncContextToHANDLE(NotifierHandles::AsyncContextHandle::FrequencyChange)
	);
}

FrequencyType QualcommMiniportProxy::GetFrequency()
{
	std::lock_guard<decltype(LibraryLock)> Lock(LibraryLock);

	FM_FREQUENCY Frequency;
	Windows::CheckedMemberAPICall(MiniportReceiveDevice, &IMiniportFmRxDevice::GetTunedFrequency, &Frequency);
	return static_cast<FrequencyType>(Frequency);
}