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
  parser.add_option("-n", dest="instance", help="Varnish instance");
  (options, args) = parser.parse_args(args=argv)

  if len(args) != 2:
    parser.error("Required argument: datastore")
  _, dest = args
  if not dest:
    parser.error("Required argument: datastore")

  cmdline = [ "/usr/bin/varnishstat", "-1" ]
  if options.instance:
    cmdline.extend(["-n", options.instance])

  output = subprocess.Popen(cmdline, stdout=subprocess.PIPE).communicate()[0]
  if not output:
    print "No output from varnishstat"
    return 1

  timestamp = int(time.time() * 1000)
  addrequest = proto.AddRequest()
  for line in output.split("\n"):
    try:
      key, value, rate, help = re.split(r"\s+", line, 3)
      key = key.lower()
    except ValueError as e:
      continue
    matches = re.match(r"vbe\.([^\(]+)\(([^\)]+)\).(.+)", key)
    var = None
    if matches:
      host, _, port = matches.group(2).split(",")
      var = StoreClient.Variable("/varnish/status/director/%s/%s" % (matches.group(1), matches.group(3)))
      var.SetLabel("backend", host)
      var.SetLabel("backend_port", port)
    else:
      key = key.replace(".", "_")
      var = StoreClient.Variable("/varnish/status/" + key)
    floatvalue = 0.0
    try:
      floatvalue = float(value)
    except ValueError as e:
      print str(e)
      continue

    var.SetLabel("hostname", socket.gethostname())
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

