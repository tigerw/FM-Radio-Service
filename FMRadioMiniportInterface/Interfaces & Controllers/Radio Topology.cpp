#include "pch.h"
#include "Radio Topology.h"
#include <Endpointvolume.h>
#include "Checked Windows API Calls.h"

const CLSID CLSID_MMDeviceEnumerator = { 0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E } };

RadioTopology::RadioTopology() :
	m_CommunicationSpeakers(nullptr),
	Event(m_CommunicationSpeakers, OnAntennaStatusChange)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (FAILED(hr))
	{
		std::cout << "CoInitializeEx failed: " << std::error_code(hr, std::system_category()).message() << std::endl;
		throw;
	}

	CComPtr<IMMDeviceEnumerator> spEnumerator;
	CComPtr<IMMDeviceCollection> spMMDeviceCollection;
	UINT deviceCount = 0;

	hr = spEnumerator.CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_INPROC_SERVER);
	hr = spEnumerator->RegisterEndpointNotificationCallback(&Event);

	if (SUCCEEDED(hr))
	{
		hr = spEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &spMMDeviceCollection);
	}

	if (SUCCEEDED(hr))
	{
		hr = spMMDeviceCollection->GetCount(&deviceCount);
	}

	// This loop will iterate through all the endpoints and look the KSNODETYPE_FM_RX endpoint
	// For this endpoint it will then find a topology filter and a wave filter and activate IKSControl on these filters.
	for (UINT i = 0; i < deviceCount; i++)
	{
		CComPtr<IMMDevice>       spEndpoint;
		CComPtr<IMMDevice>       spDevice;
		CComPtr<IPart>           spTopologyFilterEndpointConnectorPart;
		CComPtr<IDeviceTopology> spEndpointTopology;
		CComPtr<IConnector>      spEndpointConnector;
		CComPtr<IConnector>      spTopologyFilterEndpointConnector;
		CComPtr<IDeviceTopology> spAdapterTopology;
		CComPtr<IKsControl>      spWaveKsControl;
		CComHeapPtr<wchar_t>     wszAdapterTopologyDeviceId;
		CComPtr<IMMDevice>       spAdapterDevice;
		GUID                     guidEndpointClass = GUID_NULL;
		CComPtr<IConnector>      spConnector;
		CComPtr<IConnector>      spConConnectedTo;
		CComPtr<IPart>           spConnectedPart;
		CComPtr<IDeviceTopology> spMiniportTopology;
		CComHeapPtr<wchar_t>     wszPhoneTopologyDeviceId;
		CComPtr<IMMDevice>       spWaveAdapterDevice;
		CComPtr<IPartsList>      spPartsList;
		CComPtr<IPart>           spPart;

		if (SUCCEEDED(hr))
		{
			hr = spMMDeviceCollection->Item(i, &spEndpoint);
		}

		// Get the IDeviceTopology interface from the endpoint device.
		if (SUCCEEDED(hr))
		{
			hr = spEndpoint->Activate(
				__uuidof(IDeviceTopology),
				CLSCTX_ALL,
				NULL,
				(VOID**)&spEndpointTopology);
		}

		// Get the default connector for the endpoint device which is connected to
		// the connector on the topology filter.
		if (SUCCEEDED(hr))
		{
			hr = spEndpointTopology->GetConnector(0, &spEndpointConnector);
		}

		// Get the endpoint connector on the topology filter.
		if (SUCCEEDED(hr))
		{
			hr = spEndpointConnector->GetConnectedTo(&spTopologyFilterEndpointConnector);
		}

		// Get the endpoint connector part, endpoint class, and local ID.
		if (SUCCEEDED(hr))
		{
			hr = spTopologyFilterEndpointConnector.QueryInterface(&spTopologyFilterEndpointConnectorPart);
		}

		if (SUCCEEDED(hr))
		{
			hr = spTopologyFilterEndpointConnectorPart->GetSubType(&guidEndpointClass);
		}

		// If this is the FM_RX node we've found the FM topology
		if (SUCCEEDED(hr))
		{
			if (guidEndpointClass == KSNODETYPE_HEADSET_SPEAKERS) // or COMMUNICATION_SPEAKER
			{
				CommunicationSpeakersPart = spTopologyFilterEndpointConnectorPart;
				hr = spEndpoint->GetId(&m_CommunicationSpeakers);
				continue;
			}

			if (guidEndpointClass != KSNODETYPE_FM_RX)
			{
				continue;
			}
		}

		// Get the IDeviceTopology interface for the adapter device from the endpoint connector part.
		if (SUCCEEDED(hr))
		{
			hr = spTopologyFilterEndpointConnectorPart->GetTopologyObject(&spAdapterTopology);
		}

		// Activate the KSControl on topo filter
		if (SUCCEEDED(hr))
		{
			hr = spAdapterTopology->GetDeviceId(&wszAdapterTopologyDeviceId);
		}

		if (SUCCEEDED(hr))
		{
			hr = spEnumerator->GetDevice(wszAdapterTopologyDeviceId, &spAdapterDevice);
		}

		if (SUCCEEDED(hr))
		{
			/*IAudioEndpointVolume* Client;
			Windows::CheckedMemberAPICall(&(*spEndpoint), &IMMDevice::Activate, __uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)& Client);*/

			hr = spAdapterDevice->Activate(
				__uuidof(IKsControl),
				CLSCTX_ALL,
				NULL,
				(VOID**)&m_spTopologyKsControl);

			if (FAILED(hr))
			{
				std::wcerr << L"Activating KsControl for Topo filter failed. hr=" << hr << std::endl;
			}
		}

		// Now proceed with enumerating the rest of the topology
		if (SUCCEEDED(hr))
		{
			hr = spTopologyFilterEndpointConnectorPart->EnumPartsOutgoing(&spPartsList);
		}

		if (SUCCEEDED(hr))
		{
			hr = spPartsList->GetPart(0, &spPart);
		}

		if (SUCCEEDED(hr))
		{
			hr = spPart.QueryInterface(&spConnector);
		}

		if (SUCCEEDED(hr))
		{
			hr = spConnector->GetConnectedTo(&spConConnectedTo);
		}

		// retrieve the part that this connector is on, which should be a part
		// on the miniport.
		if (SUCCEEDED(hr))
		{
			hr = spConConnectedTo.QueryInterface(&spConnectedPart);
		}

		if (SUCCEEDED(hr))
		{
			hr = spConnectedPart->GetTopologyObject(&spMiniportTopology);
		}

		if (SUCCEEDED(hr))
		{
			hr = spMiniportTopology->GetDeviceId(&wszPhoneTopologyDeviceId);
		}

		if (SUCCEEDED(hr))
		{
			hr = spEnumerator->GetDevice(wszPhoneTopologyDeviceId, &spWaveAdapterDevice);
		}

		if (SUCCEEDED(hr))
		{
			hr = spWaveAdapterDevice->Activate(
				__uuidof(IKsControl),
				CLSCTX_ALL,
				NULL,
				(VOID**)&spWaveKsControl);

			if (FAILED(hr))
			{
				std::wcerr << L"Activating KsControl for Wave filter failed. hr =" << hr << std::endl;
			}
		}

		if (SUCCEEDED(hr))
		{
			m_spWaveKsControl = spWaveKsControl;
		}
	}

	if (m_CommunicationSpeakers == nullptr)
	{
		throw;
	}

	if (FAILED(hr))
	{
		std::wcerr << "Initialization failed with hr=" << hr << std::endl;
		throw;
	}
}

// Sets fm volume. This is a property on topology filter.
HRESULT RadioTopology::SetFmVolume(_In_ LONG volume)
{
    HRESULT hr = S_OK;
    KSPROPERTY volumeProp;
    ULONG ulBytesReturned;
    KSPROPERTY_DESCRIPTION desc = { 0 };
    KSPROPERTY_DESCRIPTION * pdesc = NULL;
    PKSPROPERTY_MEMBERSHEADER pKsPropMembHead = NULL;
    PKSPROPERTY_STEPPING_LONG pKsPropStepLong = NULL;

    if (m_spTopologyKsControl == NULL)
    {
        hr = E_POINTER;
    }

    // Determine valid volume ranges
    if (SUCCEEDED(hr))
    {
        KSPROPERTY volumeSupportProp;
        ULONG cbSupportProp = 0;

        volumeSupportProp.Set = KSPROPSETID_FMRXTopology;
        volumeSupportProp.Id = KSPROPERTY_FMRX_VOLUME;
        volumeSupportProp.Flags = KSPROPERTY_TYPE_BASICSUPPORT;

        hr = m_spTopologyKsControl->KsProperty(
            (PKSPROPERTY)&volumeSupportProp,
            sizeof(volumeSupportProp),
            &desc,
            sizeof(desc),
            &cbSupportProp
            );

        if (SUCCEEDED(hr) && desc.DescriptionSize > sizeof(desc))
        {
            pdesc = (KSPROPERTY_DESCRIPTION*)CoTaskMemAlloc(desc.DescriptionSize);
            if (NULL == pdesc)
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                hr = m_spTopologyKsControl->KsProperty(
                    (PKSPROPERTY)&volumeSupportProp,
                    sizeof(volumeSupportProp),
                    pdesc,
                    desc.DescriptionSize,
                    &cbSupportProp
                    );
            }

            if (SUCCEEDED(hr))
            {
                if (pdesc->PropTypeSet.Set != KSPROPTYPESETID_General ||
                    pdesc->PropTypeSet.Id != VT_I4 ||
                    pdesc->PropTypeSet.Flags != 0 ||
                    pdesc->MembersListCount < 1 ||
                    pdesc->Reserved != 0 ||
                    pdesc->DescriptionSize < (sizeof(KSPROPERTY_DESCRIPTION)+sizeof(KSPROPERTY_MEMBERSHEADER)+sizeof(KSPROPERTY_STEPPING_LONG)))
                {
					wprintf(L"Full volume basicsupport Property Set is invalid or isn't large enough to include stepping information\n");
                    hr = E_INVALIDARG;
                }
            }

            if (SUCCEEDED(hr))
            {
                pKsPropMembHead = reinterpret_cast<PKSPROPERTY_MEMBERSHEADER>(pdesc + 1);
                ULONG flags = pKsPropMembHead->Flags & (KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL | KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_UNIFORM);
                if (pKsPropMembHead->MembersFlags != KSPROPERTY_MEMBER_STEPPEDRANGES ||
                    pKsPropMembHead->MembersSize < sizeof(KSPROPERTY_STEPPING_LONG) ||
                    pKsPropMembHead->MembersCount < 1 ||
                    (flags != (KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL | KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_UNIFORM)
                    && flags != 0))
                {
					wprintf(L"Volume basicsupport Property members invalid\n");
                    hr = E_INVALIDARG;
                }
            }

            if (SUCCEEDED(hr))
            {
                pKsPropStepLong = reinterpret_cast<PKSPROPERTY_STEPPING_LONG>(pKsPropMembHead + 1);

                // Round volume to nearest supported value
                volume = VALUE_NORMALIZE_IN_RANGE_EX(
                    volume,
                    pKsPropStepLong->Bounds.SignedMinimum,
                    pKsPropStepLong->Bounds.SignedMaximum,
                    (LONG)(pKsPropStepLong->SteppingDelta)
                    );

				wprintf(L"Applying driver stepping: %d %d %d (step, min, max); new volume = %d\n",
                    pKsPropStepLong->SteppingDelta,
                    pKsPropStepLong->Bounds.SignedMinimum,
                    pKsPropStepLong->Bounds.SignedMaximum,
                    volume
                    );
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        volumeProp.Set = KSPROPSETID_FMRXTopology;
        volumeProp.Id = KSPROPERTY_FMRX_VOLUME;
        volumeProp.Flags = KSPROPERTY_TYPE_SET;

        hr = m_spTopologyKsControl->KsProperty(
            (PKSPROPERTY)&volumeProp,
            sizeof(volumeProp),
            &volume,
            sizeof(LONG),
            &ulBytesReturned
            );
    }

    if (pdesc)
    {
        CoTaskMemFree(pdesc);
    }
    return hr;
}


// Returns current fm volume. This is a property on topology filter.
HRESULT RadioTopology::GetFmVolume(_Out_ LONG* plVolume)
{
    HRESULT hr = S_OK;
    KSPROPERTY volumeProp;
    ULONG ulBytesReturned;
    LONG vol = 0;

    if (plVolume == NULL || m_spTopologyKsControl == NULL)
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        volumeProp.Set = KSPROPSETID_FMRXTopology;
        volumeProp.Id = KSPROPERTY_FMRX_VOLUME;
        volumeProp.Flags = KSPROPERTY_TYPE_GET;

        hr = m_spTopologyKsControl->KsProperty(
            (PKSPROPERTY)&volumeProp,
            sizeof(volumeProp),
            &vol,
            sizeof(LONG),
            &ulBytesReturned
            );
    }

    if (SUCCEEDED(hr))
    {
        *plVolume = vol;
    }

    return hr;
}

// Sets fm endpoint id. Takes index
HRESULT RadioTopology::SetFmEndpointId(_In_ LONG index)
{
    HRESULT hr = S_OK;
    KSPROPERTY endpointIdProp;
    ULONG ulBytesReturned;
    KSPROPERTY_DESCRIPTION desc = { 0 };
    KSPROPERTY_DESCRIPTION * pdesc = NULL;
    PKSPROPERTY_MEMBERSHEADER pKsPropMembHead = NULL;
    PKSTOPOLOGY_ENDPOINTID pKsPropEndpointId = NULL;
    ULONG endpointCount = 0;

    if (m_spTopologyKsControl == NULL)
    {
        hr = E_POINTER;
    }

    // Determine valid endpoints
    if (SUCCEEDED(hr))
    {
        KSPROPERTY endpointIdSupportProp;
        ULONG cbSupportProp = 0;

        endpointIdSupportProp.Set = KSPROPSETID_FMRXTopology;
        endpointIdSupportProp.Id = KSPROPERTY_FMRX_ENDPOINTID;
        endpointIdSupportProp.Flags = KSPROPERTY_TYPE_BASICSUPPORT;

        hr = m_spTopologyKsControl->KsProperty(
            (PKSPROPERTY)&endpointIdSupportProp,
            sizeof(endpointIdSupportProp),
            &desc,
            sizeof(desc),
            &cbSupportProp
            );

        if (SUCCEEDED(hr) && desc.DescriptionSize > sizeof(desc))
        {
            pdesc = (KSPROPERTY_DESCRIPTION*)CoTaskMemAlloc(desc.DescriptionSize);
            if (NULL == pdesc)
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                hr = m_spTopologyKsControl->KsProperty(
                    (PKSPROPERTY)&endpointIdSupportProp,
                    sizeof(endpointIdSupportProp),
                    pdesc,
                    desc.DescriptionSize,
                    &cbSupportProp
                    );
            }

            if (SUCCEEDED(hr))
            {
                if (pdesc->PropTypeSet.Set != KSPROPSETID_FMRXTopology ||
                    pdesc->PropTypeSet.Id != KSPROPERTY_FMRX_ENDPOINTID ||
                    pdesc->PropTypeSet.Flags != 0 ||
                    pdesc->MembersListCount < 1 ||
                    pdesc->Reserved != 0 ||
                    pdesc->DescriptionSize < (sizeof(KSPROPERTY_DESCRIPTION)+sizeof(KSPROPERTY_MEMBERSHEADER)+sizeof(KSTOPOLOGY_ENDPOINTID)))
                {
					wprintf(L"Full endpointid basicsupport Property Set is invalid or isn't large enough to include at least 1 endpoint id\n");
                    hr = E_INVALIDARG;
                }
            }

            if (SUCCEEDED(hr))
            {
                pKsPropMembHead = reinterpret_cast<PKSPROPERTY_MEMBERSHEADER>(pdesc + 1);
                if (pKsPropMembHead->MembersFlags != KSPROPERTY_MEMBER_VALUES ||
                    pKsPropMembHead->MembersSize < sizeof(KSTOPOLOGY_ENDPOINTID) ||
                    pKsPropMembHead->MembersCount < 1 ||
                    pKsPropMembHead->Flags != 0)
                {
					wprintf(L"FM Endpoint basicsupport Property members invalid\n");
                    hr = E_INVALIDARG;
                }
                endpointCount = pKsPropMembHead->MembersCount;
            }

            if (SUCCEEDED(hr))
            {
                pKsPropEndpointId = reinterpret_cast<PKSTOPOLOGY_ENDPOINTID>(pKsPropMembHead + 1);

                for (ULONG i = 0; i < endpointCount; ++i)
                {
					wprintf(L"FM Endpoint %d: %s (%d)\n", i, pKsPropEndpointId[i].TopologyName, pKsPropEndpointId[i].PinId);
                }
            }
        }
    }

    if ((ULONG)(index) >= endpointCount)
    {
        wprintf(L"FM Endpoint index invalid, must be in [0 - %d]\n", endpointCount - 1);
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        endpointIdProp.Set = KSPROPSETID_FMRXTopology;
        endpointIdProp.Id = KSPROPERTY_FMRX_ENDPOINTID;
        endpointIdProp.Flags = KSPROPERTY_TYPE_SET;

        hr = m_spTopologyKsControl->KsProperty(
            (PKSPROPERTY)&endpointIdProp,
            sizeof(endpointIdProp),
            &pKsPropEndpointId[index],
            sizeof(KSTOPOLOGY_ENDPOINTID),
            &ulBytesReturned
            );
    }

    if (pdesc)
    {
        CoTaskMemFree(pdesc);
    }
    return hr;
}

HRESULT RadioTopology::GetFmEndpointId(_Out_ PKSTOPOLOGY_ENDPOINTID pEndpointId)
{
    HRESULT hr = S_OK;
    KSPROPERTY endpointIdProp;
    ULONG ulBytesReturned;
    KSTOPOLOGY_ENDPOINTID ksEndpointId = { 0 };


    if (pEndpointId == NULL || m_spTopologyKsControl == NULL)
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        endpointIdProp.Set = KSPROPSETID_FMRXTopology;
        endpointIdProp.Id = KSPROPERTY_FMRX_ENDPOINTID;
        endpointIdProp.Flags = KSPROPERTY_TYPE_GET;

        hr = m_spTopologyKsControl->KsProperty(
            (PKSPROPERTY)&endpointIdProp,
            sizeof(endpointIdProp),
            &ksEndpointId,
            sizeof(KSTOPOLOGY_ENDPOINTID),
            &ulBytesReturned
            );
    }

    if (SUCCEEDED(hr))
    {
        memcpy_s(pEndpointId, sizeof(*pEndpointId), &ksEndpointId, sizeof(*pEndpointId));
    }

    return hr;
}

HRESULT RadioTopology::GetFmAntennaEndpointId(_Out_ PKSTOPOLOGY_ENDPOINTID pEndpointId)
{
    HRESULT hr = S_OK;
    KSPROPERTY endpointIdProp;
    ULONG ulBytesReturned;
    KSTOPOLOGY_ENDPOINTID ksEndpointId = { 0 };


    if (pEndpointId == NULL || m_spTopologyKsControl == NULL)
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        endpointIdProp.Set = KSPROPSETID_FMRXTopology;
        endpointIdProp.Id = KSPROPERTY_FMRX_ANTENNAENDPOINTID;
        endpointIdProp.Flags = KSPROPERTY_TYPE_GET;

        hr = m_spTopologyKsControl->KsProperty(
            (PKSPROPERTY)&endpointIdProp,
            sizeof(endpointIdProp),
            &ksEndpointId,
            sizeof(KSTOPOLOGY_ENDPOINTID),
            &ulBytesReturned
            );
    }

    if (SUCCEEDED(hr))
    {
        memcpy_s(pEndpointId, sizeof(*pEndpointId), &ksEndpointId, sizeof(*pEndpointId));
    }

    return hr;
}

HRESULT RadioTopology::SetFmState(_In_ BOOL state)
{
    HRESULT hr = S_OK;
    KSPROPERTY stateProp;
    ULONG ulBytesReturned;

    if (m_spWaveKsControl == NULL)
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        stateProp.Set = KSPROPSETID_FMRXControl;
        stateProp.Id = KSPROPERTY_FMRX_STATE;
        stateProp.Flags = KSPROPERTY_TYPE_SET;

        hr = m_spWaveKsControl->KsProperty(
            (PKSPROPERTY)&stateProp,
            sizeof(stateProp),
            &state,
            sizeof(BOOL),
            &ulBytesReturned
            );
    }

    return hr;
}
HRESULT RadioTopology::GetFmState(_Out_ BOOL * pbState)
{
    HRESULT hr = S_OK;
    KSPROPERTY stateProp;
    ULONG ulBytesReturned;
    BOOL state = 0;

    if (pbState == NULL || m_spWaveKsControl == NULL)
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        stateProp.Set = KSPROPSETID_FMRXControl;
        stateProp.Id = KSPROPERTY_FMRX_STATE;
        stateProp.Flags = KSPROPERTY_TYPE_GET;

        hr = m_spWaveKsControl->KsProperty(
            (PKSPROPERTY)&stateProp,
            sizeof(stateProp),
            &state,
            sizeof(BOOL),
            &ulBytesReturned
            );
    }

    if (SUCCEEDED(hr))
    {
        *pbState = state;
    }

    return hr;
}

bool RadioTopology::IsAntennaPresent()
{
	IKsJackDescription * Jack;
	CommunicationSpeakersPart->Activate(CLSCTX_INPROC_SERVER, __uuidof(IKsJackDescription), reinterpret_cast<void **>(&Jack));
	KSJACK_DESCRIPTION Description;
	Jack->GetJackDescription(0, &Description);
	return Description.IsConnected;
}