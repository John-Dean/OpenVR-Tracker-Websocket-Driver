#include "HMDDevice.hpp"
#include <Windows.h>

websocket_trackersDriver::HMDDevice::HMDDevice(std::string serial):serial_(serial)
{
}

std::string websocket_trackersDriver::HMDDevice::GetSerial()
{
    return this->serial_;
}

void websocket_trackersDriver::HMDDevice::Update()
{
    if (this->device_index_ == vr::k_unTrackedDeviceIndexInvalid)
        return;
    
    // Setup pose for this frame
    auto pose = IVRDevice::MakeDefaultPose();

    float delta_seconds = GetDriver()->GetLastFrameTime().count() / 1000.0f;

    // Get orientation
    this->rot_y_ += (1.0f * (GetAsyncKeyState(VK_RIGHT) == 0) - 1.0f * (GetAsyncKeyState(VK_LEFT) == 0)) * delta_seconds;
    this->rot_x_ += (-1.0f * (GetAsyncKeyState(VK_UP) == 0) + 1.0f * (GetAsyncKeyState(VK_DOWN) == 0)) * delta_seconds;
    this->rot_x_ = std::fmax(this->rot_x_, -3.14159f/2);
    this->rot_x_ = std::fmin(this->rot_x_, 3.14159f/2);

    linalg::vec<float, 4> y_quat{ 0, std::sinf(this->rot_y_ / 2), 0, std::cosf(this->rot_y_ / 2) };

    linalg::vec<float, 4> x_quat{ std::sinf(this->rot_x_ / 2), 0, 0, std::cosf(this->rot_x_ / 2) };

    linalg::vec<float, 4> pose_rot = linalg::qmul(y_quat, x_quat);

    pose.qRotation.w = (float) pose_rot.w;
    pose.qRotation.x = (float) pose_rot.x;
    pose.qRotation.y = (float) pose_rot.y;
    pose.qRotation.z = (float) pose_rot.z;

    // Update position based on rotation
    linalg::vec<float, 3> forward_vec{-1.0f * (GetAsyncKeyState(0x44) == 0) + 1.0f * (GetAsyncKeyState(0x41) == 0), 0, 0};
    linalg::vec<float, 3> right_vec{0, 0, 1.0f * (GetAsyncKeyState(0x57) == 0) - 1.0f * (GetAsyncKeyState(0x53) == 0) };
    linalg::vec<float, 3> final_dir = forward_vec + right_vec;
    if (linalg::length(final_dir) > 0.01) {
        final_dir = linalg::normalize(final_dir) * (float)delta_seconds;
        final_dir = linalg::qrot(pose_rot, final_dir);
        this->pos_x_ += final_dir.x;
        this->pos_y_ += final_dir.y;
        this->pos_z_ += final_dir.z;
    }

    pose.vecPosition[0] = (float) this->pos_x_;
    pose.vecPosition[1] = (float) this->pos_y_;
    pose.vecPosition[2] = (float) this->pos_z_;

    // Post pose
    GetDriver()->GetDriverHost()->TrackedDevicePoseUpdated(this->device_index_, pose, sizeof(vr::DriverPose_t));
    this->last_pose_ = pose;
}

DeviceType websocket_trackersDriver::HMDDevice::GetDeviceType()
{
    return DeviceType::HMD;
}

vr::TrackedDeviceIndex_t websocket_trackersDriver::HMDDevice::GetDeviceIndex()
{
    return this->device_index_;
}

vr::EVRInitError websocket_trackersDriver::HMDDevice::Activate(uint32_t unObjectId)
{
    this->device_index_ = unObjectId;

    GetDriver()->Log("Activating HMD " + this->serial_);

    // Load settings values
    // Could probably make this cleaner with making a wrapper class
    try {
        int window_x = std::get<int>(GetDriver()->GetSettingsValue("window_x"));
        if (window_x > 0)
            this->window_x_ = window_x;
    }
    catch (const std::bad_variant_access&) {}; // Wrong type or doesnt exist

    try {
        int window_y = std::get<int>(GetDriver()->GetSettingsValue("window_y"));
        if (window_y > 0)
            this->window_x_ = window_y;
    }
    catch (const std::bad_variant_access&) {}; // Wrong type or doesnt exist

    try {
        int window_width = std::get<int>(GetDriver()->GetSettingsValue("window_width"));
        if (window_width > 0)
            this->window_width_ = window_width;
    }
    catch (const std::bad_variant_access&) {}; // Wrong type or doesnt exist

    try {
        int window_height = std::get<int>(GetDriver()->GetSettingsValue("window_height"));
        if (window_height > 0)
            this->window_height_ = window_height;
    }
    catch (const std::bad_variant_access&) {}; // Wrong type or doesnt exist

    // Get the properties handle
    auto props = GetDriver()->GetProperties()->TrackedDeviceToPropertyContainer(this->device_index_);

    // Set some universe ID (Must be 2 or higher)
    GetDriver()->GetProperties()->SetUint64Property(props, vr::Prop_CurrentUniverseId_Uint64, 2);

    // Set the IPD to be whatever steam has configured
    GetDriver()->GetProperties()->SetFloatProperty(props, vr::Prop_UserIpdMeters_Float, vr::VRSettings()->GetFloat(vr::k_pch_SteamVR_Section, vr::k_pch_SteamVR_IPD_Float));

    // Set the display FPS
    GetDriver()->GetProperties()->SetFloatProperty(props, vr::Prop_DisplayFrequency_Float, 90.f);
    
    // Set up a model "number" (not needed but good to have)
    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_ModelNumber_String, "websocket_trackers_HMD_DEVICE");

    // Set up icon paths
    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceReady_String, "{websocket_trackers}/icons/hmd_ready.png");

    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceOff_String, "{websocket_trackers}/icons/hmd_not_ready.png");
    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceSearching_String, "{websocket_trackers}/icons/hmd_not_ready.png");
    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{websocket_trackers}/icons/hmd_not_ready.png");
    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{websocket_trackers}/icons/hmd_not_ready.png");
    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceNotReady_String, "{websocket_trackers}/icons/hmd_not_ready.png");
    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceStandby_String, "{websocket_trackers}/icons/hmd_not_ready.png");
    GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceAlertLow_String, "{websocket_trackers}/icons/hmd_not_ready.png");

    


    return vr::EVRInitError::VRInitError_None;
}

void websocket_trackersDriver::HMDDevice::Deactivate()
{
    this->device_index_ = vr::k_unTrackedDeviceIndexInvalid;
}

void websocket_trackersDriver::HMDDevice::EnterStandby()
{
}

void* websocket_trackersDriver::HMDDevice::GetComponent(const char* pchComponentNameAndVersion)
{
    if (!_stricmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version)) {
        return static_cast<vr::IVRDisplayComponent*>(this);
    }
    return nullptr;
}

void websocket_trackersDriver::HMDDevice::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
    if (unResponseBufferSize >= 1)
        pchResponseBuffer[0] = 0;
}

vr::DriverPose_t websocket_trackersDriver::HMDDevice::GetPose()
{
    return this->last_pose_;
}

void websocket_trackersDriver::HMDDevice::GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight)
{
    *pnX = this->window_x_;
    *pnY = this->window_y_;
    *pnWidth = this->window_width_;
    *pnHeight = this->window_height_;
}

bool websocket_trackersDriver::HMDDevice::IsDisplayOnDesktop()
{
    return true;
}

bool websocket_trackersDriver::HMDDevice::IsDisplayRealDisplay()
{
    return false;
}

void websocket_trackersDriver::HMDDevice::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight)
{
    *pnWidth = this->window_width_;
    *pnHeight = this->window_height_;
}

void websocket_trackersDriver::HMDDevice::GetEyeOutputViewport(vr::EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight)
{
    *pnY = 0;
    *pnWidth = this->window_width_ / 2;
    *pnHeight = this->window_height_;

    if (eEye == vr::EVREye::Eye_Left) {
        *pnX = 0;
    }
    else {
        *pnX = this->window_width_ / 2;
    }
}

void websocket_trackersDriver::HMDDevice::GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom)
{
    *pfLeft = -1;
    *pfRight = 1;
    *pfTop = -1;
    *pfBottom = 1;
}

vr::DistortionCoordinates_t websocket_trackersDriver::HMDDevice::ComputeDistortion(vr::EVREye eEye, float fU, float fV)
{
    vr::DistortionCoordinates_t coordinates;
    coordinates.rfBlue[0] = fU;
    coordinates.rfBlue[1] = fV;
    coordinates.rfGreen[0] = fU;
    coordinates.rfGreen[1] = fV;
    coordinates.rfRed[0] = fU;
    coordinates.rfRed[1] = fV;
    return coordinates;
}
