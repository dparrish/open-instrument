/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "server/disk_datastore.h"
#include "lib/timer.h"

namespace openinstrument {

class DatastoreTest : public ::testing::Test {};


TEST_F(DatastoreTest, IteratorTest) {
  uint64_t start_timestamp = 1363940931985LL;
  uint64_t end_timestamp = 1363940951985LL;
  DiskDatastore datastore("/r2/services/openinstrument");
  DiskDatastoreIterator it(start_timestamp, end_timestamp);
  for (auto &variable : datastore.FindVariables("/*")) {
    LOG(INFO) << "Adding variable " << variable.ToString();
    it.AddStream(&datastore.GetVariable(variable));
  }
  ++it;
  int counter = 0;
  uint64_t last_timestamp = 0;
  for (; it != it.end(); ++it) {
    if (++counter >= 50)
      break;
    auto &value = *it;
    Variable var(value.variable());
    LOG(INFO) << var.ToString() << " " << value.timestamp() << ": " << std::fixed << value.double_value();
    ASSERT_TRUE(value.timestamp() >= last_timestamp);
    ASSERT_TRUE(value.timestamp() >= start_timestamp);
    last_timestamp = value.timestamp();
  }
  ASSERT_GT(counter, 0);
}

/*
TEST_F(DataStoreTest, SortOrdering) {
  HostPort *source = HostPort::get("foobar", 8000);
  ASSERT_TRUE(source);
  EXPECT_EQ(source->hostname(), "foobar");
  EXPECT_EQ(source->port(), 8000);
  for (int i = 0; i < 5; ++i) {
    Timer timer;
    timer.Start();
    LabelMap labels;
    labels["job"] = "testjob";
    labels["cluster"] = "home";
    labels["label3"] = "label3";
    Timestamp stamp;
    ASSERT_TRUE(datastore.record(source, "/test/key", labels, stamp.ms(), i));
    timer.Stop();
    VLOG(1) << "Time taken to add a " << stamp.ms() << " record is " << timer.ms() << " ms";
    usleep(1000);
  }

  // Verify that all the data comes out in sorted order
  ValueStream *stream = datastore.get_stream(source, "/test/key");
  vector<DataValue> values = stream->values();
  int64_t last = 0;
  for (int i = 0; i < 5; ++i) {
    if (last)
      ASSERT_GT(values[i].timestamp, last);
    last = values[i].timestamp;
  }
}

TEST_F(DataStoreTest, LowerBound) {
  add_test_data();
  HostPort *source = HostPort::get("foobar", 8000);
  ValueStream *stream = datastore.get_stream(source, "/test/key");
  ASSERT_TRUE(stream);
  EXPECT_EQ("/test/key", stream->variable());
  EXPECT_EQ("foobar", stream->source()->hostname());
  // At this point, there are 3 values added after item 2
  DataValueIterators iter = stream->lower_bound(2 * 1000 * 1000);
  int count = 0;
  for (; iter.iter != iter.end; ++iter.iter)
    ++count;
  EXPECT_EQ(3, count);
}

TEST_F(DataStoreTest, FetchRange) {
  add_test_data(10);
  HostPort *source = HostPort::get("foobar", 8000);
  ValueStream *stream = datastore.get_stream(source, "/test/key");
  ASSERT_TRUE(stream);
  EXPECT_EQ("/test/key", stream->variable());
  EXPECT_EQ("foobar", stream->source()->hostname());
  // The stream contains timestamps between 0 and 9000000, grab between 2000000 and 7000000
  DataValueIterators iter = stream->fetch_range(2 * 1000 * 1000, 7 * 1000 * 1000);
  int count = 0;
  for (; iter.iter != iter.end; ++iter.iter)
    ++count;
  EXPECT_EQ(6, count);
}
*/

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

