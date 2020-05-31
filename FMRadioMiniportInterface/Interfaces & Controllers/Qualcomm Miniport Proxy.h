#pragma once

#include "pch.h"
#include "Miniport Service Interface_h.h"

class QualcommMiniportProxy
{
	std::mutex LibraryLock;
	IMiniportTunerDevice *& MiniportTunerDevice;
	IMiniportFmRxDevice *& MiniportReceiveDevice;

public:

	QualcommMiniportProxy(IMiniportTunerDevice *& MiniportTunerDevice, IMiniportFmRxDevice *& MiniportReceiveDevice);

	void SeekForwards();

	void SeekBackwards();

	void SetFrequency(FrequencyType Frequency);

	FrequencyType GetFrequency();

	unsigned GetSignalQuality();

};