#!/usr/bin/python

__author__ = 'david@dparrish.com (David Parrish)'

import os, sys
sys.path.append('/home/open-instrument/client/python')

import optparse
import re
import socket
import StoreClient
import subprocess
import time
import openinstrument_pb2 as proto
import thread


class VarnishLog(object):
  def __init__(self, counters, instance):
    self.instance = instance
    self.thread = thread.start_new_thread(self.LogWatcher, ())
    self.counters = counters

  def LogWatcher(self):
    while True:
      try:
        cmdline = [ "/usr/bin/varnishlog", "-c", "-i", "TxStatus" ]
        if self.instance:
          cmdline.extend(["-n", self.instance])
        try:
          child = subprocess.Popen(cmdline, stdout=subprocess.PIPE)
        except OSError as e:
          print cmdline
          print e
          return False
        if not child or not child.pid:
          break
        for line in child.stdout:
          matches = re.match(r"\s*(\d+)\s+TxStatus\s+[a-z]\s(\d+)", line)
          if not matches:
            continue
          try:
            self.counters[matches.group(2)] = self.counters.get(matches.group(2), 0) + 1
          except Queue.Full:
            pass
        if child:
          os.kill(child.pid, 15)
      except:
        if child:
          os.kill(child.pid, 15)


def main(argv):
  parser = optparse.OptionParser(usage="usage: %prog <store hostname:port> args...", add_help_option=False)
  parser.add_option("-n", dest="instance", help="Varnish instance");
  (options, args) = parser.parse_args(args=argv)

  if len(args) != 2:
    parser.error("Required argument: datastore")
  _, dest = args
  if not dest:
    parser.error("Required argument: datastore")

  counters = {}
  logger = VarnishLog(counters, options.instance)
  interval = 60
  while True:
    time.sleep(interval)
    timestamp = int(time.time() * 1000)
    addrequest = proto.AddRequest()

    for code, counter in counters.items():
      print "%s: %d" % (code, counter)
      var = StoreClient.Variable("/varnish/response_code")
      var.SetLabel("code", code)
      floatvalue = 0.0
      try:
        floatvalue = float(counter)
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


if __name__ == '__main__':
  sys.exit(main(sys.argv))


