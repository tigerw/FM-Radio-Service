#pragma once

#include "pch.h"
#include "Checked Windows API Calls.h"

class RPCServer
{
public:
	static void Listen(RPC_IF_HANDLE ServerHandle, wchar_t Endpoint[])
	{
		// An array of bytes with automatic storage duration to hold the security identifier for UWP AppContainers
		// Could also be dynamically allocated with operator new
		char AppContainerSID[SECURITY_MAX_SID_SIZE];

		// Size in bytes of the previous array.
		// Later API calls will modify this to be the actual SID size, though we don't use this information
		size_t AppContainerSIDSize = sizeof(AppContainerSID);

		// Populate the SID with AppContainer so we can later grant UWP applications access to our RPC endpoint
		Windows::CheckedBooleanAPICall(
			CreateWellKnownSid,
			WELL_KNOWN_SID_TYPE::WinBuiltinAnyPackageSid,
			nullptr,
			AppContainerSID,
			reinterpret_cast<DWORD *>(&AppContainerSIDSize)
		);

		// SID representing everyone else
		// Essentially, everyone has access to this RPC endpoint
		char WorldEveryoneSID[SECURITY_MAX_SID_SIZE];
		size_t WorldEveryoneSIDSize = sizeof(WorldEveryoneSID);
		Windows::CheckedBooleanAPICall(
			CreateWellKnownSid,
			WELL_KNOWN_SID_TYPE::WinWorldSid,
			nullptr,
			WorldEveryoneSID,
			reinterpret_cast<DWORD *>(&WorldEveryoneSIDSize)
		);

		// Define what is allowed to access our RPC server
		// We will have both World and AppContainers granted permission
		std::array<EXPLICIT_ACCESS, 2> Permissions;
		{
			Permissions[0].grfAccessMode = SET_ACCESS;
			Permissions[0].grfAccessPermissions = GENERIC_ALL;
			Permissions[0].grfInheritance = NO_INHERITANCE;
			Permissions[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
			Permissions[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			Permissions[0].Trustee.ptstrName = reinterpret_cast<LPWSTR>(WorldEveryoneSID);
			Permissions[1].grfAccessMode = SET_ACCESS;
			Permissions[1].grfAccessPermissions = GENERIC_ALL;
			Permissions[1].grfInheritance = NO_INHERITANCE;
			Permissions[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
			Permissions[1].Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
			Permissions[1].Trustee.ptstrName = reinterpret_cast<LPWSTR>(AppContainerSID);
		}

		// Populate an access control list for the above permissions
		PACL AllowedAccessors;
		Windows::CheckedAPICall(
			SetEntriesInAcl,
			static_cast<ULONG>(Permissions.size()),
			Permissions.data(),
			nullptr,
			&AllowedAccessors
		);

		// Now prepare a security descriptor for the RPC stuff
		SECURITY_DESCRIPTOR Descriptor;
		Windows::CheckedBooleanAPICall(
			InitializeSecurityDescriptor,
			&Descriptor,
			SECURITY_DESCRIPTOR_REVISION
		);
		Windows::CheckedBooleanAPICall(
			SetSecurityDescriptorDacl,
			&Descriptor,
			true,
			AllowedAccessors,
			false
		);

		// Protocol for the RPCs. The API doesn't take consts so must have allocated strings (same thing for Endpoint)
		// Also apparently it's the fastest protocol, so use this instead of e.g. pipes
		wchar_t ProtocolSequence[] = L"ncalrpc";

		// OK! Put everything in motion
		Windows::CheckedRPCCall(
			RpcServerUseProtseqEp,
			reinterpret_cast<RPC_WSTR>(ProtocolSequence),
			RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
			reinterpret_cast<RPC_WSTR>(Endpoint),
			&Descriptor
		);

		// Do registration voodoo
		Windows::CheckedRPCCall(
			RpcServerRegisterIf3,
			ServerHandle,
			nullptr,
			nullptr,
			RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_LOCAL_ONLY,
			RPC_C_LISTEN_MAX_CALLS_DEFAULT,
			0,
			nullptr,
			&Descriptor
		);

		// Start a blocking listen
		Windows::CheckedRPCCall(
			RpcServerListen,
			1,
			RPC_C_LISTEN_MAX_CALLS_DEFAULT,
			false
		);
	}
};