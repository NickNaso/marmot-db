// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "../core/gc_state.h"
#include "../core/light_epoch.h"
#include "../environment/file.h"

namespace FASTER {
namespace device {

/// A dummy (null) disk, used when you want an in-memory-only FASTER store.

struct NullHandler {
};

class NullFile {
 public:
  Status Open(NullHandler* handler) {
    return Status::Ok;
  }
  Status Close() {
    return Status::Ok;
  }
  Status Delete() {
    return Status::Ok;
  }
  void Truncate(uint64_t new_begin_offset, GcState::truncate_callback_t callback) {
    if(callback) {
      callback(new_begin_offset);
    }
  }

  Status ReadAsync(uint64_t source, void* dest, uint32_t length,
                   AsyncIOCallback callback, IAsyncContext& context) const {
    callback(&context, Status::Ok, length);
    return Status::Ok;
  }
  Status WriteAsync(const void* source, uint64_t dest, uint32_t length,
                    AsyncIOCallback callback, IAsyncContext& context) {
    callback(&context, Status::Ok, length);
    return Status::Ok;
  }

  static size_t alignment() {
    // Align null device to cache line.
    return 64;
  }

  void set_handler(NullHandler* handler) {
  }
};

class NullDisk {
 public:
  typedef NullHandler handler_t;
  typedef NullFile file_t;
  typedef NullFile log_file_t;

  NullDisk(const std::string& filename, LightEpoch& epoch) {
  }

  static uint32_t sector_size() {
    return 64;
  }

  /// Methods required by the (implicit) disk interface.
  const file_t& log() const {
    return log_;
  }
  file_t& log() {
    return log_;
  }

  std::string relative_index_checkpoint_path(uint32_t version) const {
    assert(false);
    return "";
  }
  std::string index_checkpoint_path(uint32_t version) const {
    assert(false);
    return "";
  }

  std::string relative_cpr_checkpoint_path(uint32_t version) const {
    assert(false);
    return "";
  }
  std::string cpr_checkpoint_path(uint32_t version) const {
    assert(false);
    return "";
  }

  void CreateIndexCheckpointDirectory(uint32_t version) {
    assert(false);
  }
  void CreateCprCheckpointDirectory(uint32_t version) {
    assert(false);
  }

  file_t NewFile(const std::string& relative_path) {
    assert(false);
    return file_t{};
  }

  handler_t& handler() {
    return handler_;
  }

  inline static constexpr bool TryComplete() {
    return false;
  }

 private:
  handler_t handler_;
  file_t log_;
};

}
} // namespace FASTER::device