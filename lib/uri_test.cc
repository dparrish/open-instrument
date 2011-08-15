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
#include "lib/common.h"
#include "lib/uri.h"

using namespace openinstrument::http;

class UriTest : public ::testing::Test {};

TEST_F(UriTest, UriDecode) {
  string input_str = "/test/filename%20+%25%2d%2F";
  EXPECT_EQ("/test/filename  %-/", Uri::UrlDecode(input_str));
}

TEST_F(UriTest, UriEncode) {
  string input_str = "/test/filename  %-/";
  EXPECT_EQ("/test/filename%20%20%25%2d/", Uri::UrlEncode(input_str));
}

TEST_F(UriTest, JustPathParse) {
  string input_uri = "/path/to/filename?key=foo";
  Uri uri(input_uri);
  EXPECT_EQ("http", uri.protocol);
  EXPECT_EQ("", uri.hostname);
  EXPECT_EQ("", uri.username);
  EXPECT_EQ("", uri.password);
  EXPECT_EQ(80, uri.port);
  EXPECT_EQ("/path/to/filename", uri.path);
  EXPECT_EQ("key=foo", uri.query_string);
}

TEST_F(UriTest, ProbablyBadUriParse) {
  string input_uri = "host.domain.com/path/to/filename";
  Uri uri(input_uri);
  EXPECT_EQ("http", uri.protocol);
  EXPECT_EQ("host.domain.com", uri.hostname);
  EXPECT_EQ("", uri.username);
  EXPECT_EQ("", uri.password);
  EXPECT_EQ(80, uri.port);
  EXPECT_EQ("/path/to/filename", uri.path);
  EXPECT_EQ("", uri.query_string);
}

TEST_F(UriTest, SimpleUriParse) {
  string input_uri = "http://host.domain.com/path/to/filename";
  Uri uri(input_uri);
  EXPECT_EQ("http", uri.protocol);
  EXPECT_EQ("host.domain.com", uri.hostname);
  EXPECT_EQ("", uri.username);
  EXPECT_EQ("", uri.password);
  EXPECT_EQ(80, uri.port);
  EXPECT_EQ("/path/to/filename", uri.path);
  EXPECT_EQ("", uri.query_string);
}

TEST_F(UriTest, UriParse) {
  string input_uri = "http://username:password@host.domain.com:123/path/to/filename?key1=value1&key2=%20value2%20";
  Uri uri(input_uri);
  EXPECT_EQ("http", uri.protocol);
  EXPECT_EQ("username", uri.username);
  EXPECT_EQ("password", uri.password);
  EXPECT_EQ("host.domain.com", uri.hostname);
  EXPECT_EQ(123, uri.port);
  EXPECT_EQ("/path/to/filename", uri.path);
  EXPECT_EQ("key1=value1&key2=%20value2%20", uri.query_string);
  ASSERT_EQ(2, uri.params.size());
  EXPECT_EQ("key1", uri.params[0].key);
  EXPECT_EQ("value1", uri.params[0].value);
  EXPECT_EQ("key2", uri.params[1].key);
  EXPECT_EQ(" value2 ", uri.params[1].value);
}

TEST_F(UriTest, FullUriAssemble) {
  string input_uri = "http://username:password@host.domain.com:123/path/to/filename?key1=value1&key2=%20value2%20";
  Uri uri(input_uri);
  string output_uri = uri.Assemble();
  EXPECT_EQ(output_uri, input_uri);
}

TEST_F(UriTest, ShortUriAssemble) {
  string input_uri = "host.domain.com/path/to/filename";
  Uri uri(input_uri);
  EXPECT_EQ("http://host.domain.com/path/to/filename", uri.Assemble());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

