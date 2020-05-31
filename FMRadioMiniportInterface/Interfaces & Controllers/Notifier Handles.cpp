#include "pch.h"
#include "Notifier Handles.h"

HTUNER_ASYNCCTXT NotifierHandles::AsyncContextToHANDLE(AsyncContextHandle Enum)
{
	return reinterpret_cast<HTUNER_ASYNCCTXT>(Enum);
}