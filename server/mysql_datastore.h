/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include "lib/mysql_builder.h"
#include "mysql/errmsg.h"
#include <mysql++.h>
#include <mysql++/mystring.h>

namespace openinstrument {

class Value {
 public:
  Value() : timestamp(0), value(0) {}
  Value(const string &variable, const string &host, const string &instance, uint64_t timestamp, double value)
    : variable(variable),
      host(host),
      instance(instance),
      timestamp(timestamp),
      value(value) {}

  string variable;
  string host;
  string instance;
  Timestamp timestamp;
  double value;

  bool valid() const {
    return timestamp.ms() != 0;
  }
};


class MysqlDatastore {
 public:
  MysqlDatastore() : conn_() {}

  bool connect(const string &database, const string &host, const string &username, const string &password) {
    database_ = database;
    host_ = host;
    username_ = username;
    password_ = password;
    reconnect();
    build_db();
    return true;
  }

  // Return a vector of all variables that start with the given prefix.
  void ListVariables(const string &prefix, vector<string> *vars) {
    CatchMysqlErrors(bind(&MysqlDatastore::_list_variables, this, prefix, vars));
  }

  void Record(const string &variable, const string &host, const string &instance, Timestamp timestamp, double value) {
    CatchMysqlErrors(bind(&MysqlDatastore::_record, this, variable, host, instance, timestamp, value));
  }

  void Record(const string &variable, const string &host, const string &instance, double value) {
    CatchMysqlErrors(bind(&MysqlDatastore::_record, this, variable, host, instance, Timestamp::Now(), value));
  }

  Value GetLatest(const string &variable) {
    Value value;
    CatchMysqlErrors(bind(&MysqlDatastore::_get_latest, this, variable, &value));
    return value;
  }

  void GetRange(const string &variable, const Timestamp &start, const Timestamp &end, proto::ValueStream *stream) {
    CatchMysqlErrors(bind(&MysqlDatastore::_get_range, this, variable, start, end, stream));
  }

 private:
  bool reconnect() {
    conn_.connect(database_.c_str(), host_.c_str(), username_.c_str(), password_.c_str());
    return true;
  }

  void build_db() {
    MysqlBuilder builder;
    builder.from_string(
      "CREATE TABLE variables ("
      "  id             BIGINT(20) NOT NULL PRIMARY KEY AUTO_INCREMENT,"
      "  variable       VARCHAR(255) NOT NULL,"
      "  unique index   (variable)\n"
      ") CHARSET=utf8");

    builder.from_string(
      "CREATE TABLE values ("
      "  variable       BIGINT not null,"
      "  when           BIGINT not null,"
      "  value          DOUBLE not null,"
      "  hostname       VARCHAR(255) NULL,"
      "  instance       VARCHAR(255) NULL,"
      "  index          (variable),"
      "  index          (when)\n"
      ") CHARSET=utf8");

    builder.Run(conn_);
  }

  // Run a callback, catching the standard MySQL errors.
  // If the connection is broken, reconnect() is run and the requested is attempted again, up to 10 times. Any other
  // errors will be passed on.
  template<typename F> void CatchMysqlErrors(F callback) {
    for (size_t i = 0; i < 10; i++) {
      try {
        callback();
        return;
      } catch (mysqlpp::ConnectionFailed) {
        LOG(WARNING) << "Mysql connection broken";
        reconnect();
        continue;
      } catch (mysqlpp::BadQuery &e) {
        if (e.errnum() == CR_SERVER_GONE_ERROR || e.errnum() == CR_SERVER_LOST) {
          LOG(WARNING) << "Mysql connection broken";
          reconnect();
          continue;
        }
        LOG(WARNING) << "Unknown mysql error " << e.errnum() << ": " << e.what();
        throw e;
      }
    }
    throw runtime_error("Too many attempts to contact MySQL");
  }

  void _get_ts_id(const string &variable, uint64_t *out) {
    unordered_map<string, uint64_t>::iterator it = tsids_.find(variable);
    if (it != tsids_.end()) {
      *out = it->second;
      return;
    }
    mysqlpp::Query query = conn_.query();
    query << "SELECT id FROM variables WHERE variable = " << mysqlpp::quote << variable;
    mysqlpp::StoreQueryResult res = query.store();
    if (res.num_rows() != 1) {
      // Can't find variable
      *out = 0;
      return;
    }
    tsids_[variable] = *out = res[0][0];
  }

  uint64_t get_ts_id(const string &variable) {
    uint64_t out = 0;
    CatchMysqlErrors(bind(&MysqlDatastore::_get_ts_id, this, variable, &out));
    return out;
  }

  void _create_variable(const string &variable, uint64_t *out) {
    mysqlpp::Query query = conn_.query();
    query << "INSERT INTO variables (variable) VALUES (" << mysqlpp::quote << variable << ")";
    *out = 0;
    mysqlpp::SimpleResult res = query.execute();
    VLOG(1) << "Created variable " << variable << " (" << res.insert_id() << ")";
    tsids_[variable] = *out = res.insert_id();
  }

  uint64_t CreateVariable(const string &variable) {
    uint64_t out = 0;
    CatchMysqlErrors(bind(&MysqlDatastore::_create_variable, this, variable, &out));
    return out;
  }

  void _list_variables(const string &prefix, vector<string> *vars) {
    mysqlpp::Query query = conn_.query();
    query << "SELECT variable FROM variables WHERE SUBSTR(variable, 1, " << prefix.size() << ") = " << mysqlpp::quote
          << prefix;
    VLOG(1) << "Listing variables: " << query;
    mysqlpp::StoreQueryResult res = query.store();
    for (size_t i = 0; i < res.num_rows(); ++i) {
      VLOG(1) << "Found variable " << res[i][0];
      vars->push_back(string(res[i][0]));
    }
  }

  void _record(const string &variable, const string &host, const string &instance, Timestamp timestamp, double value) {
    uint64_t tsid = get_ts_id(variable);
    if (!tsid)
      tsid = CreateVariable(variable);
    if (!tsid) {
      LOG(ERROR) << "Could not find id for variable " << variable;
      return;
    }
    mysqlpp::Query query = conn_.query();
    Timestamp now;
    query << "INSERT INTO `values` (variable, `when`, value";
    if (host.size())
      query << ", hostname";
    if (instance.size())
      query << ", instance";
    query << ") VALUES (" << tsid << ", " << timestamp.ms()<< ", " << fixed << value;
    if (host.size())
      query << ", " << mysqlpp::quote << host;
    if (instance.size())
      query << ", " << mysqlpp::quote << instance;
    query << ")";
    mysqlpp::SimpleResult res = query.execute();
  }

  void _get_latest(const string &variable, Value *value) {
    uint64_t tsid = get_ts_id(variable);
    if (!tsid)
      return;
    mysqlpp::Query query = conn_.query();
    Timestamp now;
    query << "SELECT hostname, instance, `when`, value FROM `values` WHERE variable = " << mysqlpp::quote << tsid
          << " ORDER BY `when` DESC LIMIT 1";
    mysqlpp::StoreQueryResult res = query.store();
    if (res.num_rows() != 1)
      return;
    string host(res[0][0]);
    if (res[0][0] == mysqlpp::null)
      host.clear();
    string instance(res[0][1]);
    if (res[0][1] == mysqlpp::null)
      instance.clear();
    *value = Value(variable, host, instance, res[0][2], res[0][3]);
  }

  void _get_range(const string &variable, const Timestamp &start, const Timestamp &end, proto::ValueStream *stream) {
    uint64_t tsid = get_ts_id(variable);
    if (!tsid)
      throw runtime_error(StringPrintf("No stream found for variable \"%s\"", variable.c_str()));
    mysqlpp::Query query = conn_.query();
    query << "SELECT hostname, instance, `when`, value FROM `values` WHERE variable = " << tsid << " AND `when` >= "
          << start.ms() << " AND `when` <= " << end.ms() << " ORDER BY `when` DESC";
    mysqlpp::StoreQueryResult res = query.store();
    for (size_t i = 0; i < res.num_rows(); ++i) {
      VLOG(1) << "Found value " << res[i][1] << "@" << res[i][0];
      string host(res[i][0]);
      if (res[i][0] == mysqlpp::null)
        host.clear();
      string instance(res[i][1]);
      if (res[i][1] == mysqlpp::null)
        instance.clear();
      stream->set_host(host);
      stream->set_variable(variable);
      stream->set_instance(instance);
      proto::Value *value = stream->add_value();
      value->set_timestamp(res[i][2]);
      value->set_value(res[i][3]);
    }
  }

  mysqlpp::Connection conn_;
  string database_;
  string host_;
  string username_;
  string password_;

  unordered_map<string, uint64_t> tsids_;
};

}  // namespace openinstrument
