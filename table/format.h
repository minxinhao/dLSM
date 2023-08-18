// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_dLSM_TABLE_FORMAT_H_
#define STORAGE_dLSM_TABLE_FORMAT_H_

#include <cstdint>
#include <string>

#include "dLSM/slice.h"
#include "dLSM/status.h"
#include <map>
#include "util/rdma.h"
//#include "dLSM/table_builder.h"

namespace dLSM {

class Block;
class RandomAccessFile;
struct ReadOptions;
//struct ibv_mr;
// BlockHandle is a pointer to the extent of a file that stores a data
// block or a meta block.
class BlockHandle {
 public:
  // Maximum encoding length of a BlockHandle
  enum { kMaxEncodedLength = 10 + 10 };

  BlockHandle();

  // The offset of the block in the file.
  uint64_t offset() const { return offset_; }
  void set_offset(uint64_t offset) { offset_ = offset; }

  // The size of the stored block
  uint64_t size() const { return size_; }
  void set_size(uint64_t size) { size_ = size; }

  void EncodeTo(std::string* dst) const;
  Status DecodeFrom(Slice* input);

 private:
  uint64_t offset_;// count in the blocktrailer.
  uint64_t size_;
};

// Footer encapsulates the fixed information stored at the tail
// end of every table file.
class Footer {
 public:
  // Encoded length of a Footer.  Note that the serialization of a
  // Footer will always occupy exactly this many bytes.  It consists
  // of two block handles and a magic number.
  enum { kEncodedLength = 2 * BlockHandle::kMaxEncodedLength + 8 };

  Footer() = default;

  // The block handle for the metaindex block of the table
  const BlockHandle& metaindex_handle() const { return metaindex_handle_; }
  void set_metaindex_handle(const BlockHandle& h) { metaindex_handle_ = h; }

  // The block handle for the index block of the table
  const BlockHandle& index_handle() const { return index_handle_; }
  void set_index_handle(const BlockHandle& h) { index_handle_ = h; }

  void EncodeTo(std::string* dst) const;
  Status DecodeFrom(Slice* input);

 private:
  BlockHandle metaindex_handle_;
  BlockHandle index_handle_;
};

// kTableMagicNumber was picked by running
//    echo http://code.google.com/p/dLSM/ | sha1sum
// and taking the leading 64 bits.
static const uint64_t kTableMagicNumber = 0xdb4775248b80fb57ull;

// 1-byte type + 32-bit crc
static const size_t kBlockTrailerSize = 5;

struct BlockContents {
  Slice data;           // Actual contents of data
//  bool cachable;        // True iff data can be cached
//  bool heap_allocated;  // True iff caller should delete[] data.data()
};
void Find_Local_MR(std::map<uint32_t, ibv_mr*>* remote_data_blocks,
                    const BlockHandle& handle, Slice& data);
void Find_Remote_MR(std::map<uint32_t, ibv_mr*>* remote_data_blocks,
                         const BlockHandle& handle, ibv_mr* remote_mr);
bool Find_prefetch_MR(std::map<uint32_t, ibv_mr*>* remote_data_blocks,
                       const size_t& offset, ibv_mr* remote_mr);

// Read the block identified by "handle" from "file".  On failure
// return non-OK.  On success fill *result and return OK.
Status ReadDataBlock(std::map<uint32_t, ibv_mr*>* remote_data_blocks, const ReadOptions& options,
                 const BlockHandle& handle, BlockContents* result);
Status ReadKVPair(std::map<uint32_t, ibv_mr*>* remote_data_blocks,
                  const ReadOptions& options, const BlockHandle& handle,
                  Slice* result, uint8_t target_node_id);
Status ReadDataIndexBlock(ibv_mr* remote_mr, const ReadOptions& options,
                          BlockContents* result, uint8_t target_node_id);
Status ReadFilterBlock(ibv_mr* remote_mr, const ReadOptions& options,
                       BlockContents* result, uint8_t target_node_id);
// Implementation details follow.  Clients should ignore,

inline BlockHandle::BlockHandle()
    : offset_(~static_cast<uint64_t>(0)), size_(~static_cast<uint64_t>(0)) {}

}  // namespace dLSM

#endif  // STORAGE_dLSM_TABLE_FORMAT_H_