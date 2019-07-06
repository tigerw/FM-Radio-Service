#include "pch.h"
#include "Interfaces & Controllers/Command Listener.h"

int main()
{
	try
	{
		FmController FM;
		const auto TopologyInitialiseResult = FM.Initialize();
		// std::wcout << "Error message: " << std::error_code(TopologyInitialiseResult, std::system_category()).message() << std::endl;

		FM.SetFmState(true);
		FM.SetFmVolume(-1000000);

		CommandListener Server(FM);
		Server.Listen();
	}
	catch (std::system_error& Error)
	{
		std::wcerr << "Failure. " << Error.what() << std::endl;
		std::cin.get();
	}
	catch (...)
	{
		std::wcerr << "Unknown failure." << std::endl;
		std::cin.get();
	}
}