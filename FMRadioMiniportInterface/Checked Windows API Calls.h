#pragma once

#include "pch.h"

namespace Windows
{
	template<typename FunctionType, typename ...ArgumentTypes>
	void CheckedBooleanAPICall(FunctionType Function, ArgumentTypes ...Arguments)
	{
		if (Function(Arguments...))
		{
			return;
		}

		throw std::system_error(::GetLastError(), std::system_category());
	}

	template<typename FunctionType, typename ...ArgumentTypes>
	void CheckedAPICall(FunctionType Function, ArgumentTypes ...Arguments)
	{
		const auto Result = Function(Arguments...);
		if (SUCCEEDED(Result))
		{
			return;
		}

		throw std::system_error(Result, std::system_category());
	}

	template<class ClassType, typename FunctionType, typename ...ArgumentTypes>
	void CheckedMemberAPICall(ClassType * Instance, FunctionType Function, ArgumentTypes ...Arguments)
	{
		const auto Result = (Instance->*Function)(Arguments...);
		if (SUCCEEDED(Result))
		{
			return;
		}

		throw std::system_error(Result, std::system_category());
	}

	template<typename FunctionType, typename ...ArgumentTypes>
	static void CheckedRPCCall(FunctionType Function, ArgumentTypes ...Arguments)
	{
		const auto Result = Function(Arguments...);
		if (Result == RPC_S_OK)
		{
			return;
		}

		throw std::system_error(MAKE_HRESULT(1, FACILITY_WIN32, Result), std::system_category());
	}

	template<typename FunctionType>
	static FunctionType * GetFunctionAddress(HMODULE StartAddress, LPCSTR ExportedName)
	{
		const auto FunctionAddress = reinterpret_cast<FunctionType *>(::GetProcAddress(StartAddress, ExportedName));
		if (FunctionAddress == nullptr)
		{
			throw std::system_error(::GetLastError(), std::system_category());
		}

		return FunctionAddress;
	}
};