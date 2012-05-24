#!/usr/bin/python

__author__ = 'david@dparrish.com (David Parrish)'

import optparse
import os
import re
import socket
import StoreClient
import subprocess
import sys
import time
import urllib2
import openinstrument_pb2 as proto


def AddVar(addrequest, variable, value, labels={}, timestamp=None):
  var = StoreClient.Variable(variable)
  for key, val in labels.items():
    var.SetLabel(key, val)
  if not "hostname" in labels:
    var.SetLabel("hostname", socket.gethostname())
  stream = addrequest.stream.add()
  var.ToProtobuf(stream.variable)
  val = stream.value.add()
  try:
    val.double_value = float(value)
  except ValueError as e:
    val.string_value = value
  if timestamp:
    val.timestamp = timestamp
  else:
    val.timestamp = int(time.time() * 1000)
  return stream


def GetEntropyStats(addrequest):
  timestamp = int(time.time() * 1000)
  AddVar(addrequest, "/system/random/entropy_available", file("/proc/sys/kernel/random/entropy_avail").read().strip(), {
    'datatype': 'gauge'
    }, timestamp)


def main(argv):
  parser = optparse.OptionParser(usage="usage: %prog <store hostname:port> args...", add_help_option=False)
  (options, args) = parser.parse_args(args=argv)

  if len(args) < 3:
    parser.error("Required argument: datastore <url>...")

  addrequest = proto.AddRequest()

  for url in args[2:]:
    start_time = time.time()
    try:
      req = urllib2.urlopen(url)
      req.read()
      status_code = 200
    except urllib2.HTTPError as e:
      status_code = e.code

    end_time = time.time()
    AddVar(addrequest, "/http_fetch/time", end_time - start_time,
        { 'datatype': 'gauge', 'units': 'seconds', 'url': url }, int(start_time))
    AddVar(addrequest, "/http_fetch/status", status_code,
        { 'datatype': 'gauge', 'units': 'seconds', 'url': url }, int(start_time))

  # Send the config to the data store
  hostname, port = args[1].split(":")
  port = int(port)
  client = StoreClient.StoreClient(hostname, port)
  response = client.Add(addrequest)
  if not response.success:
    print "Error sending status to datastore: ", response
    return 1

if __name__ == '__main__':
  sys.exit(main(sys.argv))

