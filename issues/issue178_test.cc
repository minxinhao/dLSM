// Copyright (c) 2013 The dLSM Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Test for issue 178: a manual compaction causes deleted data to reappear.
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"
#include "dLSM/db.h"
#include "dLSM/write_batch.h"
#include "util/testutil.h"

namespace {

const int kNumKeys = 1100000;

std::string Key1(int i) {
  char buf[100];
  std::snprintf(buf, sizeof(buf), "my_key_%d", i);
  return buf;
}

std::string Key2(int i) { return Key1(i) + "_xxx"; }

TEST(Issue178, Test) {
  // Get rid of any state from an old run.
  std::string dbpath = testing::TempDir() + "dLSM_cbug_test";
  DestroyDB(dbpath, dLSM::Options());

  // Open database.  Disable compression since it affects the creation
  // of layers and the code below is trying to test against a very
  // specific scenario.
  dLSM::DB* db;
  dLSM::Options db_options;
  db_options.create_if_missing = true;
  db_options.compression = dLSM::kNoCompression;
  ASSERT_dLSM_OK(dLSM::DB::Open(db_options, dbpath, &db));

  // create first key range
  dLSM::WriteBatch batch;
  for (size_t i = 0; i < kNumKeys; i++) {
    batch.Put(Key1(i), "value for range 1 key");
  }
  ASSERT_dLSM_OK(db->Write(dLSM::WriteOptions(), &batch));

  // create second key range
  batch.Clear();
  for (size_t i = 0; i < kNumKeys; i++) {
    batch.Put(Key2(i), "value for range 2 key");
  }
  ASSERT_dLSM_OK(db->Write(dLSM::WriteOptions(), &batch));

  // delete second key range
  batch.Clear();
  for (size_t i = 0; i < kNumKeys; i++) {
    batch.Delete(Key2(i));
  }
  ASSERT_dLSM_OK(db->Write(dLSM::WriteOptions(), &batch));

  // compact database
  std::string start_key = Key1(0);
  std::string end_key = Key1(kNumKeys - 1);
  dLSM::Slice least(start_key.data(), start_key.size());
  dLSM::Slice greatest(end_key.data(), end_key.size());

  // commenting out the line below causes the example to work correctly
  db->CompactRange(&least, &greatest);

  // count the keys
  dLSM::Iterator* iter = db->NewIterator(dLSM::ReadOptions());
  size_t num_keys = 0;
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    num_keys++;
  }
  delete iter;
  ASSERT_EQ(kNumKeys, num_keys) << "Bad number of keys";

  // close database
  delete db;
  DestroyDB(dbpath, dLSM::Options());
}

}  // anonymous namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
