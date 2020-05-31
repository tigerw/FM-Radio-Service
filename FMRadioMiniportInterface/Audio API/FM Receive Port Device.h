#pragma once

#include "pch.h"

class FakePortFmRx : public IPortFmRxDevice
{
	[[ nodiscard ]]
	HRESULT STDMETHODCALLTYPE ProcessRdsGroup(
		/* [in] */ const RDS_GROUP * pRdsGroup) final override
	{
		std::wcout << "#2: Processing RDS group!" << std::endl;
		wprintf(L"FM: Miniport: block 1 is %x, block2 is %x, block3 is %x, block4 is %x\n",
			pRdsGroup->wBlockA, pRdsGroup->wBlockB, pRdsGroup->wBlockC, pRdsGroup->wBlockD);
		return S_OK;
	}

	[[ nodiscard ]]
	HRESULT STDMETHODCALLTYPE QueryInterface(
		[[ maybe_unused ]] /* [in] */ REFIID riid,
		[[ maybe_unused ]] /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) final override
	{
		return E_ILLEGAL_METHOD_CALL;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) final override
	{
		return ReferenceCounter.fetch_add(1);
	}

	ULONG STDMETHODCALLTYPE Release(void) final override
	{
		return ReferenceCounter.fetch_sub(1);
	}

private:
	std::atomic_ulong ReferenceCounter;
};