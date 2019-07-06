#pragma once

#include "pch.h"
#include "Event Notifier.h"
#include "Miniport Service Interface_h.h"
#include "Audio API/FM Tuner Port Device.h"
#include "Interfaces & Controllers/Topology Controller.h"

class CommandListener
{
	using CreateAdapterDriver_t = HRESULT(IAdapterDriver ** Out);

	FakePortFmRx PortReceiveDevice;
	FakePortTuner PortTunerDevice;
	IMiniportTunerDevice * MiniportTunerDevice;

	/// <summary>Interface exposing finer-grained control of the radio.</summary>
	/// <remarks>An alias of <see cref="MiniportTunerDevice" /> for our Qualcomm implementation.</remarks>
	IMiniportFmRxDevice * MiniportReceiveDevice;

	FmController & TopologyController;

	EventNotifier Notifier;

public:
	CommandListener(FmController &);
	~CommandListener();

	/// <summary>Begin listening to clients that are long-polling for notifications.</summary>
	/// <remarks>This procedure is blocking.</remarks>
	void Listen();

	/// <summary>Returns a new radio event for the client to use to update its UI.</summary>
	/// <remarks>
	///  <para>Since UWAs can't host RPC servers, radio clients use long-polling to determine when there was a radio event.</para>
	///  <para>This function will block until a new event becomes available, then return a structure populated with all of that event's data.</para>
	///  <para>NB: the native radio API is inherently asynchronous.</para>
	/// </remarks>
	/// <returns>A <see cref="Notification" /> structure for the client to update its UI.</returns>
	/// <seealso cref="EventNotifier::AcquireEvent" />
	void AcquireEvent(Client, Notification *);

	Client AcquireClientId();

	void EnableRadio();
	void DisableRadio();
	void SeekForwards();
	void SeekBackwards();
};