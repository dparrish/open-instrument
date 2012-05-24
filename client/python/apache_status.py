#!/usr/bin/python

__author__ = 'david@dparrish.com (David Parrish)'

import optparse
import os
import re
import socket
import StoreClient
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


def GetServerStatus(addrequest, options):
  timestamp = int(time.time() * 1000)
  worker_counts = {
      'waiting': 0,
      'starting': 0,
      'reading': 0,
      'writing': 0,
      'keepalive': 0,
      'dns': 0,
      'closing': 0,
      'logging': 0,
      'lameduck': 0,
      'idle_cleanup': 0,
      'open': 0,
  }
  worker_types = {
      '_': 'waiting',
      'S': 'starting',
      'R': 'reading',
      'W': 'writing',
      'K': 'keepalive',
      'D': 'dns',
      'C': 'closing',
      'L': 'logging',
      'G': 'lameduck',
      'I': 'idle_cleanup',
      '.': 'open',
  }

  url = urllib2.urlopen("http://%s:%s/server-status?auto" % (options.host, options.port))
  for line in url.readlines():
    key, value = line.split(': ')
    if key == "Scoreboard":
      for worker in value:
        if not worker in worker_types:
          continue
        worker_counts[worker_types[worker]] += 1
    elif key == "Total Accesses":
      AddVar(addrequest, "/webserver/apache/hits", value)
    elif key == "Total kBytes":
      AddVar(addrequest, "/webserver/apache/bytes", float(value) * 1024.0, labels={'units': 'bytes'})
    elif key == "CPULoad":
      AddVar(addrequest, "/webserver/apache/cpu_load", value)
    elif key == "Uptime":
      AddVar(addrequest, "/webserver/apache/uptime", value)
    elif key == "ReqPerSec":
      AddVar(addrequest, "/webserver/apache/req_per_sec", value)
    elif key == "BytesPerSec":
      AddVar(addrequest, "/webserver/apache/bytes_per_sec", value)
    elif key == "BytesPerReq":
      AddVar(addrequest, "/webserver/apache/bytes_per_req", value)
    elif key == "BusyWorkers" or key == "BusyServers":
      AddVar(addrequest, "/webserver/apache/busy_workers", value)
    elif key == "IdleWorkers" or key == "IdleServers":
      AddVar(addrequest, "/webserver/apache/idle_workers", value)

  for type, count in worker_counts.items():
    AddVar(addrequest, "/webserver/apache/worker_slots/%s" % type, count)


def GetApcStatus(addrequest, options):
  headers = None
  if options.apc_host_header:
    headers = { 'Host':  options.apc_host_header }
  try:
    url = "http://%s:%s/%s?auto" % (options.apc_host, options.apc_port, options.apc_url)
    content = urllib2.urlopen(url, "", headers).read()
    for match in re.findall(r"\w+: [\d\.]+", content):
      key, value = match.split(': ')
      if not key or not value:
        continue
      AddVar(addrequest, "/webserver/apache/apc/%s" % key, value)

  except urllib2.HTTPError:
    return


def main(argv):
  parser = optparse.OptionParser(usage="usage: %prog <store hostname:port> args...", add_help_option=False)
  parser.add_option("-h", dest="host", help="Apache host", default="localhost");
  parser.add_option("-p", dest="port", help="Apache port", default="80");
  parser.add_option("-H", dest="apc_host", help="Apc host", default="localhost");
  parser.add_option("-P", dest="apc_port", help="Apc port", default="80");
  parser.add_option("-x", dest="apc_host_header", help="Apc Host header");
  parser.add_option("-u", dest="apc_url", help="Apc URL", default="apc_info.php");
  (options, args) = parser.parse_args(args=argv)

  if len(args) != 2:
    parser.error("Required argument: datastore")
  _, dest = args
  if not dest:
    parser.error("Required argument: datastore")

  addrequest = proto.AddRequest()
  GetServerStatus(addrequest, options)
  GetApcStatus(addrequest, options)

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

