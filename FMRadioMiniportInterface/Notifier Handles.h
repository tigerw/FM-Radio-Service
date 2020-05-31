#pragma once

#include "pch.h"

namespace NotifierHandles
{
	enum class AsyncContextHandle
	{
		/* Start at 1 since this is type-punned into a HANDLE pointer, and we all know what a 0 pointer means... */
		FrequencyChange = 1,
		PlayStateChange = 2,
		AntennaStatusChange = 3,
		RDSAvailablility = 4
	};

	HTUNER_ASYNCCTXT AsyncContextToHANDLE(AsyncContextHandle Enum);
}