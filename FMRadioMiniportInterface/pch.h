#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <initguid.h>
#include <guiddef.h>
#include <cguid.h>
#include <atlcomcli.h>
#include <atlcoll.h>
#include <atlbase.h>
#include <atlstr.h>
#include <mmdeviceapi.h>
#include <devicetopology.h>
#include "Audio API/Header Files/AudioTunerDrv.h"

#include <sddl.h>
#include <securitybaseapi.h>
#include <AclAPI.h>

#include <iostream>
#include <string>
#include <atomic>
#include <array>
#include <functional>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <bitset>
#include <future>
#include <optional>
#include <cstdlib>