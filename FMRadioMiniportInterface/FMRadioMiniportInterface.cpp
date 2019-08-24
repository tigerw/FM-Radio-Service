#include "pch.h"
#include "Checked Windows API Calls.h"
#include "Interfaces & Controllers/Command Listener.h"

SERVICE_STATUS GlobalServiceStatus;
CommandListener * GlobalCommandListener;
SERVICE_STATUS_HANDLE GlobalServiceStatusHandle;

std::mutex GlobalStopMutex;
std::condition_variable GlobalStopSignal;
std::atomic_flag GlobalShouldContinue = ATOMIC_FLAG_INIT;

VOID WINAPI ServiceCtrlHandler(const DWORD CtrlCode)
{
	if (CtrlCode != SERVICE_CONTROL_STOP)
	{
		return;
	}

	/*
	* Perform tasks neccesary to prepare to stop the service here
	*/

	// This will signal the worker thread to start shutting down
	// Everything should be nothrow
	GlobalShouldContinue.clear();
	GlobalStopSignal.notify_one();
}

void ServiceWorker()
{
	// Tell the service controller we are starting
	GlobalServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	GlobalServiceStatus.dwControlsAccepted = 0;
	GlobalServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	GlobalServiceStatus.dwCheckPoint = 0;
	Windows::CheckedBooleanAPICall(SetServiceStatus, GlobalServiceStatusHandle, &GlobalServiceStatus);

	/*
	 * Perform tasks neccesary to start the service here
	 */

	auto ListenResult = std::async(
		std::launch::async,
		[] {
			RadioTopology FM;
			FM.SetFmVolume(-1000000);

			CommandListener Server(FM);
			GlobalCommandListener = &Server;
			Server.Listen();

			// Getting here means we are about to stop, but don't exit until ServiceWorker has finished the Shutdown call
			{
				std::unique_lock<decltype(GlobalStopMutex)> Guard(GlobalStopMutex);
				GlobalStopSignal.wait(Guard, [] { return !GlobalShouldContinue.test_and_set(); });
			}
		}
	);

	do
	{
		if (ListenResult.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			// Something went wrong, we shouldn't be ready since the thread should be blocked listening
			// Get any exceptions, this *should* be a _throw_ guarantee (lol)
			ListenResult.get();
		}

		// eh method to wait until the server is actually listening
		// CommandListener delays 10 seconds for qcfmslminiport.dll but check every two for that nice fake progress bar
		// Note: if the worker thread is a bit slow to start listening, and then the CtrlHandler receives a stop, this ensures the RPC stuff still stops
		// (this is because CtrlHandler will not signal until we say the service is running)
		GlobalServiceStatus.dwCheckPoint++;
		Windows::CheckedBooleanAPICall(SetServiceStatus, GlobalServiceStatusHandle, &GlobalServiceStatus);
		std::this_thread::sleep_for(std::chrono::seconds(2));

	} while (RpcMgmtIsServerListening(nullptr) != RPC_S_OK);

	// Initialise the flag
	GlobalShouldContinue.test_and_set();

	// Tell the service controller we are started
	GlobalServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	GlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
	Windows::CheckedBooleanAPICall(SetServiceStatus, GlobalServiceStatusHandle, &GlobalServiceStatus);

	// Wait until our worker thread exits effectively signaling that the service needs to stop
	{
		std::unique_lock<decltype(GlobalStopMutex)> Guard(GlobalStopMutex);
		GlobalStopSignal.wait(Guard, [] { return !GlobalShouldContinue.test_and_set(); });
	}

	// Tell the service controller we are stopping
	GlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
	GlobalServiceStatus.dwCheckPoint++;
	Windows::CheckedBooleanAPICall(SetServiceStatus, GlobalServiceStatusHandle, &GlobalServiceStatus);

	// Signal the server to stop listening and unblock
	// Re-use the ShouldContinue flag to prevent the worker thread from exiting until we've finished cleanup
	GlobalShouldContinue.test_and_set();
	GlobalCommandListener->Shutdown();
	GlobalShouldContinue.clear();
	GlobalStopSignal.notify_one();

	// Get any exceptions, may throw
	ListenResult.get();
}

VOID WINAPI ServiceMain(DWORD, LPTSTR *)
{
	static wchar_t ServiceName[] = L"FM Radio Miniport Interface";
	GlobalServiceStatusHandle = RegisterServiceCtrlHandler(ServiceName, ServiceCtrlHandler);

	if (GlobalServiceStatusHandle == nullptr)
	{
		throw std::system_error(::GetLastError(), std::system_category());
	}

	std::optional<DWORD> ReturnValue;
	try
	{
		ServiceWorker();
	}
	catch (std::system_error & Error)
	{
		ReturnValue = static_cast<DWORD>(Error.code().value());
	}
	catch (...)
	{
		ReturnValue = E_FAIL;
	}

	/*
	 * Perform any cleanup tasks
	 */
	GlobalServiceStatus.dwControlsAccepted = 0;
	GlobalServiceStatus.dwCurrentState = SERVICE_STOPPED;
	GlobalServiceStatus.dwWin32ExitCode = ReturnValue.value_or(S_OK);
	Windows::CheckedBooleanAPICall(SetServiceStatus, GlobalServiceStatusHandle, &GlobalServiceStatus); // If this fails we're hosed
}

int main()
{
	wchar_t ServiceName[] = L"FM Radio Miniport Interface";
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ ServiceName, reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(ServiceMain) },
		{ nullptr, nullptr }
	};

	if (!StartServiceCtrlDispatcher(ServiceTable))
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}