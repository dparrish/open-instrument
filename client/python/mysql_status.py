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
import openinstrument_pb2 as proto


def main(argv):
  parser = optparse.OptionParser(usage="usage: %prog <store hostname:port> args...", add_help_option=False)
  parser.add_option("-p", dest="password", help="MySQL password");
  parser.add_option("-u", dest="username", help="MySQL username");
  parser.add_option("-h", dest="host", help="MySQL host");
  parser.add_option("-P", dest="port", help="MySQL port");
  parser.add_option("--defaults-file", dest="defaults_file", help="MySQL defaults file");
  (options, args) = parser.parse_args(args=argv)

  if len(args) != 2:
    parser.error("Required argument: datastore")
  _, dest = args
  if not dest:
    parser.error("Required argument: datastore")

  cmdline = [ "/usr/bin/mysql" ]
  if options.username:
    cmdline.extend(["-u", options.username])
  if options.password:
    cmdline.append("-p%s" % options.password)
  if options.host:
    cmdline.extend(["-h", options.host])
  if options.port:
    cmdline.extend(["-P", options.port])
  if options.defaults_file:
    cmdline.extend("--defaults-file=%s" % options.defaults_file)
  cmdline.extend(["-e", "show status"])

  output = subprocess.Popen(cmdline, stdout=subprocess.PIPE).communicate()[0]
  if not output:
    print "No output from MySQL"
    return 1

  timestamp = int(time.time() * 1000)
  addrequest = proto.AddRequest()
  for line in output.split("\n"):
    try:
      key, value = re.split(r"\s+", line, 2)
      key = key.lower()
    except ValueError as e:
      continue
    floatvalue = 0.0
    try:
      if value == "OFF":
        value = 0
      if value == "ON":
        value = 1
      floatvalue = float(value)
    except ValueError as e:
      continue
    var = StoreClient.Variable("/database/mysql/status/" + key)
    var.SetLabel("hostname", options.host or socket.gethostname())
    if options.port:
      var.SetLabel("port", options.port)
    stream = addrequest.stream.add()
    var.ToProtobuf(stream.variable)
    value = stream.value.add()
    value.double_value = floatvalue
    value.timestamp = timestamp

  # Send the config to the data store
  hostname, port = dest.split(":")
  port = int(port)
  client = StoreClient.StoreClient(hostname, port)
  response = client.Add(addrequest)
  if not response.success:
    print "Error sending status to datastore: ", response
    return 1

if __name__ == '__main__':
  sys.exit(main(sys.argv))

