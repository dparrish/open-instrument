/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <vector>
#include <set>
#include <string>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/string.h"
#include "lib/timer.h"
#include "server/datastore.h"

DECLARE_string(datastore);

namespace openinstrument {

IndexedStoreFile::IndexedStoreFile(const string &filename) : filename(filename), reader(filename) {
  reader.Next(&header);
  if (!header.start_timestamp() || !header.end_timestamp() || header.end_timestamp() <= header.start_timestamp()) {
    LOG(ERROR) << "Could not read header from datastore file " << filename;
  } else {
    VLOG(2) << "Opened datastore file " << filename << " that has " << header.index_size() << " items";
  }
}

vector<proto::ValueStream> IndexedStoreFile::GetVariable(const Variable &variable) {
  vector<proto::ValueStream> results;
  GetVariable(variable, &results);
  return results;
}

bool IndexedStoreFile::GetVariable(const Variable &variable, vector<proto::ValueStream> *results) {
  for (auto &index : header.index()) {
    Variable index_var(index.variable());
    if (index_var.Matches(variable)) {
      VLOG(3) << "Seeking to " << index.offset();
      reader.fh()->SeekAbs(index.offset());
      proto::ValueStream stream;
      if (!reader.Next(&stream)) {
        LOG(WARNING) << "EOF reading datastore file " << filename << " at " << index.offset();
        break;
      }
      Variable stream_var(stream.variable());
      if (!stream_var.Matches(index_var)) {
        LOG(WARNING) << "Variable at " << index.offset() << " in " << filename << " does not match header";
        continue;
      }
      VLOG(3) << "Found item " << index_var.ToString() << " in " << filename;
      results->push_back(stream);
    }
  }
  return results->size() > 0;
}

}  // namespace openinstrument
