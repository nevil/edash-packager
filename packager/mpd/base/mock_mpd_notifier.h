// Copyright 2015 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef MPD_BASE_MOCK_MPD_NOTIFIER_H_
#define MPD_BASE_MOCK_MPD_NOTIFIER_H_

#include "packager/mpd/base/mpd_notifier.h"

#include <gmock/gmock.h>

#include "packager/mpd/base/content_protection_element.h"
#include "packager/mpd/base/media_info.pb.h"

namespace edash_packager {

class MockMpdNotifier : public MpdNotifier {
 public:
  MockMpdNotifier(DashProfile profile);
  virtual ~MockMpdNotifier();

  MOCK_METHOD0(Init, bool());
  MOCK_METHOD2(NotifyNewContainer,
               bool(const MediaInfo& media_info, uint32_t* container_id));
  MOCK_METHOD2(NotifySampleDuration,
               bool(uint32_t container_id, uint32_t sample_duration));
  MOCK_METHOD4(NotifyNewSegment,
               bool(uint32_t container_id,
                    uint64_t start_time,
                    uint64_t duration,
                    uint64_t size));
  MOCK_METHOD4(NotifyEncryptionUpdate,
               bool(uint32_t container_id,
                    const std::string& drm_uuid,
                    const std::vector<uint8_t>& new_key_id,
                    const std::vector<uint8_t>& new_pssh));
  MOCK_METHOD2(
      AddContentProtectionElement,
      bool(uint32_t container_id,
           const ContentProtectionElement& content_protection_element));
  MOCK_METHOD0(Flush, bool());
};

}  // namespace edash_packager

#endif  // MPD_BASE_MOCK_MPD_NOTIFIER_H_
