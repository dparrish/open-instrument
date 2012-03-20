#!/usr/bin/python

__author__ = 'david@dparrish.com (David Parrish)'

# Add the OpenInstrument python client path to sys.path
import os
import sys
sys.path.insert(0, os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../client/python"))

import time
import stat
import ConfigParser
import platform

from pysnmp.entity.rfc3413.oneliner import cmdgen
from pysnmp.smi import builder, view
from pysnmp.proto.rfc1902 import *
import StoreClient
import openinstrument_pb2 as proto

mibBuilder = builder.MibBuilder().loadModules('SNMPv2-MIB', 'IF-MIB')
mibViewController = view.MibViewController(mibBuilder)


class AutoRestart(object):
  def __init__(self, argv):
    stat = os.stat(argv[0])
    self.inode = stat.st_ino
    self.mtime = stat.st_mtime
    self.argv = argv

  def Check(self):
    counter = 0
    while True:
      try:
        stat = os.stat(self.argv[0])
        if self.inode != stat.st_ino or self.mtime != stat.st_mtime:
          # File is changed, re-execute
          print "Executable has changed, re-executing"
          time.sleep(1)
          os.execv(self.argv[0], self.argv)
        return
      except OSError, e:
        counter += 1
        if counter > 5:
          print "Can't get mtime or inode for %s: %s" % (self.argv[0], e)
          return
        time.sleep(1)


class PollerConfig(object):
  filename = None
  inode = None
  mtime = None

  def __init__(self, filename):
    self.filename = filename
    self.Reload()

  def Reload(self):
    stat = os.stat(self.filename)
    self.inode = stat.st_ino
    self.mtime = stat.st_mtime
    self.config = ConfigParser.RawConfigParser()
    self.config.read(self.filename)

  def Check(self):
    stat = os.stat(self.filename)
    if self.inode != stat.st_ino or self.mtime != stat.st_mtime:
      print "Reloading configuration file ", self.filename
      self.Reload()

  def Hosts(self):
    return self.config.sections()

  def Get(self, host, item, default=None):
    if self.config.has_option(host, item):
      return self.config.get(host, item)
    return default


class Poller(object):
  def __init__(self, config, host):
    self.host = host
    self.config = config
    self.ip = self.config.Get(host, "ip", self.host)
    self.port = int(self.config.Get(host, "port", 161))
    self.community = self.config.Get(host, "community")
    self.snmp_version = int(self.config.Get(host, "snmp_version", 2))

  def OidMatch(self, oid, prefix):
    try:
      for i in range(0, len(prefix)):
          if oid[i] != prefix[i]:
            return False
    except IndexError:
      return False
    return True

  def BulkGet(self, prefix):
    authData = None
    if self.snmp_version == 1:
      authData = cmdgen.CommunityData('test-agent', self.config.Get(self.host, "community"), 0)
    elif self.snmp_version == 2:
      authData = cmdgen.CommunityData('test-agent', self.config.Get(self.host, "community"))
    elif self.snmp_version == 3:
      authData = cmdgen.UsmUserData('test-user', self.config.Get(self.host, "username"),
                                    self.config.Get(self.host, "password"))

    errorIndication, errorStatus, errorIndex, varBindTable = cmdgen.CommandGenerator().bulkCmd(
        authData, cmdgen.UdpTransportTarget((self.ip, self.port)), 0, 25, prefix)

    results = []
    if errorIndication:
      if errorIndication == "requestTimedOut":
        print "SNMP timout polling %s (%s) with auth %s" % (self.host, self.ip, authData)
      else:
        print "SNMP error: %s" % errorIndication
    else:
      if errorStatus:
        print '%s at %s\n' % (errorStatus.prettyPrint(),
                              errorIndex and varBindTable[-1][int(errorIndex) - 1] or '?')
      else:
        for varBindTableRow in varBindTable:
          for name, val in varBindTableRow:
            oid, label, suffix = mibViewController.getNodeName(name)
            if self.OidMatch(name, prefix):
              fulloid = list(label)
              fulloid.extend([str(x) for x in suffix])
              results.append((tuple(fulloid), val))
    return results

  def GetTableIndex(self, table, title_field):
    index = {}
    for row in table:
      if row[0][-2] == title_field:
        index[row[0][-1]] = str(row[1])
    return index

  def Run(self):
    if self.config.Get(self.host, "skip") == "1":
      return
    poll_start_time = time.time()
    print "Polling %s at %s with community %s, storing results at %s" % (
        self.host, self.ip, self.community, self.config.Get(self.host, "store"))
    addrequest = proto.AddRequest()

    time_ms = int(time.time() * 1000)
    results = self.BulkGet((1,3,6,1,2,1,2,2))  # RFC1213-MIB::ifTable
    index = self.GetTableIndex(results, "ifDescr")

    for result in results:
      oid, value = result[0], result[1]
      var = StoreClient.Variable("/network/interface/stats/%s" % oid[-2])
      if oid[-2] in ("ifIndex", "ifLastChange", "ifType"):
        continue
      try:
        var.SetLabel("hostname", self.host)
        var.SetLabel("srchost", platform.node())
        var.SetLabel("interface", index[oid[-1]])
      except:
        continue
      try:
        if isinstance(value, Counter32):
          var.SetLabel("datatype", "counter")
          value = int(value)
        elif isinstance(value, Counter64):
          var.SetLabel("datatype", "counter")
          value = int(value)
        elif isinstance(value, Gauge32):
          var.SetLabel("datatype", "gauge")
          value = int(value)
        elif isinstance(value, Integer):
          var.SetLabel("datatype", "integer")
          value = int(value)
        elif isinstance(value, TimeTicks):
          var.SetLabel("datatype", "integer")
          var.SetLabel("units", "ticks");
          value = int(value)
        elif isinstance(value, OctetString):
          # TODO(dparrish): Add support for strings
          var.SetLabel("datatype", "string")
          value = str(value)
          continue
      except ValueError, TypeError:
        continue;

      # Skip any types we don't know about
      if not var.GetLabel("interface"):
        continue
      try:
        var.GetLabel("datatype")
      except:
        continue

      stream = addrequest.stream.add()
      var.ToProtobuf(stream.variable)
      val = stream.value.add()
      val.timestamp = time_ms
      try:
        val.double_value = float(value)
      except AttributeError:
        continue
      except ValueError, TypeError:
        val.string_value = value

    poll_end_time = time.time()

    try:
      if len(addrequest.stream) > 0:
        # Write the results to the data store
        start_time = time.time()
        client = StoreClient.StoreClient(self.config.Get(self.host, "store"))
        client.Add(addrequest)
        end_time = time.time()
        print "Finished polling %s. poll_time=%0.2f, store_time=%0.2f" % (self.host, poll_end_time - poll_start_time,
                                                                          end_time - start_time)
    except Exception, e:
      print "Error writing results for %s: %s" % (self.host, e)


def main(argv):
  autorestart = AutoRestart(argv)
  config = PollerConfig("/home/dparrish/src/openinstrument/tools/snmp_hoststats.cfg")
  last_poll = {}
  for host in config.Hosts():
    last_poll[host] = 0;

  while True:
    autorestart.Check()
    config.Check()
    for host in config.Hosts():
      if config.Get(host, "skip") == "1":
        continue
      time_since_last = time.time() - last_poll[host]
      if time_since_last >= int(config.Get(host, "interval")):
        poller = Poller(config=config, host=host)
        poller.Run()
        last_poll[host] = time.time()

    time.sleep(1)


if __name__ == '__main__':
  sys.exit(main(sys.argv))


