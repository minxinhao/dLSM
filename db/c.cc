// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "dLSM/c.h"

#include <string.h>

#include <cstdint>
#include <cstdlib>

#include "dLSM/cache.h"
#include "dLSM/comparator.h"
#include "dLSM/db.h"
#include "dLSM/env.h"
#include "dLSM/filter_policy.h"
#include "dLSM/iterator.h"
#include "dLSM/options.h"
#include "dLSM/status.h"
#include "dLSM/write_batch.h"

using dLSM::Cache;
using dLSM::Comparator;
using dLSM::CompressionType;
using dLSM::DB;
using dLSM::Env;
using dLSM::FileLock;
using dLSM::FilterPolicy;
using dLSM::Iterator;
using dLSM::kMajorVersion;
using dLSM::kMinorVersion;
using dLSM::Logger;
using dLSM::NewBloomFilterPolicy;
using dLSM::NewLRUCache;
using dLSM::Options;
using dLSM::RandomAccessFile;
using dLSM::Range;
using dLSM::ReadOptions;
using dLSM::SequentialFile;
using dLSM::Slice;
using dLSM::Snapshot;
using dLSM::Status;
using dLSM::WritableFile;
using dLSM::WriteBatch;
using dLSM::WriteOptions;

extern "C" {

struct dLSM_t {
  DB* rep;
};
struct dLSM_iterator_t {
  Iterator* rep;
};
struct dLSM_writebatch_t {
  WriteBatch rep;
};
struct dLSM_snapshot_t {
  const Snapshot* rep;
};
struct dLSM_readoptions_t {
  ReadOptions rep;
};
struct dLSM_writeoptions_t {
  WriteOptions rep;
};
struct dLSM_options_t {
  Options rep;
};
struct dLSM_cache_t {
  Cache* rep;
};
struct dLSM_seqfile_t {
  SequentialFile* rep;
};
struct dLSM_randomfile_t {
  RandomAccessFile* rep;
};
struct dLSM_writablefile_t {
  WritableFile* rep;
};
struct dLSM_logger_t {
  Logger* rep;
};
struct dLSM_filelock_t {
  FileLock* rep;
};

struct dLSM_comparator_t : public Comparator {
  ~dLSM_comparator_t() override { (*destructor_)(state_); }

  int Compare(const Slice& a, const Slice& b) const override {
    return (*compare_)(state_, a.data(), a.size(), b.data(), b.size());
  }

  const char* Name() const override { return (*name_)(state_); }

  // No-ops since the C binding does not support key shortening methods.
  void FindShortestSeparator(std::string*, const Slice&) const override {}
  void FindShortSuccessor(std::string* key) const override {}

  void* state_;
  void (*destructor_)(void*);
  int (*compare_)(void*, const char* a, size_t alen, const char* b,
                  size_t blen);
  const char* (*name_)(void*);
};

struct dLSM_filterpolicy_t : public FilterPolicy {
  ~dLSM_filterpolicy_t() override { (*destructor_)(state_); }

  const char* Name() const override { return (*name_)(state_); }

  void CreateFilter(const Slice* keys, int n, Slice* dst) const override {
    std::vector<const char*> key_pointers(n);
    std::vector<size_t> key_sizes(n);
    for (int i = 0; i < n; i++) {
      key_pointers[i] = keys[i].data();
      key_sizes[i] = keys[i].size();
    }
    size_t len;
    char* filter = (*create_)(state_, &key_pointers[0], &key_sizes[0], n, &len);
    dst->append(filter, len);
    std::free(filter);
  }

  bool KeyMayMatch(const Slice& key, const Slice& filter) const override {
    return (*key_match_)(state_, key.data(), key.size(), filter.data(),
                         filter.size());
  }

  void* state_;
  void (*destructor_)(void*);
  const char* (*name_)(void*);
  char* (*create_)(void*, const char* const* key_array,
                   const size_t* key_length_array, int num_keys,
                   size_t* filter_length);
  uint8_t (*key_match_)(void*, const char* key, size_t length,
                        const char* filter, size_t filter_length);
};

struct dLSM_env_t {
  Env* rep;
  bool is_default;
};

static bool SaveError(char** errptr, const Status& s) {
  assert(errptr != nullptr);
  if (s.ok()) {
    return false;
  } else if (*errptr == nullptr) {
    *errptr = strdup(s.ToString().c_str());
  } else {
    // TODO(sanjay): Merge with existing error?
    std::free(*errptr);
    *errptr = strdup(s.ToString().c_str());
  }
  return true;
}

static char* CopyString(const std::string& str) {
  char* result =
      reinterpret_cast<char*>(std::malloc(sizeof(char) * str.size()));
  std::memcpy(result, str.data(), sizeof(char) * str.size());
  return result;
}

dLSM_t* dLSM_open(const dLSM_options_t* options, const char* name,
                        char** errptr) {
  DB* db;
  if (SaveError(errptr, DB::Open(options->rep, std::string(name), &db))) {
    return nullptr;
  }
  dLSM_t* result = new dLSM_t;
  result->rep = db;
  return result;
}

void dLSM_close(dLSM_t* db) {
  delete db->rep;
  delete db;
}

void dLSM_put(dLSM_t* db, const dLSM_writeoptions_t* options,
                 const char* key, size_t keylen, const char* val, size_t vallen,
                 char** errptr) {
  SaveError(errptr,
            db->rep->Put(options->rep, Slice(key, keylen), Slice(val, vallen)));
}

void dLSM_delete(dLSM_t* db, const dLSM_writeoptions_t* options,
                    const char* key, size_t keylen, char** errptr) {
  SaveError(errptr, db->rep->Delete(options->rep, Slice(key, keylen)));
}

void dLSM_write(dLSM_t* db, const dLSM_writeoptions_t* options,
                   dLSM_writebatch_t* batch, char** errptr) {
  SaveError(errptr, db->rep->Write(options->rep, &batch->rep));
}

char* dLSM_get(dLSM_t* db, const dLSM_readoptions_t* options,
                  const char* key, size_t keylen, size_t* vallen,
                  char** errptr) {
  char* result = nullptr;
  std::string tmp;
  Status s = db->rep->Get(options->rep, Slice(key, keylen), &tmp);
  if (s.ok()) {
    *vallen = tmp.size();
    result = CopyString(tmp);
  } else {
    *vallen = 0;
    if (!s.IsNotFound()) {
      SaveError(errptr, s);
    }
  }
  return result;
}

dLSM_iterator_t* dLSM_create_iterator(
    dLSM_t* db, const dLSM_readoptions_t* options) {
  dLSM_iterator_t* result = new dLSM_iterator_t;
  result->rep = db->rep->NewIterator(options->rep);
  return result;
}

const dLSM_snapshot_t* dLSM_create_snapshot(dLSM_t* db) {
  dLSM_snapshot_t* result = new dLSM_snapshot_t;
  result->rep = db->rep->GetSnapshot();
  return result;
}

void dLSM_release_snapshot(dLSM_t* db,
                              const dLSM_snapshot_t* snapshot) {
  db->rep->ReleaseSnapshot(snapshot->rep);
  delete snapshot;
}

char* dLSM_property_value(dLSM_t* db, const char* propname) {
  std::string tmp;
  if (db->rep->GetProperty(Slice(propname), &tmp)) {
    // We use strdup() since we expect human readable output.
    return strdup(tmp.c_str());
  } else {
    return nullptr;
  }
}

void dLSM_approximate_sizes(dLSM_t* db, int num_ranges,
                               const char* const* range_start_key,
                               const size_t* range_start_key_len,
                               const char* const* range_limit_key,
                               const size_t* range_limit_key_len,
                               uint64_t* sizes) {
  Range* ranges = new Range[num_ranges];
  for (int i = 0; i < num_ranges; i++) {
    ranges[i].start = Slice(range_start_key[i], range_start_key_len[i]);
    ranges[i].limit = Slice(range_limit_key[i], range_limit_key_len[i]);
  }
  db->rep->GetApproximateSizes(ranges, num_ranges, sizes);
  delete[] ranges;
}

void dLSM_compact_range(dLSM_t* db, const char* start_key,
                           size_t start_key_len, const char* limit_key,
                           size_t limit_key_len) {
  Slice a, b;
  db->rep->CompactRange(
      // Pass null Slice if corresponding "const char*" is null
      (start_key ? (a = Slice(start_key, start_key_len), &a) : nullptr),
      (limit_key ? (b = Slice(limit_key, limit_key_len), &b) : nullptr));
}

void dLSM_destroy_db(const dLSM_options_t* options, const char* name,
                        char** errptr) {
  SaveError(errptr, DestroyDB(name, options->rep));
}

void dLSM_repair_db(const dLSM_options_t* options, const char* name,
                       char** errptr) {
  SaveError(errptr, RepairDB(name, options->rep));
}

void dLSM_iter_destroy(dLSM_iterator_t* iter) {
  delete iter->rep;
  delete iter;
}

uint8_t dLSM_iter_valid(const dLSM_iterator_t* iter) {
  return iter->rep->Valid();
}

void dLSM_iter_seek_to_first(dLSM_iterator_t* iter) {
  iter->rep->SeekToFirst();
}

void dLSM_iter_seek_to_last(dLSM_iterator_t* iter) {
  iter->rep->SeekToLast();
}

void dLSM_iter_seek(dLSM_iterator_t* iter, const char* k, size_t klen) {
  iter->rep->Seek(Slice(k, klen));
}

void dLSM_iter_next(dLSM_iterator_t* iter) { iter->rep->Next(); }

void dLSM_iter_prev(dLSM_iterator_t* iter) { iter->rep->Prev(); }

const char* dLSM_iter_key(const dLSM_iterator_t* iter, size_t* klen) {
  Slice s = iter->rep->key();
  *klen = s.size();
  return s.data();
}

const char* dLSM_iter_value(const dLSM_iterator_t* iter, size_t* vlen) {
  Slice s = iter->rep->value();
  *vlen = s.size();
  return s.data();
}

void dLSM_iter_get_error(const dLSM_iterator_t* iter, char** errptr) {
  SaveError(errptr, iter->rep->status());
}

dLSM_writebatch_t* dLSM_writebatch_create() {
  return new dLSM_writebatch_t;
}

void dLSM_writebatch_destroy(dLSM_writebatch_t* b) { delete b; }

void dLSM_writebatch_clear(dLSM_writebatch_t* b) { b->rep.Clear(); }

void dLSM_writebatch_put(dLSM_writebatch_t* b, const char* key,
                            size_t klen, const char* val, size_t vlen) {
  b->rep.Put(Slice(key, klen), Slice(val, vlen));
}

void dLSM_writebatch_delete(dLSM_writebatch_t* b, const char* key,
                               size_t klen) {
  b->rep.Delete(Slice(key, klen));
}

void dLSM_writebatch_iterate(const dLSM_writebatch_t* b, void* state,
                                void (*put)(void*, const char* k, size_t klen,
                                            const char* v, size_t vlen),
                                void (*deleted)(void*, const char* k,
                                                size_t klen)) {
  class H : public WriteBatch::Handler {
   public:
    void* state_;
    void (*put_)(void*, const char* k, size_t klen, const char* v, size_t vlen);
    void (*deleted_)(void*, const char* k, size_t klen);
    void Put(const Slice& key, const Slice& value) override {
      (*put_)(state_, key.data(), key.size(), value.data(), value.size());
    }
    void Delete(const Slice& key) override {
      (*deleted_)(state_, key.data(), key.size());
    }
  };
  H handler;
  handler.state_ = state;
  handler.put_ = put;
  handler.deleted_ = deleted;
  b->rep.Iterate(&handler);
}

void dLSM_writebatch_append(dLSM_writebatch_t* destination,
                               const dLSM_writebatch_t* source) {
  destination->rep.Append(source->rep);
}

dLSM_options_t* dLSM_options_create() { return new dLSM_options_t; }

void dLSM_options_destroy(dLSM_options_t* options) { delete options; }

void dLSM_options_set_comparator(dLSM_options_t* opt,
                                    dLSM_comparator_t* cmp) {
  opt->rep.comparator = cmp;
}

void dLSM_options_set_filter_policy(dLSM_options_t* opt,
                                       dLSM_filterpolicy_t* policy) {
  opt->rep.filter_policy = policy;
}

void dLSM_options_set_create_if_missing(dLSM_options_t* opt, uint8_t v) {
  opt->rep.create_if_missing = v;
}

void dLSM_options_set_error_if_exists(dLSM_options_t* opt, uint8_t v) {
  opt->rep.error_if_exists = v;
}

void dLSM_options_set_paranoid_checks(dLSM_options_t* opt, uint8_t v) {
  opt->rep.paranoid_checks = v;
}

void dLSM_options_set_env(dLSM_options_t* opt, dLSM_env_t* env) {
  opt->rep.env = (env ? env->rep : nullptr);
}

void dLSM_options_set_info_log(dLSM_options_t* opt, dLSM_logger_t* l) {
  opt->rep.info_log = (l ? l->rep : nullptr);
}

void dLSM_options_set_write_buffer_size(dLSM_options_t* opt, size_t s) {
  opt->rep.write_buffer_size = s;
}

void dLSM_options_set_max_open_files(dLSM_options_t* opt, int n) {
  opt->rep.max_open_files = n;
}

void dLSM_options_set_cache(dLSM_options_t* opt, dLSM_cache_t* c) {
  opt->rep.block_cache = c->rep;
}

void dLSM_options_set_block_size(dLSM_options_t* opt, size_t s) {
  opt->rep.block_size = s;
}

void dLSM_options_set_block_restart_interval(dLSM_options_t* opt, int n) {
  opt->rep.block_restart_interval = n;
}

void dLSM_options_set_max_file_size(dLSM_options_t* opt, size_t s) {
  opt->rep.max_file_size = s;
}

void dLSM_options_set_compression(dLSM_options_t* opt, int t) {
  opt->rep.compression = static_cast<CompressionType>(t);
}

dLSM_comparator_t* dLSM_comparator_create(
    void* state, void (*destructor)(void*),
    int (*compare)(void*, const char* a, size_t alen, const char* b,
                   size_t blen),
    const char* (*name)(void*)) {
  dLSM_comparator_t* result = new dLSM_comparator_t;
  result->state_ = state;
  result->destructor_ = destructor;
  result->compare_ = compare;
  result->name_ = name;
  return result;
}

void dLSM_comparator_destroy(dLSM_comparator_t* cmp) { delete cmp; }

dLSM_filterpolicy_t* dLSM_filterpolicy_create(
    void* state, void (*destructor)(void*),
    char* (*create_filter)(void*, const char* const* key_array,
                           const size_t* key_length_array, int num_keys,
                           size_t* filter_length),
    uint8_t (*key_may_match)(void*, const char* key, size_t length,
                             const char* filter, size_t filter_length),
    const char* (*name)(void*)) {
  dLSM_filterpolicy_t* result = new dLSM_filterpolicy_t;
  result->state_ = state;
  result->destructor_ = destructor;
  result->create_ = create_filter;
  result->key_match_ = key_may_match;
  result->name_ = name;
  return result;
}

void dLSM_filterpolicy_destroy(dLSM_filterpolicy_t* filter) {
  delete filter;
}

dLSM_filterpolicy_t* dLSM_filterpolicy_create_bloom(int bits_per_key) {
  // Make a dLSM_filterpolicy_t, but override all of its methods so
  // they delegate to a NewBloomFilterPolicy() instead of user
  // supplied C functions.
  struct Wrapper : public dLSM_filterpolicy_t {
    static void DoNothing(void*) {}

    ~Wrapper() { delete rep_; }
    const char* Name() const { return rep_->Name(); }
    void CreateFilter(const Slice* keys, int n, std::string* dst) const {
//      return rep_->CreateFilter(keys, n, dst);
    }
    bool KeyMayMatch(const Slice& key, const Slice& filter) const {
      return rep_->KeyMayMatch(key, filter);
    }

    const FilterPolicy* rep_;
  };
  Wrapper* wrapper = new Wrapper;
  wrapper->rep_ = NewBloomFilterPolicy(bits_per_key);
  wrapper->state_ = nullptr;
  wrapper->destructor_ = &Wrapper::DoNothing;
  return wrapper;
}

dLSM_readoptions_t* dLSM_readoptions_create() {
  return new dLSM_readoptions_t;
}

void dLSM_readoptions_destroy(dLSM_readoptions_t* opt) { delete opt; }

void dLSM_readoptions_set_verify_checksums(dLSM_readoptions_t* opt,
                                              uint8_t v) {
  opt->rep.verify_checksums = v;
}

void dLSM_readoptions_set_fill_cache(dLSM_readoptions_t* opt, uint8_t v) {
  opt->rep.fill_cache = v;
}

void dLSM_readoptions_set_snapshot(dLSM_readoptions_t* opt,
                                      const dLSM_snapshot_t* snap) {
  opt->rep.snapshot = (snap ? snap->rep : nullptr);
}

dLSM_writeoptions_t* dLSM_writeoptions_create() {
  return new dLSM_writeoptions_t;
}

void dLSM_writeoptions_destroy(dLSM_writeoptions_t* opt) { delete opt; }

void dLSM_writeoptions_set_sync(dLSM_writeoptions_t* opt, uint8_t v) {
  opt->rep.sync = v;
}

dLSM_cache_t* dLSM_cache_create_lru(size_t capacity) {
  dLSM_cache_t* c = new dLSM_cache_t;
  c->rep = NewLRUCache(capacity);
  return c;
}

void dLSM_cache_destroy(dLSM_cache_t* cache) {
  delete cache->rep;
  delete cache;
}

dLSM_env_t* dLSM_create_default_env() {
  dLSM_env_t* result = new dLSM_env_t;
  result->rep = Env::Default();
  result->is_default = true;
  return result;
}

void dLSM_env_destroy(dLSM_env_t* env) {
  if (!env->is_default) delete env->rep;
  delete env;
}

char* dLSM_env_get_test_directory(dLSM_env_t* env) {
  std::string result;
  if (!env->rep->GetTestDirectory(&result).ok()) {
    return nullptr;
  }

  char* buffer = static_cast<char*>(std::malloc(result.size() + 1));
  std::memcpy(buffer, result.data(), result.size());
  buffer[result.size()] = '\0';
  return buffer;
}

void dLSM_free(void* ptr) { std::free(ptr); }

int dLSM_major_version() { return kMajorVersion; }

int dLSM_minor_version() { return kMinorVersion; }

}  // end extern "C"
