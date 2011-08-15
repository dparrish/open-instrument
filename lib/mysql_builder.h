/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_MYSQL_BUILDER_H_
#define OPENINSTRUMENT_LIB_MYSQL_BUILDER_H_

#include <mysql++.h>
#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/string.h"

namespace openinstrument {

class MysqlBuilder {
 public:
  MysqlBuilder() {}

  class Index {
   public:
    Index(const string &type, const string &name, const string &columns);
    const string &name() const;
    string ToString() const;

   private:
    string type_;
    string name_;
    vector<string> columns_;
  };

  class Column {
   public:
    Column(const string &name, const string &type);
    const string &name() const;
    bool equals(const Column &other);
    Column *primary_key(bool newval = true);
    bool is_primary_key() const;
    Column *set_unsigned(bool newval = true);
    Column *charset(const string &charset);
    Column *null(bool newval = true);
    Column *not_null(bool newval = false);
    Column *auto_increment(bool newval = true);
    Column *set_default(const string &newval);
    Column *set_default(uint64_t newval);
    Column *set_default(bool newval);
    Column *unique(bool newval = true);
    string ToString() const;

   private:
    string name_;
    string type_;
    bool unsigned_;
    string charset_;
    bool primary_key_;
    bool null_;
    bool unique_;
    bool auto_increment_;
    string default_;
  };

  class Table {
   public:
    explicit Table(const string &name);
    const string &name() const;
    Column *get_column(const string &name);
    Column *column(const string &name, const string &type);
    Table *index(const string &type, const string &name, const string &columns);
    Table *option(const string &option);
    string ToString() const;

    vector<Column> columns;
    vector<Index> indexes;
    vector<string> options;
   private:
    string name_;
  };

  Table *table(const string &name);
  string ToString() const;
  // Create a table instance from a CREATE TABLE string
  Table *from_string(const string &input);
  void Run(mysqlpp::Connection &conn);

 private:
  void drop_column(mysqlpp::Connection &conn, const string &table, const string &column);
  void add_column(mysqlpp::Connection &conn, const string &table, const string &definition);
  void modify_column(mysqlpp::Connection &conn, const string &table, const string &definition);
  bool streq(vector<string>::iterator start, const string &comp) const;
  bool streq(vector<string>::iterator start, size_t num_entries, const string &comp) const;

  vector<Table> tables_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_MYSQL_BUILDER_H_
