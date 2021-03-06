// Copyright 2015 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PACKAGER_MEDIA_BASE_TEXT_STREAM_INFO_H_
#define PACKAGER_MEDIA_BASE_TEXT_STREAM_INFO_H_

#include "packager/media/base/stream_info.h"

#include <string>

namespace edash_packager {
namespace media {

class TextStreamInfo : public StreamInfo {
 public:
  /// No encryption supported.
  /// @param track_id is the track ID of this stream.
  /// @param time_scale is the time scale of this stream.
  /// @param duration is the duration of this stream.
  /// @param codec_string is the codec.
  /// @param language is the language of this stream. This may be empty.
  /// @param extra_data is extra data for this text stream. This could be the
  ///        metadata that applies to all the samples of this stream. This may
  ///        be empty.
  /// @param width of the text. This may be 0.
  /// @param height of the text. This may be 0.
  TextStreamInfo(int track_id,
                 uint32_t time_scale,
                 uint64_t duration,
                 const std::string& codec_string,
                 const std::string& language,
                 const std::string& extra_data,
                 uint16_t width,
                 uint16_t height);

  bool IsValidConfig() const override;

  uint16_t width() const { return width_; }
  uint16_t height() const { return height_; }

 protected:
  ~TextStreamInfo() override;

 private:
  uint16_t width_;
  uint16_t height_;

  // Allow copying. This is very light weight.
};

}  // namespace media
}  // namespace edash_packager

#endif  // PACKAGER_MEDIA_BASE_TEXT_STREAM_INFO_H_
