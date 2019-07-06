#include "pch.h"
#include "Event Notifier.h"
#include "Checked Windows API Calls.h"

EventNotifier::EventNotifier(IMiniportFmRxDevice *& Device, FmController & Controller) :
	MiniportReceiveDevice(Device),
	TopologyController(Controller)
{
}

Notification EventNotifier::AcquireEvent()
{
	std::unique_lock<decltype(QueueMutex)> Lock(QueueMutex);
	NewEvent.wait(Lock, [this] { return !Events.empty(); });

	const auto Event = Events.front();
	Events.pop();
	return Event;
}

void EventNotifier::OnRadioEvent(HTUNER_ASYNCCTXT EventHandle)
{
	{
		std::lock_guard<decltype(QueueMutex)> Lock(QueueMutex);
		Notification RadioEvent;

		switch (reinterpret_cast<unsigned>(EventHandle))
		{
			case 1:
			{
				FM_FREQUENCY Frequency;
				Windows::CheckedMemberAPICall(MiniportReceiveDevice, &IMiniportFmRxDevice::GetTunedFrequency, &Frequency);
				RadioEvent.Type = Event::FrequencyChanged;
				RadioEvent.tagged_union.KHz = Frequency;
				break;
			}
			case 2:
			{
				BOOL State;
				TopologyController.GetFmState(&State);
				RadioEvent.Type = Event::PlayStateChanged;
				RadioEvent.tagged_union.PlayState = State;
				break;
			}
			default: return;
		}

		Events.emplace(RadioEvent);
	}

	NewEvent.notify_one();
}