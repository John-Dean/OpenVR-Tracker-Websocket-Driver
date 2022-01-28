#include "DriverFactory.hpp"
#include <thread>
#include <Driver/VRDriver.hpp>
#include <Windows.h>
#include <sstream>

static std::shared_ptr<websocket_trackersDriver::IVRDriver> driver;

void* HmdDriverFactory(const char* interface_name, int* return_code) {
	if (std::strcmp(interface_name, vr::IServerTrackedDeviceProvider_Version) == 0) {
		if (!driver) {
			// Instantiate concrete impl
			driver = std::make_shared<websocket_trackersDriver::VRDriver>();
		}
		// We always have at least 1 ref to the shared ptr in "driver" so passing out raw pointer is ok
		return driver.get();
	}

	if (return_code)
		*return_code = vr::VRInitError_Init_InterfaceNotFound;

	return nullptr;
}

std::shared_ptr<websocket_trackersDriver::IVRDriver> websocket_trackersDriver::GetDriver() {
	return driver;
}
