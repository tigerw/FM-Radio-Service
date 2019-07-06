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

	std::mutex EventsMutex;
	std::unordered_map<Client, std::bitset<2>> Events;
	std::condition_variable NewEvent;

	static const size_t FrequencyBit = 0;
	static const size_t PlayStateBit = 1;

	size_t AsyncContextToBit(HTUNER_ASYNCCTXT);

public:
	EventNotifier(IMiniportFmRxDevice *&, FmController &);

	/// <summary>Blocks until there is one new event in the queue, then processes it.</summary>
	/// <remarks>Should be called in a loop to continuously process asynchronous events.</remarks>
	/// <returns>A <see cref="Notification" /> structure with all relevant data needed by the client to update their UI for a given event.</returns>
	void AcquireEvent(Client, Notification *);

	void OnRadioEvent(HTUNER_ASYNCCTXT);

	void OnClientAdded(Client);
};

namespace NotifierHandles
{
	enum class AsyncContextHandle
	{
		/* Start at 1 since this is type-punned into a HANDLE pointer, and we all know what a 0 pointer means... */
		FrequencyChange = 1,
		PlayStateChange = 2
	};

	HTUNER_ASYNCCTXT AsyncContextToHANDLE(AsyncContextHandle);
}