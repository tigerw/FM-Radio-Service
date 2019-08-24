#pragma once

#include "pch.h"
#include "Miniport Service Interface_h.h"

class QualcommMiniportProxy
{
	std::mutex LibraryLock;
	IMiniportFmRxDevice *& MiniportReceiveDevice;

public:

	QualcommMiniportProxy(IMiniportFmRxDevice *& MiniportReceiveDevice);

	void SetFrequency(FrequencyType Frequency);

	FrequencyType GetFrequency();

};