// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_DECRYPT_CONFIG_H_
#define MEDIA_BASE_DECRYPT_CONFIG_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "packager/base/memory/scoped_ptr.h"
#include "packager/media/base/fourccs.h"

namespace edash_packager {
namespace media {

/// The Common Encryption spec provides for subsample encryption, where portions
/// of a sample are not encrypted. A SubsampleEntry specifies the number of
/// clear and encrypted bytes in each subsample. For decryption, all of the
/// encrypted bytes in a sample should be considered a single logical stream,
/// regardless of how they are divided into subsamples, and the clear bytes
/// should not be considered as part of decryption. This is logically equivalent
/// to concatenating all @a cipher_bytes portions of subsamples, decrypting that
/// result, and then copying each byte from the decrypted block over the
/// corresponding encrypted byte.
struct SubsampleEntry {
  SubsampleEntry()
    : clear_bytes(0), cipher_bytes(0) {}
  SubsampleEntry(uint16_t clear_bytes, uint32_t cipher_bytes)
    : clear_bytes(clear_bytes), cipher_bytes(cipher_bytes) {}

  uint16_t clear_bytes;
  uint32_t cipher_bytes;
};

/// Contains all the information that a decryptor needs to decrypt a media
/// sample.
class DecryptConfig {
 public:
  /// Keys are always 128 bits.
  static const size_t kDecryptionKeySize = 16;

  /// Create a 'cenc' decrypt config.
  /// @param key_id is the ID that references the decryption key.
  /// @param iv is the initialization vector defined by the encryptor.
  /// @param subsamples defines the clear and encrypted portions of the sample
  ///        as described in SubsampleEntry. A decrypted buffer will be equal
  ///        in size to the sum of the subsample sizes.
  DecryptConfig(const std::vector<uint8_t>& key_id,
                const std::vector<uint8_t>& iv,
                const std::vector<SubsampleEntry>& subsamples);

  /// Create a general decrypt config with possible pattern-based encryption.
  /// @param key_id is the ID that references the decryption key.
  /// @param iv is the initialization vector defined by the encryptor.
  /// @param subsamples defines the clear and encrypted portions of the sample
  ///        as described in SubsampleEntry. A decrypted buffer will be equal
  ///        in size to the sum of the subsample sizes.
  /// @param protection_scheme specifies the protection scheme: 'cenc', 'cens',
  ///        'cbc1', 'cbcs'.
  /// @param crypt_byte_block indicates number of encrypted blocks (16-byte) in
  ///        pattern based encryption, 'cens' and 'cbcs'. Ignored otherwise.
  /// @param skip_byte_block indicates number of unencrypted blocks (16-byte)
  ///        in pattern based encryption, 'cens' and 'cbcs'. Ignored otherwise.
  DecryptConfig(const std::vector<uint8_t>& key_id,
                const std::vector<uint8_t>& iv,
                const std::vector<SubsampleEntry>& subsamples,
                FourCC protection_scheme,
                uint8_t crypt_byte_block,
                uint8_t skip_byte_block);

  ~DecryptConfig();

  const std::vector<uint8_t>& key_id() const { return key_id_; }
  const std::vector<uint8_t>& iv() const { return iv_; }
  const std::vector<SubsampleEntry>& subsamples() const { return subsamples_; }
  FourCC protection_scheme() const { return protection_scheme_; }
  uint8_t crypt_byte_block() const { return crypt_byte_block_; }
  uint8_t skip_byte_block() const { return skip_byte_block_; }

 private:
  const std::vector<uint8_t> key_id_;

  // Initialization vector.
  const std::vector<uint8_t> iv_;

  // Subsample information. May be empty for some formats, meaning entire frame
  // (less data ignored by data_offset_) is encrypted.
  const std::vector<SubsampleEntry> subsamples_;

  const FourCC protection_scheme_;
  // For pattern-based protection schemes, like CENS and CBCS.
  const uint8_t crypt_byte_block_;
  const uint8_t skip_byte_block_;

  DISALLOW_COPY_AND_ASSIGN(DecryptConfig);
};

}  // namespace media
}  // namespace edash_packager

#endif  // MEDIA_BASE_DECRYPT_CONFIG_H_
