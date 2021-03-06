// Copyright 2014 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "packager/base/files/file_util.h"
#include "packager/media/file/file.h"

DECLARE_uint64(io_cache_size);
DECLARE_uint64(io_block_size);

namespace {
const int kDataSize = 1024;
}

namespace edash_packager {
namespace media {

class LocalFileTest : public testing::Test {
 protected:
  void SetUp() override {
    data_.resize(kDataSize);
    for (int i = 0; i < kDataSize; ++i)
      data_[i] = i % 256;

    // Test file path for file_util API.
    ASSERT_TRUE(base::CreateTemporaryFile(&test_file_path_));
    local_file_name_no_prefix_ = test_file_path_.value();

    // Local file name with prefix for File API.
    local_file_name_ = kLocalFilePrefix;
    local_file_name_ += local_file_name_no_prefix_;
  }

  void TearDown() override {
    // Remove test file if created.
    base::DeleteFile(base::FilePath(local_file_name_no_prefix_), false);
  }

  std::string data_;

  // Path to the temporary file for this test.
  base::FilePath test_file_path_;
  // Same as |test_file_path_| but in string form.
  std::string local_file_name_no_prefix_;

  // Same as |local_file_name_no_prefix_| but with the file prefix.
  std::string local_file_name_;
};

TEST_F(LocalFileTest, ReadNotExist) {
  // Remove test file if it exists.
  base::DeleteFile(base::FilePath(local_file_name_no_prefix_), false);
  ASSERT_TRUE(File::Open(local_file_name_.c_str(), "r") == NULL);
}

TEST_F(LocalFileTest, Size) {
  ASSERT_EQ(kDataSize,
            base::WriteFile(test_file_path_, data_.data(), kDataSize));
  ASSERT_EQ(kDataSize, File::GetFileSize(local_file_name_.c_str()));
}

TEST_F(LocalFileTest, Copy) {
  ASSERT_EQ(kDataSize,
            base::WriteFile(test_file_path_, data_.data(), kDataSize));

  base::FilePath temp_dir;
  ASSERT_TRUE(base::CreateNewTempDirectory("", &temp_dir));

  // Copy the test file to temp dir as filename "a".
  base::FilePath destination = temp_dir.Append("a");
  ASSERT_TRUE(
      File::Copy(local_file_name_.c_str(), destination.value().c_str()));

  // Make a buffer bigger than the expected file content size to make sure that
  // there isn't extra stuff appended.
  char copied_file_content_buffer[kDataSize * 2] = {};
  ASSERT_EQ(kDataSize, base::ReadFile(destination,
                                      copied_file_content_buffer,
                                      arraysize(copied_file_content_buffer)));

  ASSERT_EQ(data_, std::string(copied_file_content_buffer, kDataSize));

  base::DeleteFile(temp_dir, true);
}

TEST_F(LocalFileTest, Write) {
  // Write file using File API.
  File* file = File::Open(local_file_name_.c_str(), "w");
  ASSERT_TRUE(file != NULL);
  EXPECT_EQ(kDataSize, file->Write(&data_[0], kDataSize));
  EXPECT_EQ(kDataSize, file->Size());
  EXPECT_TRUE(file->Close());

  // Read file using file_util API.
  std::string read_data(kDataSize, 0);
  ASSERT_EQ(kDataSize,
            base::ReadFile(test_file_path_, &read_data[0], kDataSize));

  // Compare data written and read.
  EXPECT_EQ(data_, read_data);
}

TEST_F(LocalFileTest, Read_And_Eof) {
  // Write file using file_util API.
  ASSERT_EQ(kDataSize,
            base::WriteFile(test_file_path_, data_.data(), kDataSize));

  // Read file using File API.
  File* file = File::Open(local_file_name_.c_str(), "r");
  ASSERT_TRUE(file != NULL);

  // Read half of the file.
  const int kFirstReadBytes = kDataSize / 2;
  std::string read_data(kFirstReadBytes + kDataSize, 0);
  EXPECT_EQ(kFirstReadBytes, file->Read(&read_data[0], kFirstReadBytes));

  // Read the remaining half of the file and verify EOF.
  EXPECT_EQ(kDataSize - kFirstReadBytes,
            file->Read(&read_data[kFirstReadBytes], kDataSize));
  uint8_t single_byte;
  EXPECT_EQ(0, file->Read(&single_byte, sizeof(single_byte)));
  EXPECT_TRUE(file->Close());

  // Compare data written and read.
  read_data.resize(kDataSize);
  EXPECT_EQ(data_, read_data);
}

TEST_F(LocalFileTest, WriteRead) {
  // Write file using File API, using file name directly (without prefix).
  File* file = File::Open(local_file_name_no_prefix_.c_str(), "w");
  ASSERT_TRUE(file != NULL);
  EXPECT_EQ(kDataSize, file->Write(&data_[0], kDataSize));
  EXPECT_EQ(kDataSize, file->Size());
  EXPECT_TRUE(file->Close());

  // Read file using File API, using local file prefix + file name.
  file = File::Open(local_file_name_.c_str(), "r");
  ASSERT_TRUE(file != NULL);

  // Read half of the file and verify that Eof is not true.
  std::string read_data(kDataSize, 0);
  EXPECT_EQ(kDataSize, file->Read(&read_data[0], kDataSize));
  EXPECT_TRUE(file->Close());

  // Compare data written and read.
  EXPECT_EQ(data_, read_data);
}

TEST_F(LocalFileTest, WriteFlushCheckSize) {
  const uint32 kNumCycles(10);
  const uint32 kNumWrites(10);

  for (uint32 cycle_idx = 0; cycle_idx < kNumCycles; ++cycle_idx) {
    // Write file using File API, using file name directly (without prefix).
    File* file = File::Open(local_file_name_no_prefix_.c_str(), "w");
    ASSERT_TRUE(file != NULL);
    for (uint32 write_idx = 0; write_idx < kNumWrites; ++write_idx)
      EXPECT_EQ(kDataSize, file->Write(data_.data(), kDataSize));
    ASSERT_NO_FATAL_FAILURE(file->Flush());
    EXPECT_TRUE(file->Close());

    file = File::Open(local_file_name_.c_str(), "r");
    ASSERT_TRUE(file != NULL);
    EXPECT_EQ(static_cast<int64_t>(data_.size() * kNumWrites), file->Size());

    EXPECT_TRUE(file->Close());
  }
}

class ParamLocalFileTest : public LocalFileTest,
                           public ::testing::WithParamInterface<uint8_t> {
};

TEST_P(ParamLocalFileTest, SeekWriteAndSeekRead) {
  const uint32_t kBlockSize(10);
  const uint32_t kInitialWriteSize(100);
  const uint32_t kFinalFileSize(200);

  google::FlagSaver flag_saver;
  FLAGS_io_block_size = kBlockSize;
  FLAGS_io_cache_size = GetParam();

  std::vector<uint8_t> buffer(kInitialWriteSize);
  File* file = File::Open(local_file_name_no_prefix_.c_str(), "w");
  ASSERT_TRUE(file != nullptr);
  ASSERT_EQ(kInitialWriteSize, file->Write(buffer.data(), kInitialWriteSize));
  EXPECT_EQ(kInitialWriteSize, file->Size());
  uint64_t position;
  ASSERT_TRUE(file->Tell(&position));
  ASSERT_EQ(kInitialWriteSize, position);

  for (uint8_t offset = 0; offset < kFinalFileSize; ++offset) {
    EXPECT_TRUE(file->Seek(offset));
    ASSERT_TRUE(file->Tell(&position));
    EXPECT_EQ(offset, position);
    EXPECT_EQ(2u, file->Write(buffer.data(), 2u));
    ASSERT_TRUE(file->Tell(&position));
    EXPECT_EQ(offset + 2u, position);
    ++offset;
    EXPECT_TRUE(file->Seek(offset));
    ASSERT_TRUE(file->Tell(&position));
    EXPECT_EQ(offset, position);
    EXPECT_EQ(1, file->Write(&offset, 1));
    ASSERT_TRUE(file->Tell(&position));
    EXPECT_EQ(offset + 1u, position);
  }
  EXPECT_EQ(kFinalFileSize, file->Size());
  ASSERT_TRUE(file->Close());

  file = File::Open(local_file_name_no_prefix_.c_str(), "r");
  ASSERT_TRUE(file != nullptr);
  for (uint8_t offset = 1; offset < kFinalFileSize; offset += 2) {
    uint8_t read_byte;
    EXPECT_TRUE(file->Seek(offset));
    ASSERT_TRUE(file->Tell(&position));
    EXPECT_EQ(offset, position);
    EXPECT_EQ(1, file->Read(&read_byte, 1));
    ASSERT_TRUE(file->Tell(&position));
    EXPECT_EQ(offset + 1u, position);
    EXPECT_EQ(offset, read_byte);
  }
  EXPECT_EQ(0, file->Read(buffer.data(), 1));
  ASSERT_TRUE(file->Seek(0));
  EXPECT_EQ(1, file->Read(buffer.data(), 1));
  EXPECT_TRUE(file->Close());
}
INSTANTIATE_TEST_CASE_P(TestSeekWithDifferentCacheSizes,
                        ParamLocalFileTest,
                        ::testing::Values(20u, 1000u));

// This test should only be enabled for filesystems which do not allow seeking
// past EOF.
TEST_F(LocalFileTest, DISABLED_WriteSeekOutOfBounds) {
  const uint32_t kFileSize(100);

  std::vector<uint8_t> buffer(kFileSize);
  File* file = File::Open(local_file_name_no_prefix_.c_str(), "w");
  ASSERT_TRUE(file != nullptr);
  ASSERT_EQ(kFileSize, file->Write(buffer.data(), kFileSize));
  ASSERT_EQ(kFileSize, file->Size());
  EXPECT_FALSE(file->Seek(kFileSize + 1));
  EXPECT_TRUE(file->Seek(kFileSize));
  EXPECT_EQ(1, file->Write(buffer.data(), 1));
  EXPECT_TRUE(file->Seek(kFileSize + 1));
  EXPECT_EQ(kFileSize + 1, file->Size());
}

// This test should only be enabled for filesystems which do not allow seeking
// past EOF.
TEST_F(LocalFileTest, DISABLED_ReadSeekOutOfBounds) {
  const uint32_t kFileSize(100);

  File::Delete(local_file_name_no_prefix_.c_str());
  std::vector<uint8_t> buffer(kFileSize);
  File* file = File::Open(local_file_name_no_prefix_.c_str(), "w");
  ASSERT_TRUE(file != nullptr);
  ASSERT_EQ(kFileSize, file->Write(buffer.data(), kFileSize));
  ASSERT_EQ(kFileSize, file->Size());
  ASSERT_TRUE(file->Close());
  file = File::Open(local_file_name_no_prefix_.c_str(), "r");
  ASSERT_TRUE(file != nullptr);
  EXPECT_FALSE(file->Seek(kFileSize + 1));
  EXPECT_TRUE(file->Seek(kFileSize));
  uint64_t position;
  EXPECT_TRUE(file->Tell(&position));
  EXPECT_EQ(kFileSize, position);
  EXPECT_EQ(0u, file->Read(buffer.data(), 1));
  EXPECT_TRUE(file->Seek(0));
  EXPECT_TRUE(file->Tell(&position));
  EXPECT_EQ(0u, position);
  EXPECT_EQ(kFileSize, file->Read(buffer.data(), kFileSize));
  EXPECT_EQ(0u, file->Read(buffer.data(), 1));
  EXPECT_TRUE(file->Close());
}

}  // namespace media
}  // namespace edash_packager
