/* Copyright (c) 2011 The dLSM Authors. All rights reserved.
  Use of this source code is governed by a BSD-style license that can be
  found in the LICENSE file. See the AUTHORS file for names of contributors.

  C bindings for dLSM.  May be useful as a stable ABI that can be
  used by programs that keep dLSM in a shared library, or for
  a JNI api.

  Does not support:
  . getters for the option types
  . custom comparators that implement key shortening
  . custom iter, db, env, table_cache implementations using just the C bindings

  Some conventions:

  (1) We expose just opaque struct pointers and functions to clients.
  This allows us to change internal representations without having to
  recompile clients.

  (2) For simplicity, there is no equivalent to the Slice type.  Instead,
  the caller has to pass the pointer and length as separate
  arguments.

  (3) Errors are represented by a null-terminated c string.  NULL
  means no error.  All operations that can raise an error are passed
  a "char** errptr" as the last argument.  One of the following must
  be true on entry:
     *errptr == NULL
     *errptr points to a malloc()ed null-terminated error message
       (On Windows, *errptr must have been malloc()-ed by this library.)
  On success, a dLSM routine leaves *errptr unchanged.
  On failure, dLSM frees the old value of *errptr and
  set *errptr to a malloc()ed error message.

  (4) Bools have the type uint8_t (0 == false; rest == true)

  (5) All of the pointer arguments must be non-NULL.
*/

#ifndef STORAGE_dLSM_INCLUDE_C_H_
#define STORAGE_dLSM_INCLUDE_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "dLSM/export.h"

/* Exported types */

typedef struct dLSM_t dLSM_t;
typedef struct dLSM_cache_t dLSM_cache_t;
typedef struct dLSM_comparator_t dLSM_comparator_t;
typedef struct dLSM_env_t dLSM_env_t;
typedef struct dLSM_filelock_t dLSM_filelock_t;
typedef struct dLSM_filterpolicy_t dLSM_filterpolicy_t;
typedef struct dLSM_iterator_t dLSM_iterator_t;
typedef struct dLSM_logger_t dLSM_logger_t;
typedef struct dLSM_options_t dLSM_options_t;
typedef struct dLSM_randomfile_t dLSM_randomfile_t;
typedef struct dLSM_readoptions_t dLSM_readoptions_t;
typedef struct dLSM_seqfile_t dLSM_seqfile_t;
typedef struct dLSM_snapshot_t dLSM_snapshot_t;
typedef struct dLSM_writablefile_t dLSM_writablefile_t;
typedef struct dLSM_writebatch_t dLSM_writebatch_t;
typedef struct dLSM_writeoptions_t dLSM_writeoptions_t;

/* DB operations */

dLSM_EXPORT dLSM_t* dLSM_open(const dLSM_options_t* options,
                                       const char* name, char** errptr);

dLSM_EXPORT void dLSM_close(dLSM_t* db);

dLSM_EXPORT void dLSM_put(dLSM_t* db,
                                const dLSM_writeoptions_t* options,
                                const char* key, size_t keylen, const char* val,
                                size_t vallen, char** errptr);

dLSM_EXPORT void dLSM_delete(dLSM_t* db,
                                   const dLSM_writeoptions_t* options,
                                   const char* key, size_t keylen,
                                   char** errptr);

dLSM_EXPORT void dLSM_write(dLSM_t* db,
                                  const dLSM_writeoptions_t* options,
                                  dLSM_writebatch_t* batch, char** errptr);

/* Returns NULL if not found.  A malloc()ed array otherwise.
   Stores the length of the array in *vallen. */
dLSM_EXPORT char* dLSM_get(dLSM_t* db,
                                 const dLSM_readoptions_t* options,
                                 const char* key, size_t keylen, size_t* vallen,
                                 char** errptr);

dLSM_EXPORT dLSM_iterator_t* dLSM_create_iterator(
    dLSM_t* db, const dLSM_readoptions_t* options);

dLSM_EXPORT const dLSM_snapshot_t* dLSM_create_snapshot(dLSM_t* db);

dLSM_EXPORT void dLSM_release_snapshot(
    dLSM_t* db, const dLSM_snapshot_t* snapshot);

/* Returns NULL if property name is unknown.
   Else returns a pointer to a malloc()-ed null-terminated value. */
dLSM_EXPORT char* dLSM_property_value(dLSM_t* db,
                                            const char* propname);

dLSM_EXPORT void dLSM_approximate_sizes(
    dLSM_t* db, int num_ranges, const char* const* range_start_key,
    const size_t* range_start_key_len, const char* const* range_limit_key,
    const size_t* range_limit_key_len, uint64_t* sizes);

dLSM_EXPORT void dLSM_compact_range(dLSM_t* db, const char* start_key,
                                          size_t start_key_len,
                                          const char* limit_key,
                                          size_t limit_key_len);

/* Management operations */

dLSM_EXPORT void dLSM_destroy_db(const dLSM_options_t* options,
                                       const char* name, char** errptr);

dLSM_EXPORT void dLSM_repair_db(const dLSM_options_t* options,
                                      const char* name, char** errptr);

/* Iterator */

dLSM_EXPORT void dLSM_iter_destroy(dLSM_iterator_t*);
dLSM_EXPORT uint8_t dLSM_iter_valid(const dLSM_iterator_t*);
dLSM_EXPORT void dLSM_iter_seek_to_first(dLSM_iterator_t*);
dLSM_EXPORT void dLSM_iter_seek_to_last(dLSM_iterator_t*);
dLSM_EXPORT void dLSM_iter_seek(dLSM_iterator_t*, const char* k,
                                      size_t klen);
dLSM_EXPORT void dLSM_iter_next(dLSM_iterator_t*);
dLSM_EXPORT void dLSM_iter_prev(dLSM_iterator_t*);
dLSM_EXPORT const char* dLSM_iter_key(const dLSM_iterator_t*,
                                            size_t* klen);
dLSM_EXPORT const char* dLSM_iter_value(const dLSM_iterator_t*,
                                              size_t* vlen);
dLSM_EXPORT void dLSM_iter_get_error(const dLSM_iterator_t*,
                                           char** errptr);

/* Write batch */

dLSM_EXPORT dLSM_writebatch_t* dLSM_writebatch_create(void);
dLSM_EXPORT void dLSM_writebatch_destroy(dLSM_writebatch_t*);
dLSM_EXPORT void dLSM_writebatch_clear(dLSM_writebatch_t*);
dLSM_EXPORT void dLSM_writebatch_put(dLSM_writebatch_t*,
                                           const char* key, size_t klen,
                                           const char* val, size_t vlen);
dLSM_EXPORT void dLSM_writebatch_delete(dLSM_writebatch_t*,
                                              const char* key, size_t klen);
dLSM_EXPORT void dLSM_writebatch_iterate(
    const dLSM_writebatch_t*, void* state,
    void (*put)(void*, const char* k, size_t klen, const char* v, size_t vlen),
    void (*deleted)(void*, const char* k, size_t klen));
dLSM_EXPORT void dLSM_writebatch_append(
    dLSM_writebatch_t* destination, const dLSM_writebatch_t* source);

/* Options */

dLSM_EXPORT dLSM_options_t* dLSM_options_create(void);
dLSM_EXPORT void dLSM_options_destroy(dLSM_options_t*);
dLSM_EXPORT void dLSM_options_set_comparator(dLSM_options_t*,
                                                   dLSM_comparator_t*);
dLSM_EXPORT void dLSM_options_set_filter_policy(dLSM_options_t*,
                                                      dLSM_filterpolicy_t*);
dLSM_EXPORT void dLSM_options_set_create_if_missing(dLSM_options_t*,
                                                          uint8_t);
dLSM_EXPORT void dLSM_options_set_error_if_exists(dLSM_options_t*,
                                                        uint8_t);
dLSM_EXPORT void dLSM_options_set_paranoid_checks(dLSM_options_t*,
                                                        uint8_t);
dLSM_EXPORT void dLSM_options_set_env(dLSM_options_t*, dLSM_env_t*);
dLSM_EXPORT void dLSM_options_set_info_log(dLSM_options_t*,
                                                 dLSM_logger_t*);
dLSM_EXPORT void dLSM_options_set_write_buffer_size(dLSM_options_t*,
                                                          size_t);
dLSM_EXPORT void dLSM_options_set_max_open_files(dLSM_options_t*, int);
dLSM_EXPORT void dLSM_options_set_cache(dLSM_options_t*,
                                              dLSM_cache_t*);
dLSM_EXPORT void dLSM_options_set_block_size(dLSM_options_t*, size_t);
dLSM_EXPORT void dLSM_options_set_block_restart_interval(
    dLSM_options_t*, int);
dLSM_EXPORT void dLSM_options_set_max_file_size(dLSM_options_t*,
                                                      size_t);

enum { dLSM_no_compression = 0, dLSM_snappy_compression = 1 };
dLSM_EXPORT void dLSM_options_set_compression(dLSM_options_t*, int);

/* Comparator */

dLSM_EXPORT dLSM_comparator_t* dLSM_comparator_create(
    void* state, void (*destructor)(void*),
    int (*compare)(void*, const char* a, size_t alen, const char* b,
                   size_t blen),
    const char* (*name)(void*));
dLSM_EXPORT void dLSM_comparator_destroy(dLSM_comparator_t*);

/* Filter policy */

dLSM_EXPORT dLSM_filterpolicy_t* dLSM_filterpolicy_create(
    void* state, void (*destructor)(void*),
    char* (*create_filter)(void*, const char* const* key_array,
                           const size_t* key_length_array, int num_keys,
                           size_t* filter_length),
    uint8_t (*key_may_match)(void*, const char* key, size_t length,
                             const char* filter, size_t filter_length),
    const char* (*name)(void*));
dLSM_EXPORT void dLSM_filterpolicy_destroy(dLSM_filterpolicy_t*);

dLSM_EXPORT dLSM_filterpolicy_t* dLSM_filterpolicy_create_bloom(
    int bits_per_key);

/* Read options */

dLSM_EXPORT dLSM_readoptions_t* dLSM_readoptions_create(void);
dLSM_EXPORT void dLSM_readoptions_destroy(dLSM_readoptions_t*);
dLSM_EXPORT void dLSM_readoptions_set_verify_checksums(
    dLSM_readoptions_t*, uint8_t);
dLSM_EXPORT void dLSM_readoptions_set_fill_cache(dLSM_readoptions_t*,
                                                       uint8_t);
dLSM_EXPORT void dLSM_readoptions_set_snapshot(dLSM_readoptions_t*,
                                                     const dLSM_snapshot_t*);

/* Write options */

dLSM_EXPORT dLSM_writeoptions_t* dLSM_writeoptions_create(void);
dLSM_EXPORT void dLSM_writeoptions_destroy(dLSM_writeoptions_t*);
dLSM_EXPORT void dLSM_writeoptions_set_sync(dLSM_writeoptions_t*,
                                                  uint8_t);

/* Cache */

dLSM_EXPORT dLSM_cache_t* dLSM_cache_create_lru(size_t capacity);
dLSM_EXPORT void dLSM_cache_destroy(dLSM_cache_t* cache);

/* Env */

dLSM_EXPORT dLSM_env_t* dLSM_create_default_env(void);
dLSM_EXPORT void dLSM_env_destroy(dLSM_env_t*);

/* If not NULL, the returned buffer must be released using dLSM_free(). */
dLSM_EXPORT char* dLSM_env_get_test_directory(dLSM_env_t*);

/* Utility */

/* Calls free(ptr).
   REQUIRES: ptr was malloc()-ed and returned by one of the routines
   in this file.  Note that in certain cases (typically on Windows), you
   may need to call this routine instead of free(ptr) to dispose of
   malloc()-ed memory returned by this library. */
dLSM_EXPORT void dLSM_free(void* ptr);

/* Return the major version number for this release. */
dLSM_EXPORT int dLSM_major_version(void);

/* Return the minor version number for this release. */
dLSM_EXPORT int dLSM_minor_version(void);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* STORAGE_dLSM_INCLUDE_C_H_ */
