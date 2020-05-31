#pragma once

#include "pch.h"
#include "Qualcomm Miniport Proxy.h"
#include "Miniport Service Interface_h.h"
#include "Interfaces & Controllers/Radio Topology.h"

/// <summary>Encapsulates a queue storing asynchronous radio events and the ability to wait for a new one and process it.</summary>
/// <remarks>Allows our <see cref="IPortTunerDevice" /> and <see cref="CommandListener" /> to know about the queue without a circular dependency.</remarks>
class EventNotifier
{
	enum class EventBit : size_t
	{
		Frequency = 0,
		PlayState = 1,
		Shutdown = 2,
		AntennaStatus = 3,
		RDS = 4,
		Count
	};

	IMiniportFmRxDevice *& MiniportReceiveDevice;
	RadioTopology & TopologyController;
	QualcommMiniportProxy & LibraryProxy;

	std::mutex EventsMutex;
	std::unordered_map<Client, std::bitset<static_cast<size_t>(EventBit::Count)>> Events;
	std::condition_variable NewEvent;

	EventBit AsyncContextToBit(HTUNER_ASYNCCTXT);

public:
	EventNotifier(IMiniportFmRxDevice *&, RadioTopology &, QualcommMiniportProxy &);

	/// <summary>Blocks until there is one new event in the queue, then processes it.</summary>
	/// <remarks>Should be called in a loop to continuously process asynchronous events.</remarks>
	/// <returns>A <see cref="Notification" /> structure with all relevant data needed by the client to update their UI for a given event.</returns>
	void AcquireEvent(Client, Notification *);

	void OnRadioEvent(HTUNER_ASYNCCTXT);

	void OnClientAdded(Client);

	void Shutdown();
};