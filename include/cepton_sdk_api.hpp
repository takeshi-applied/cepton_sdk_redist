/*
  Copyright Cepton Technologies Inc. 2017, All rights reserved.

  Cepton Sensor SDK high level API for prototyping.
*/
#pragma once

#include <algorithm>
#include <exception>
#include <thread>

#include "cepton_sdk.hpp"
#include "cepton_sdk_util.hpp"

namespace cepton_sdk {
namespace api {

/// Returns false if capture replay is open, true otherwise.
static bool is_live() { return !capture_replay::is_open(); }

static bool is_end() { return (is_live()) ? false : capture_replay::is_end(); }

/// Returns capture replay time or live time.
static uint64_t get_time() {
  return (is_live()) ? util::get_timestamp_usec() : capture_replay::get_time();
}

/// Sleeps or resumes capture replay for duration.
static SensorErrorCode wait(float t_length = 0.1f) {
  if (is_live() || capture_replay::is_running()) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds((int)(1e3f * t_length)));
    return CEPTON_SUCCESS;
  } else {
    return capture_replay::resume_blocking(t_length);
  }
}

// -----------------------------------------------------------------------------
// Errors
// -----------------------------------------------------------------------------
/// SensorErrorCode exception.
struct SensorError : public std::runtime_error {
  SensorErrorCode error_code;

  SensorError(const char *const msg, SensorErrorCode error_code_)
      : std::runtime_error(msg), error_code(error_code_) {}
};

/// Handles error code.
/**
 * - If error, raises `cepton_sdk::api::SensorError` exception.
 * - If fault, prints message.
 * - Otherwise, does nothing.
 *
 * This is for sample code; production code should handle errors properly.
 */
static SensorErrorCode check_error_code(SensorErrorCode error_code,
                                        const std::string &msg = "") {
  if (!error_code) return error_code;
  std::string error_code_name = get_error_code_name(error_code);
  char error_msg[100];
  if (msg.empty()) {
    std::snprintf(error_msg, 100, "SDK Error: %s!\n", error_code_name.c_str());
  } else {
    std::snprintf(error_msg, 100, "%s: %s!\n", msg.c_str(),
                  error_code_name.c_str());
  }
  if (cepton_sdk::is_error_code(error_code)) {
    throw SensorError(error_msg, error_code);
  } else {
    std::fprintf(stderr, "%s", error_msg);
  }
  return error_code;
}

/// Basic SDK error callback.
/**
 * Calls `cepton_sdk::api::check_error_code`.
 */
static void default_on_error(SensorHandle h, SensorErrorCode error_code,
                             const char *const error_msg,
                             const void *const error_data,
                             std::size_t error_data_size,
                             void *const instance) {
  check_error_code(error_code, error_msg);
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
/// Initialize SDK and optionally starts capture replay.
inline static SensorErrorCode initialize(Options options = create_options(),
                                         const std::string &capture_path = "") {
  // Initialize
  if (!capture_path.empty())
    options.control_flags |= CEPTON_SDK_CONTROL_DISABLE_NETWORK;
  auto error_code =
      ::cepton_sdk::initialize(CEPTON_SDK_VERSION, options, default_on_error);
  if (error_code) return error_code;

  // Open capture replay
  if (!capture_path.empty()) {
    error_code = capture_replay::open(capture_path);
    if (error_code) return error_code;
  }

  return wait(1.0f);
}

/// Callback for image frames.
class SensorImageFramesCallback

    : public util::Callback<SensorHandle, std::size_t,
                            const SensorImagePoint *> {
 public:
  ~SensorImageFramesCallback() { deinitialize(); }
  SensorErrorCode initialize() {
    return listen_image_frames(global_on_callback, this);
  }
  SensorErrorCode deinitialize() { return unlisten_image_frames(); }
};

/// Callback for network packets.
class NetworkPacketsCallback
    : public util::Callback<SensorHandle, uint8_t const *, std::size_t> {
 public:
  ~NetworkPacketsCallback() { deinitialize(); }
  SensorErrorCode initialize() {
    return listen_network_packets(global_on_callback, this);
  }
  SensorErrorCode deinitialize() { return unlisten_network_packets(); }
};

// -----------------------------------------------------------------------------
// Sensors
// -----------------------------------------------------------------------------
static SensorErrorCode get_sensor_information_by_serial_number(
    uint64_t serial_number, SensorInformation &info) {
  CeptonSensorHandle handle;
  SensorErrorCode error_code =
      get_sensor_handle_by_serial_number(serial_number, handle);
  if (error_code) return error_code;
  return get_sensor_information(handle, info);
}

static std::vector<uint64_t> get_sensor_serial_numbers() {
  const std::size_t n_sensors = get_n_sensors();
  std::vector<uint64_t> serial_numbers;
  serial_numbers.reserve(n_sensors);
  for (std::size_t i = 0; i < n_sensors; ++i) {
    SensorInformation sensor_info;
    SensorErrorCode error_code =
        get_sensor_information_by_index(i, sensor_info);
    check_error_code(error_code);
    serial_numbers.push_back(sensor_info.serial_number);
  }
  std::sort(serial_numbers.begin(), serial_numbers.end());
  return serial_numbers;
}
}  // namespace api
}  // namespace cepton_sdk
