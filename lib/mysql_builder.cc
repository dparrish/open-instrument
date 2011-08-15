/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/tokenizer.hpp>
#include <glog/logging.h>
#include <mysql++/mystring.h>
#include <mysql++.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/mysql_builder.h"
#include "lib/string.h"

namespace openinstrument {

MysqlBuilder::Index::Index(const string &type, const string &name, const string &columns) : type_(type), name_(name) {
  boost::split(columns_, columns, boost::is_any_of("`',"), boost::token_compress_on);
}

const string &MysqlBuilder::Index::name() const {
  return name_;
}

string MysqlBuilder::Index::ToString() const {
  string str = StringPrintf("%s ", type_.c_str());
  if (name_.size())
    StringAppendf(&str, "`%s` ", name_.c_str());
  StringAppendf(&str, "(");
  for (vector<string>::const_iterator i = columns_.begin(); i != columns_.end(); ++i)
    StringAppendf(&str, "`%s`%s", i->c_str(), i == columns_.end() - 1 ? "" : ",");
  StringAppendf(&str, ")");
  return str;
}

MysqlBuilder::Column::Column(const string &name, const string &type)
  : name_(name),
    type_(type),
    unsigned_(false),
    primary_key_(false),
    null_(true),
    unique_(false),
    auto_increment_(false),
    default_("") {}

const string &MysqlBuilder::Column::name() const {
  return name_;
}

bool MysqlBuilder::Column::equals(const MysqlBuilder::Column &other) {
  if (strcasecmp(name_.c_str(), other.name_.c_str()) != 0)
    return false;
  if (strcasecmp(type_.c_str(), other.type_.c_str()) != 0)
    return false;
  if (strcasecmp(default_.c_str(), other.default_.c_str()) != 0)
    return false;
  if (null_ != other.null_)
    return false;
  if (unsigned_ != other.unsigned_)
    return false;
  if (auto_increment_ != other.auto_increment_)
    return false;
  if (primary_key_ != other.primary_key_)
    return false;
  return true;
}

MysqlBuilder::Column *MysqlBuilder::Column::primary_key(bool newval) {
  primary_key_ = newval;
  if (newval)
    null_ = false;
  return this;
}

bool MysqlBuilder::Column::is_primary_key() const {
  return primary_key_;
}

MysqlBuilder::Column *MysqlBuilder::Column::set_unsigned(bool newval) {
  unsigned_ = newval;
  return this;
}

MysqlBuilder::Column *MysqlBuilder::Column::charset(const string &charset) {
  charset_ = charset;
  return this;
}

MysqlBuilder::Column *MysqlBuilder::Column::null(bool newval) {
  null_ = newval;
  return this;
}

MysqlBuilder::Column *MysqlBuilder::Column::not_null(bool newval) {
  null_ = newval;
  return this;
}

MysqlBuilder::Column *MysqlBuilder::Column::auto_increment(bool newval) {
  auto_increment_ = newval;
  return this;
}

MysqlBuilder::Column *MysqlBuilder::Column::set_default(const string &newval) {
  default_ = newval;
  return this;
}

MysqlBuilder::Column *MysqlBuilder::Column::set_default(uint64_t newval) {
  default_ = lexical_cast<string>(newval);
  return this;
}

MysqlBuilder::Column *MysqlBuilder::Column::set_default(bool newval) {
  default_ = lexical_cast<string>(newval);
  return this;
}

MysqlBuilder::Column *MysqlBuilder::Column::unique(bool newval) {
  unique_ = newval;
  return this;
}

string MysqlBuilder::Column::ToString() const {
  string ret;
  StringAppendf(&ret, "`%s` %s", name_.c_str(), type_.c_str());
  if (unsigned_)
    StringAppendf(&ret, " UNSIGNED");
  if (charset_.size())
    StringAppendf(&ret, " CHARACTER SET %s", charset_.c_str());
  if (!null_)
    StringAppendf(&ret, " NOT NULL");
  if (default_.size())
    StringAppendf(&ret, " DEFAULT '%s'", default_.c_str());
  if (auto_increment_)
    StringAppendf(&ret, " AUTO_INCREMENT");
  return ret;
}

MysqlBuilder::Table::Table(const string &name) : name_(name) {}

const string &MysqlBuilder::Table::name() const {
  return name_;
}

MysqlBuilder::Column *MysqlBuilder::Table::get_column(const string &name) {
  BOOST_FOREACH(MysqlBuilder::Column &i, columns) {
    if (name == i.name())
      return &i;
  }
  return NULL;
}

MysqlBuilder::Column *MysqlBuilder::Table::column(const string &name, const string &type) {
  columns.push_back(MysqlBuilder::Column(name, type));
  return &(columns.back());
}

MysqlBuilder::Table *MysqlBuilder::Table::index(const string &type, const string &name, const string &columns) {
  indexes.push_back(Index(type, name, columns));
  return this;
}

MysqlBuilder::Table *MysqlBuilder::Table::option(const string &option) {
  options.push_back(option);
  return this;
}

string MysqlBuilder::Table::ToString() const {
  string ret;
  StringAppendf(&ret, "CREATE TABLE `%s` (\n", name_.c_str());
  BOOST_FOREACH(const Column &i, columns) {
    ret += "  " + i.ToString() + ",\n";
  }
  BOOST_FOREACH(const Index &i, indexes) {
    ret += "  " + i.ToString() + ",\n";
  }
  ret.erase(ret.size() - 2);
  StringAppendf(&ret, "\n)");
  BOOST_FOREACH(const string &i, options) {
    StringAppendf(&ret, " %s", i.c_str());
  }
  ret += ";";
  return ret;
}

MysqlBuilder::Table *MysqlBuilder::table(const string &name) {
  tables_.push_back(MysqlBuilder::Table(name));
  return &(tables_.back());
}

string MysqlBuilder::ToString() const {
  string ret;
  BOOST_FOREACH(const Table &i, tables_) {
    StringAppendf(&ret, "%s\n", i.ToString().c_str());
  }
  return ret;
}

// Create a table instance from a CREATE TABLE string
MysqlBuilder::Table *MysqlBuilder::from_string(const string &input) {
  typedef boost::tokenizer<boost::escaped_list_separator<char> > Tokenizer;
  boost::escaped_list_separator<char> sep("\\", " \n\t;", "\"'`");
  Tokenizer tok(input, sep);
  vector<string> tokens;
  for (Tokenizer::iterator i = tok.begin(); i != tok.end(); ++i) {
    if (i->empty())
      continue;
    if (i->at(i->size() - 1) == ',') {
      // end of column options
      tokens.push_back(i->substr(0, i->size() - 1));
      tokens.push_back(",");
    } else {
      tokens.push_back(*i);
    }
  }

  MysqlBuilder::Table *t = NULL;
  MysqlBuilder::Column *c = NULL;
  string colname;
  enum {
    STATEMENT = 0,
    COLUMN_NAME,
    COLUMN_TYPE,
    COLUMN_OPTIONS,
    TABLE_OPTIONS,
    INDEX,
    PRIMARY_KEY,
    FOREIGN_KEY,
    UNIQUE_INDEX,
    UNIQUE_KEY,
  } state = STATEMENT;
  for (vector<string>::iterator i = tokens.begin(); i != tokens.end(); ++i) {
    switch (state) {
      case STATEMENT:
        if (streq(i, 2, "CREATE TABLE")) {
          ++i;
          if (streq(i + 1, 3, "IF NOT EXISTS"))
            i += 3;
          break;
        }
        if (*i == "(") {
          if (t) {
            state = COLUMN_NAME;
            break;
          }
          LOG(ERROR) << "Invalid SQL syntax";
          return NULL;
        }
        if (t) {
          LOG(ERROR) << "Invalid SQL syntax";
          return NULL;
        }
        t = table(*i);
        break;

      case COLUMN_NAME:
        if (streq(i, "CONSTRAINT")) {
          if (streq(i + 1, 2, "PRIMARY KEY")) {
            state = PRIMARY_KEY;
            i += 2;
          } else if (streq(i + 2, 2, "PRIMARY KEY")) {
            state = PRIMARY_KEY;
            i += 3;
          } else if (streq(i + 1, 2, "FOREIGN KEY")) {
            state = FOREIGN_KEY;
            i += 2;
          } else if (streq(i + 2, 2, "FOREIGN KEY")) {
            state = FOREIGN_KEY;
            i += 3;
          } else {
            throw "Unknown data type";
          }
          break;
        } else if (streq(i, 2, "UNIQUE INDEX")) {
          state = UNIQUE_INDEX;
          ++i;
          break;
        } else if (streq(i, 2, "UNIQUE KEY")) {
          state = UNIQUE_KEY;
          ++i;
          break;
        } else if (streq(i, "INDEX") || streq(i, "KEY")) {
          state = INDEX;
          break;
        } else if (streq(i, 2, "PRIMARY KEY")) {
          state = PRIMARY_KEY;
          ++i;
          break;
        }
        colname = *i;
        state = COLUMN_TYPE;
        break;

      case COLUMN_TYPE:
        {
          string coltype(*i);
          if (strcasecmp(coltype.c_str(), "tinyint") == 0)
            coltype = "tinyint(4)";
          else if (strcasecmp(coltype.c_str(), "smallint") == 0)
            coltype = "smallint(6)";
          else if (strcasecmp(coltype.c_str(), "mediumint") == 0)
            coltype = "mediumint(9)";
          else if (strcasecmp(coltype.c_str(), "int") == 0)
            coltype = "int(11)";
          else if (strcasecmp(coltype.c_str(), "integer") == 0)
            coltype = "int(11)";
          else if (strcasecmp(coltype.c_str(), "bigint") == 0)
            coltype = "bigint(20)";
          else if (strcasecmp(coltype.c_str(), "real") == 0)
            coltype = "double";
          c = t->column(colname, coltype);
          colname.clear();
          state = COLUMN_OPTIONS;
          break;
        }

      case COLUMN_OPTIONS:
        if (*i == ")") {
          // end of columns
          c = NULL;
          state = TABLE_OPTIONS;
          break;
        }
        if (*i == ",") {
          c = NULL;
          state = COLUMN_NAME;
          break;
        }
        if (streq(i, 2, "NOT NULL")) {
          c->not_null();
          ++i;
        } else if (streq(i, "NULL")) {
          c->null();
        } else if (streq(i, "AUTO_INCREMENT")) {
          c->auto_increment();
        } else if (streq(i, 2, "PRIMARY KEY")) {
          t->index("PRIMARY KEY", "", c->name());
          ++i;
        } else if (streq(i, 2, "UNIQUE KEY")) {
          t->index("UNIQUE KEY", "", c->name());
          ++i;
        } else if (streq(i, "UNIQUE")) {
          t->index("UNIQUE KEY", "", c->name());
        } else if (streq(i, "UNSIGNED")) {
          c->set_unsigned();
        } else if (streq(i, "DEFAULT")) {
          if (*(i + 1) == "NULL")
            c->null();
          else
            c->set_default(*(i + 1));
          ++i;
        } else {
          LOG(WARNING) << "  unknown column definition '" << *i << "'";
        }
        break;

      case TABLE_OPTIONS:
        t->option(*i);
        break;

      case INDEX:
        if (*i == ")") {
          // end of columns
          c = NULL;
          state = TABLE_OPTIONS;
          break;
        }
        if (*i == ",") {
          c = NULL;
          state = COLUMN_NAME;
          break;
        }
        if (i->at(0) == '(') {
          t->index("INDEX", "", i->substr(1, i->size() - 2));
        } else {
          t->index("INDEX", *i, (i + 1)->substr(1, (i + 1)->size() - 2));
          ++i;
        }
        break;

      case PRIMARY_KEY:
        if (*i == ")") {
          // end of columns
          c = NULL;
          state = TABLE_OPTIONS;
          break;
        }
        if (*i == ",") {
          c = NULL;
          state = COLUMN_NAME;
          break;
        }
        if (i->at(0) == '(') {
          // This is the column name
          string colname(i->substr(1, i->size() - 2));
          MysqlBuilder::Column *column = t->get_column(colname);
          if (!column)
            throw "PRIMARY_KEY entry specifies unknown column";
          t->index("PRIMARY KEY", "", colname);
        } else {
          // This is the index name
        }
        break;

      case FOREIGN_KEY:
        if (*i == ")") {
          // end of columns
          c = NULL;
          state = TABLE_OPTIONS;
          break;
        }
        if (*i == ",") {
          c = NULL;
          state = COLUMN_NAME;
          break;
        }
        break;

      case UNIQUE_INDEX:
        if (*i == ")") {
          // end of columns
          c = NULL;
          state = TABLE_OPTIONS;
          break;
        }
        if (*i == ",") {
          c = NULL;
          state = COLUMN_NAME;
          break;
        }
        if (i->at(0) == '(') {
          t->index("UNIQUE INDEX", "", i->substr(1, i->size() - 2));
        } else {
          t->index("UNIQUE INDEX", *i, (i + 1)->substr(1, (i + 1)->size() - 2));
          ++i;
        }
        break;

      case UNIQUE_KEY:
        if (*i == ")") {
          // end of columns
          c = NULL;
          state = TABLE_OPTIONS;
          break;
        }
        if (*i == ",") {
          c = NULL;
          state = COLUMN_NAME;
          break;
        }
        if (i->at(0) == '(') {
          t->index("UNIQUE KEY", "", i->substr(1, i->size() - 2));
        } else {
          t->index("UNIQUE KEY", *i, (i + 1)->substr(1, (i + 1)->size() - 2));
          ++i;
        }
        break;

      default:
        break;
    }
  }
  return t;
}

void MysqlBuilder::Run(mysqlpp::Connection &conn) {
  BOOST_FOREACH(Table &i, tables_) {
    try {
      mysqlpp::Query query = conn.query();
      query << "SHOW CREATE TABLE `" << i.name() << "`";
      mysqlpp::StoreQueryResult res = query.store();
      MysqlBuilder existing;
      MysqlBuilder::Table *table = existing.from_string(string(res[0]["Create Table"]));
      BOOST_FOREACH(Column &col, table->columns) {
        // Look for any existing columns that should be dropped
        MysqlBuilder::Column *c = i.get_column(col.name());
        if (!c) {
          drop_column(conn, i.name(), col.name());
          continue;
        }
      }
      for (vector<MysqlBuilder::Column>::iterator k = i.columns.begin(); k != i.columns.end(); ++k) {
        MysqlBuilder::Column *newcolumn = table->get_column(k->name());
        if (!newcolumn) {
          string desc = k->ToString();
          if (k == i.columns.begin())
            desc += " FIRST";
          else
            StringAppendf(&desc, " AFTER `%s`", (k - 1)->name().c_str());
          add_column(conn, i.name(), desc);
        } else {
          if (!k->equals(*newcolumn)) {
            modify_column(conn, i.name(), k->ToString());
            continue;
          }
        }
      }
    } catch (mysqlpp::BadQuery) {
      // Table doesn't exist, create it
      LOG(INFO) << i.ToString();
      mysqlpp::Query query = conn.query();
      query << i.ToString();
      mysqlpp::SimpleResult res = query.execute();
    }
  }
}

void MysqlBuilder::drop_column(mysqlpp::Connection &conn, const string &table, const string &column) {
  mysqlpp::Query query = conn.query();
  query << "ALTER TABLE `" << table << "` DROP COLUMN `" << column << "`";
  LOG(INFO) << query;
  mysqlpp::SimpleResult res = query.execute();
}

void MysqlBuilder::add_column(mysqlpp::Connection &conn, const string &table, const string &definition) {
  mysqlpp::Query query = conn.query();
  query << "ALTER TABLE `" << table << "` ADD COLUMN " << definition << "";
  LOG(INFO) << query;
  try {
    mysqlpp::SimpleResult res = query.execute();
  } catch (mysqlpp::BadQuery &e) {
    LOG(WARNING) << "Error adding column " << definition << ": " << e.what();
  }
}

void MysqlBuilder::modify_column(mysqlpp::Connection &conn, const string &table, const string &definition) {
  mysqlpp::Query query = conn.query();
  query << "ALTER TABLE `" << table << "` MODIFY COLUMN " << definition << "";
  LOG(INFO) << query;
  mysqlpp::SimpleResult res = query.execute();
}

bool MysqlBuilder::streq(vector<string>::iterator start, const string &comp) const {
  return strcasecmp(start->c_str(), comp.c_str()) == 0;
}

bool MysqlBuilder::streq(vector<string>::iterator start, size_t num_entries, const string &comp) const {
  string str;
  for (size_t i = 0; i < num_entries; ++i)
    StringAppendf(&str, "%s ", (start + i)->c_str());
  if (str.size())
    str.erase(str.size() - 1);
  return strcasecmp(str.c_str(), comp.c_str()) == 0;
}

}  // namespace openinstrument
