#pragma once

#include "pch.h"
#include "Miniport Service Interface_h.h"
#include "Interfaces & Controllers/Topology Controller.h"

/// <summary>Encapsulates a queue storing asynchronous radio events and the ability to wait for a new one and process it.</summary>
/// <remarks>Allows our <see cref="IPortTunerDevice" /> and <see cref="CommandListener" /> to know about the queue without a circular dependency.</remarks>
class EventNotifier
{
	IMiniportFmRxDevice *& MiniportReceiveDevice;
	FmController & TopologyController;

	std::mutex QueueMutex;
	std::queue<Notification> Events;
	std::condition_variable NewEvent;

public:
	EventNotifier(IMiniportFmRxDevice *&, FmController &);

	/// <summary>Blocks until there is one new event in the queue, then processes it.</summary>
	/// <remarks>Should be called in a loop to continuously process asynchronous events.</remarks>
	/// <returns>A <see cref="Notification" /> structure with all relevant data needed by the client to update their UI for a given event.</returns>
	Notification AcquireEvent();

	void OnRadioEvent(HTUNER_ASYNCCTXT);
};