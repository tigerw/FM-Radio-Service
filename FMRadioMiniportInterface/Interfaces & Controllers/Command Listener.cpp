#include "pch.h"
#include "RPC Server.h"
#include "Command Listener.h"
#include "Checked Windows API Calls.h"
#include "Miniport Service Interface_s.c"

CommandListener * Server;

void __RPC_FAR* __RPC_USER midl_user_allocate(size_t len)
{
	return operator new(len);
}

void __RPC_USER midl_user_free(void __RPC_FAR* ptr)
{
	delete ptr;
}

void DisableRadio()
{
	Server->DisableRadio();
}

void SeekForwards()
{
	Server->SeekForwards();
}

void SeekBackwards()
{
	Server->SeekBackwards();
}

void EnableRadio()
{
	Server->EnableRadio();
}

void AcquireEvent(Client ClientId, Notification * Event)
{
	Server->AcquireEvent(ClientId, Event);
}

Client AcquireClientId()
{
	return Server->AcquireClientId();
}

CommandListener::CommandListener(FmController & Controller) :
	PortTunerDevice(PortReceiveDevice, Notifier),
	TopologyController(Controller),
	Notifier(MiniportReceiveDevice, Controller)
{
	Server = this;

	const auto FMSLMiniportAddress = LoadLibrary(L"qcfmslminiport.dll");
	if (FMSLMiniportAddress == nullptr)
	{
		throw std::system_error(::GetLastError(), std::system_category());
	}

	const auto CreateAdapterAddress = Windows::GetFunctionAddress<CreateAdapterDriver_t>(FMSLMiniportAddress, "CreateAdapterDriver");

	IAdapterDriver * Driver;
	Windows::CheckedAPICall(CreateAdapterAddress, &Driver);
	Windows::CheckedMemberAPICall(Driver, &IAdapterDriver::CreateMiniportTunerDevice, 0, 0, &PortTunerDevice, &MiniportTunerDevice);

	// TODO: check for errors
	MiniportTunerDevice->QueryInterface(IID_IMiniportFmRxDevice, reinterpret_cast<void **>(&MiniportReceiveDevice));

	Windows::CheckedMemberAPICall(MiniportTunerDevice, &IMiniportTunerDevice::SetPowerState, TUNER_POWERSTATE::TUNER_POWERSTATE_ON, INVALID_HANDLE_VALUE);

	FM_REGIONPARAMS p;
	p.Emphasis = FM_EMPHASIS::FM_EMPHASIS_75_USEC;
	p.FrequencyMin = 87500;
	p.FrequencyMax = 108000;
	p.FrequencySpacing = 50;
	Windows::CheckedMemberAPICall(MiniportReceiveDevice, &IMiniportFmRxDevice::SetRegionParams, &p, INVALID_HANDLE_VALUE);
}

CommandListener::~CommandListener()
{
	MiniportTunerDevice->Release();
}

void CommandListener::Listen()
{
	wchar_t Endpoint[] = L"FM Radio Miniport Service Interface";
	RPCServer::Listen(MiniportServiceInterface_v1_0_s_ifspec, Endpoint);
}

void CommandListener::AcquireEvent(Client ClientId, Notification * Event)
{
	Notifier.AcquireEvent(ClientId, Event);
}

Client CommandListener::AcquireClientId()
{
	const auto ClientId = rand();
	Notifier.OnClientAdded(ClientId);

	using namespace NotifierHandles;
	Notifier.OnRadioEvent(AsyncContextToHANDLE(AsyncContextHandle::FrequencyChange));
	Notifier.OnRadioEvent(AsyncContextToHANDLE(AsyncContextHandle::PlayStateChange));

	return ClientId;
}

void CommandListener::EnableRadio()
{
	TopologyController.SetFmState(true);

	using namespace NotifierHandles;
	Notifier.OnRadioEvent(AsyncContextToHANDLE(AsyncContextHandle::PlayStateChange));
}

void CommandListener::DisableRadio()
{
	TopologyController.SetFmState(false);

	using namespace NotifierHandles;
	Notifier.OnRadioEvent(AsyncContextToHANDLE(AsyncContextHandle::PlayStateChange));
}

void CommandListener::SeekForwards()
{
	Windows::CheckedMemberAPICall(
		MiniportReceiveDevice,
		&IMiniportFmRxDevice::Seek,
		FM_SEEKDIR::FM_SEEKDIR_FORWARD,
		NotifierHandles::AsyncContextToHANDLE(NotifierHandles::AsyncContextHandle::FrequencyChange)
	);
}

void CommandListener::SeekBackwards()
{
	Windows::CheckedMemberAPICall(
		MiniportReceiveDevice,
		&IMiniportFmRxDevice::Seek,
		FM_SEEKDIR::FM_SEEKDIR_BACKWARD,
		NotifierHandles::AsyncContextToHANDLE(NotifierHandles::AsyncContextHandle::FrequencyChange)
	);
}