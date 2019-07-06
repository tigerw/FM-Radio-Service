#include "pch.h"
#include "RPC Server.h"
#include "Command Listener.h"
#include "Checked Windows API Calls.h"
#include "Miniport Service Interface_s.c"

CommandListener * Server;

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

Notification AcquireEvent()
{
	while (true)
	{
		return Server->AcquireEvent();
	}
}

void QueueInitialStateEvents()
{
	return Server->QueueInitialStateEvents();
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

Notification CommandListener::AcquireEvent()
{
	return Notifier.AcquireEvent();
}

void CommandListener::QueueInitialStateEvents()
{
	Notifier.OnRadioEvent(reinterpret_cast<HANDLE>(1));
	Notifier.OnRadioEvent(reinterpret_cast<HANDLE>(1));
	Notifier.OnRadioEvent(reinterpret_cast<HANDLE>(2));
}

void CommandListener::EnableRadio()
{
	//TUNER_POWERSTATE Power;
	//auto PowerResult = MiniportTunerDevice->GetPowerState(&Power);
	//std::wcout << "Powerstate: " << Power << std::endl;
	//std::wcout << "Error message: " << std::error_code(PowerResult, std::system_category()).message() << std::endl;

	//std::wcout << "Now setting powerstate." << std::endl;
	TopologyController.SetFmState(true);
	Notifier.OnRadioEvent(reinterpret_cast<HANDLE>(2));
	//std::wcout << "Error message: " << std::error_code(PowerResult, std::system_category()).message() << std::endl;

	//PowerResult = MiniportTunerDevice->GetPowerState(&Power);
	//std::wcout << "Powerstate readback: " << Power << std::endl;
	//std::wcout << "Error message: " << std::error_code(PowerResult, std::system_category()).message() << std::endl;


	//std::wcout << "Setting frequency: 75 microsecond emphasis, European limits" << std::endl;
	//FM_REGIONPARAMS p;
	//p.Emphasis = FM_EMPHASIS::FM_EMPHASIS_75_USEC;
	//p.FrequencyMin = 87500;
	//p.FrequencyMax = 108000;
	//p.FrequencySpacing = 50;
	//auto res = devv->SetRegionParams(&p, INVALID_HANDLE_VALUE);
	//std::wcout << "Error message: " << std::error_code(res, std::system_category()).message() << std::endl;

	//std::wcout << "Tuning." << std::endl;
	//res = devv->Tune(87500, INVALID_HANDLE_VALUE);
	//std::wcout << "Error message: " << std::error_code(res, std::system_category()).message() << std::endl;

	//std::wcout << "Setting volume." << std::endl;
	//res = MiniportTuner->SetVolume(0);
	//std::wcout << "Error message: " << std::error_code(res, std::system_category()).message() << std::endl;

	//Sleep(1000);
	//std::wcout << "Test ran to conclusion. Streaming FM audio." << std::endl;
}

void CommandListener::DisableRadio()
{
	TopologyController.SetFmState(false);
	Notifier.OnRadioEvent(reinterpret_cast<HANDLE>(2));
}

void CommandListener::SeekForwards()
{
	Windows::CheckedMemberAPICall(MiniportReceiveDevice, &IMiniportFmRxDevice::Seek, FM_SEEKDIR::FM_SEEKDIR_FORWARD, reinterpret_cast<HANDLE>(1));
}

void CommandListener::SeekBackwards()
{
	Windows::CheckedMemberAPICall(MiniportReceiveDevice, &IMiniportFmRxDevice::Seek, FM_SEEKDIR::FM_SEEKDIR_BACKWARD, reinterpret_cast<HANDLE>(1));
}