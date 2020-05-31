#include "pch.h"
#include "Event Notifier.h"
#include "Notifier Handles.h"
#include "Checked Windows API Calls.h"

EventNotifier::EventBit EventNotifier::AsyncContextToBit(HTUNER_ASYNCCTXT EventHandle)
{
	using namespace NotifierHandles;

	auto EventType = static_cast<AsyncContextHandle>(reinterpret_cast<uintptr_t>(EventHandle));
	switch (EventType)
	{
		case AsyncContextHandle::FrequencyChange: return EventBit::Frequency;
		case AsyncContextHandle::PlayStateChange: return EventBit::PlayState;
		case AsyncContextHandle::AntennaStatusChange: return EventBit::AntennaStatus;
		case AsyncContextHandle::RDSAvailablility: return EventBit::RDS;
	}
}

EventNotifier::EventNotifier(IMiniportFmRxDevice *& Device, RadioTopology & Controller, QualcommMiniportProxy & Proxy) :
	MiniportReceiveDevice(Device),
	TopologyController(Controller),
	LibraryProxy(Proxy)
{
}

void EventNotifier::AcquireEvent(Client ClientId, Notification * RadioEvent)
{
	EventBit EventType;

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

		auto & Event = Events[ClientId];

		// Process one at a time, and unset when processed
		for (size_t Bit = 0; Bit != static_cast<size_t>(EventBit::Count); Bit++)
		{
			if (!Event[Bit])
			{
				continue;
			}

			EventType = static_cast<EventBit>(Bit);
			Event[Bit] = false;
			break;
		}

		// Condition variable only lets us through if at least 1 bit was set
	}

	switch (EventType)
	{
		case EventBit::Shutdown:
		{
			// Client doesn't come back after this, because the RPC server should already be stopped
			return;
		}
		case EventBit::Frequency:
		{
			RadioEvent->Type = Event::FrequencyChanged;
			RadioEvent->tagged_union.KHz = LibraryProxy.GetFrequency();
			return;
		}
		case EventBit::PlayState:
		{
			BOOL State;
			TopologyController.GetFmState(&State);
			RadioEvent->Type = Event::PlayStateChanged;
			RadioEvent->tagged_union.PlayState = static_cast<boolean>(State);
			return;
		}
		case EventBit::AntennaStatus:
		{
			RadioEvent->Type = Event::AntennaStateChanged;
			RadioEvent->tagged_union.Present = TopologyController.IsAntennaPresent();
			return;
		}
	}

	// assert(!"Unexpected event bit value");
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
			Event.second[static_cast<size_t>(Bit)] = true;
		}
	}

	NewEvent.notify_all();
}

void EventNotifier::OnClientAdded(Client ClientId)
{
	std::lock_guard<decltype(EventsMutex)> Lock(EventsMutex);
	Events.emplace(ClientId, decltype(Events)::value_type::second_type());
}

void EventNotifier::Shutdown()
{
	{
		std::lock_guard<decltype(EventsMutex)> Lock(EventsMutex);

		for (auto & Event : Events)
		{
			Event.second[static_cast<size_t>(EventBit::Shutdown)] = true;
		}
	}

	NewEvent.notify_all();
}