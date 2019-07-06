#include "pch.h"
#include "Event Notifier.h"
#include "Checked Windows API Calls.h"

size_t EventNotifier::AsyncContextToBit(HTUNER_ASYNCCTXT EventHandle)
{
	using namespace NotifierHandles;

	auto EventType = static_cast<AsyncContextHandle>(reinterpret_cast<uintptr_t>(EventHandle));
	switch (EventType)
	{
		case AsyncContextHandle::FrequencyChange: return FrequencyBit;
		case AsyncContextHandle::PlayStateChange: return PlayStateBit;
	}
}

EventNotifier::EventNotifier(IMiniportFmRxDevice *& Device, FmController & Controller) :
	MiniportReceiveDevice(Device),
	TopologyController(Controller)
{
}

HTUNER_ASYNCCTXT NotifierHandles::AsyncContextToHANDLE(AsyncContextHandle Enum)
{
	return reinterpret_cast<HTUNER_ASYNCCTXT>(Enum);
}

void EventNotifier::AcquireEvent(Client ClientId, Notification * RadioEvent)
{
	std::unique_lock<decltype(EventsMutex)> Lock(EventsMutex);
	NewEvent.wait(
		Lock,
		[this, ClientId]
		{
			const auto Event = Events.find(ClientId);
			if (Event == Events.cend())
			{
				throw std::invalid_argument("The specified client identifier was not found.");
			}
			return (*Event).second.any();
		}
	);
	
	auto & EventType = Events[ClientId];

	// Process one at a time, and unset when processed

	if (EventType[FrequencyBit])
	{
		EventType[FrequencyBit] = false;

		FM_FREQUENCY Frequency;
		Windows::CheckedMemberAPICall(MiniportReceiveDevice, &IMiniportFmRxDevice::GetTunedFrequency, &Frequency);
		RadioEvent->Type = Event::FrequencyChanged;
		RadioEvent->tagged_union.KHz = Frequency;
		return;
	}

	if (EventType[PlayStateBit])
	{
		EventType[PlayStateBit] = false;

		BOOL State;
		TopologyController.GetFmState(&State);
		RadioEvent->Type = Event::PlayStateChanged;
		RadioEvent->tagged_union.PlayState = static_cast<boolean>(State);
		return;
	}
}

void EventNotifier::OnRadioEvent(HTUNER_ASYNCCTXT EventHandle)
{
	if (EventHandle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	{
		std::lock_guard<decltype(EventsMutex)> Lock(EventsMutex);

		for (auto & Event : Events)
		{
			const auto Bit = AsyncContextToBit(EventHandle);
			Event.second[Bit] = true;
		}
	}

	NewEvent.notify_all();
}

void EventNotifier::OnClientAdded(Client ClientId)
{
	Events.emplace(ClientId, decltype(Events)::value_type::second_type());
}