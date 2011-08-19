/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/mime_types.h"
#include "lib/trie.h"

namespace openinstrument {

const string &MimeTypes::Lookup(const string &filename) {
  static const string unknown("application/x-binary");
  string extension(filename);
  size_t pos = filename.find_last_of(".");
  if (pos != string::npos)
    extension = filename.substr(pos + 1);
  Trie<string>::iterator it = types_.find(extension.c_str());
  if (it == types_.end())
    return unknown;
  return *it;
}

uint32_t MimeTypes::ReadFile(const string &filename) {
  FILE *fh = fopen(filename.c_str(), "r");
  if (!fh) {
    LOG(ERROR) << "Could not read mime types file " << filename << ": " << strerror(errno);
    return 0;
  }
  uint32_t num_types = 0;
  scoped_array<char> buf(new char[4096]);
  while (fgets(buf.get(), 4096, fh)) {
    char *p;
    for (p = buf.get(); p && *p; p++) {
      if (*p == '#' || *p == '\r' || *p == '\n') {
        *p = 0;
        break;
      }
    }

    vector<string> parts;
    string foo(buf.get());
    boost::split(parts, foo, boost::algorithm::is_any_of(" \t"), boost::token_compress_on);
    if (parts.size() < 2)
      continue;
    string mimetype = parts[0];
    for (vector<string>::iterator i = parts.begin() + 1; i != parts.end(); ++i) {
      types_.insert(*i, mimetype);
      num_types++;
    }
  }
  fclose(fh);
  VLOG(1) << "Loaded " << num_types << " mime types from " << filename;
  return num_types;
}

}  // namespace openinstrument
