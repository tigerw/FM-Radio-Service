#include "pch.h"
#include "Notifier Handles.h"
#include "Qualcomm Miniport Proxy.h"
#include "Checked Windows API Calls.h"

QualcommMiniportProxy::QualcommMiniportProxy(IMiniportTunerDevice *& MiniportTunerDevice, IMiniportFmRxDevice *& MiniportReceiveDevice) :
	MiniportTunerDevice(MiniportTunerDevice),
	MiniportReceiveDevice(MiniportReceiveDevice)
{
}

void QualcommMiniportProxy::SeekForwards()
{
	std::lock_guard<decltype(LibraryLock)> Lock(LibraryLock);

	Windows::CheckedMemberAPICall(
		MiniportReceiveDevice,
		&IMiniportFmRxDevice::Seek,
		FM_SEEKDIR::FM_SEEKDIR_FORWARD,
		NotifierHandles::AsyncContextToHANDLE(NotifierHandles::AsyncContextHandle::FrequencyChange)
	);
}

void QualcommMiniportProxy::SeekBackwards()
{
	std::lock_guard<decltype(LibraryLock)> Lock(LibraryLock);

	Windows::CheckedMemberAPICall(
		MiniportReceiveDevice,
		&IMiniportFmRxDevice::Seek,
		FM_SEEKDIR::FM_SEEKDIR_BACKWARD,
		NotifierHandles::AsyncContextToHANDLE(NotifierHandles::AsyncContextHandle::FrequencyChange)
	);
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

unsigned QualcommMiniportProxy::GetSignalQuality()
{
	std::lock_guard<decltype(LibraryLock)> Lock(LibraryLock);

	DWORD ReturnSize;
	TUNER_PROPDATA_RSSI RSSI;
	Windows::CheckedMemberAPICall(MiniportTunerDevice, &IMiniportTunerDevice::GetProperty, TUNER_PROPID_RSSI, &RSSI, sizeof(RSSI), &ReturnSize);

	static const auto Minimum = 320L;
	static const auto Maximum = 400L;

	// Map a 320-400 scale (by experiment) to 0-100
	return static_cast<unsigned>(std::max(0L, (100 * (static_cast<signed long>(RSSI.dwRssi) - Minimum)) / (Maximum - Minimum)));
}