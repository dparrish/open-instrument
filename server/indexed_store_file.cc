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
#include "server/indexed_store_file.h"

DECLARE_string(datastore);

namespace openinstrument {

IndexedStoreFile::IndexedStoreFile(const string &filename)
  : filename(filename),
    reader_(NULL),
    opened_(false) {
  Open();
}

bool IndexedStoreFile::Open() {
  if (opened_)
    return true;
  reader_.reset(new ProtoStreamReader(filename));
  reader_->Next(&header_);
  if (!header_.start_timestamp() || !header_.end_timestamp() || header_.end_timestamp() <= header_.start_timestamp()) {
    LOG(ERROR) << "Could not read header_ from datastore file " << filename;
    return false;
  }
  VLOG(2) << "Opened datastore file " << filename << " that has " << header_.index_size() << " items";
  opened_ = true;
  return true;
}

void IndexedStoreFile::Close() {
  if (reader_.get())
    reader_.reset(NULL);
  opened_ = false;
}

void IndexedStoreFile::Clear() {
  if (refcount() > 1) {
    LOG(ERROR) << "Attempt to Clear() a IndexedStoreFile with refcount of " << refcount();
    return;
  }
  Close();
  log_data_.erase(log_data_.begin(), log_data_.end());
  header_.Clear();
}

bool IndexedStoreFile::GetVariable(const Variable &variable, vector<proto::ValueStream> *results) {
  for (auto &index : header_.index()) {
    Variable index_var(index.variable());
    if (index_var.Matches(variable)) {
      VLOG(3) << "Seeking to " << index.offset();
      reader_->fh()->SeekAbs(index.offset());
      proto::ValueStream stream;
      if (!reader_->Next(&stream)) {
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
