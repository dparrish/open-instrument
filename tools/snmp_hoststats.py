#!/usr/bin/python

__author__ = 'david@dparrish.com (David Parrish)'

# Add the OpenInstrument python client path to sys.path
import os
import sys
sys.path.insert(0, os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../client/python"))

import ConfigParser
import platform
import stat
import struct
import time
import pprint

from pysnmp.entity.rfc3413.oneliner import cmdgen
from pysnmp.smi import builder, view
from pysnmp.proto.rfc1902 import *
import StoreClient
import openinstrument_pb2 as proto

mibBuilder = builder.MibBuilder().loadModules(
    'SNMPv2-MIB', 'IF-MIB', 'HOST-RESOURCES-MIB', 'HOST-RESOURCES-TYPES', 'TCP-MIB', 'UDP-MIB')
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
      print "Reloading configuration file %s" % self.filename
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
        authData, cmdgen.UdpTransportTarget((self.ip, self.port)), 0, 25000, prefix)

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

  def SnmpTable(self, oid, title_field):
    results = self.BulkGet(oid)
    index = {}
    for row in results:
      if row[0][-2] == title_field:
        index[row[0][-1]] = str(row[1])
    table = {}
    for result in results:
      oid, value = result[0], result[1]
      if len(oid) < 2:
        continue
      try:
        rowname = index[oid[-1]]
      except:
        rowname = oid[-1]
      item = oid[-2]
      if not rowname in table:
        table[rowname] = {'index': int(oid[-1])}
      table[rowname][item] = value
    return table

  def SnmpWalk(self, oid):
    results = self.BulkGet(oid)
    return results

  def PivotTable(self, table, field):
    results = {}
    for key, row in table.items():
      if not field in row:
        continue
      results[row[field]] = row
    return results

  def FormatMacAddress(self, value):
    try:
      octets = struct.unpack('BBBBBB', value)
    except struct.error:
      return None
    return "%02x:%02x:%02x:%02x:%02x:%02x" % octets

  def SetVarType(self, var, value):
    try:
      if isinstance(value, Counter32):
        var.SetLabel("datatype", "counter")
        return int(value)
      elif isinstance(value, Counter64):
        var.SetLabel("datatype", "counter")
        return int(value)
      elif isinstance(value, Gauge32):
        var.SetLabel("datatype", "gauge")
        return int(value)
      elif isinstance(value, Integer):
        var.SetLabel("datatype", "integer")
        return int(value)
      elif isinstance(value, TimeTicks):
        var.SetLabel("datatype", "integer")
        var.SetLabel("units", "ticks")
        return int(value)
      elif isinstance(value, OctetString):
        var.SetLabel("datatype", "string")
        return str(value)
    except ValueError, TypeError:
      return value

  def CollectInterfaceStats(self, addrequest):
    if self.config.Get(self.host, "poll_interfaces") != "1":
      return
    stats_to_keep = {
        # MIB field name: High speed field name
        'ifAdminStatus': None,
        'ifDescr': None,
        'ifInDiscards': None,
        'ifInErrors': None,
        'ifInNUcastPkts': 'ifHCInBroadcastPkts',
        'ifInOctets': 'ifHCInOctets',
        'ifInUcastPkts': 'ifHCInUcastPkts',
        'ifMtu': None,
        'ifOperStatus': None,
        'ifOutDiscards': None,
        'ifOutErrors': None,
        'ifOutNUcastPkts': 'ifHCOutBroadcastPkts',
        'ifOutOctets': 'ifHCOutOctets',
        'ifOutQLen': None,
        'ifOutUcastPkts': 'ifHCOutUcastPkts',
        'ifSpeed': None,
        'ifType': None,
        'ifPhysAddress': None,
    }
    time_ms = int(time.time() * 1000)
    table = self.SnmpTable((1,3,6,1,2,1,2,2), "ifDescr")  # RFC1213-MIB::ifTable
    hctable = self.PivotTable(self.SnmpTable((1,3,6,1,2,1,31,1,1), "ifName"), "index")

    for interface, values in table.items():
      try:
        ifindex = int(values['ifIndex'])
      except:
        continue
      for stat, hcstat in stats_to_keep.items():
        try:
          value = values[stat]
        except:
          continue
        try:
          value = hctable[ifindex][hcstat]
        except:
          pass
        var = StoreClient.Variable("/network/interface/stats/%s" % stat)
        var.SetLabel("hostname", self.host)
        var.SetLabel("srchost", platform.node())
        var.SetLabel("interface", interface)
        value = self.SetVarType(var, value)
        if stat == 'ifPhysAddress':
          value = self.FormatMacAddress(value)
        if value is None:
          continue
        stream = addrequest.stream.add()
        var.ToProtobuf(stream.variable)
        val = stream.value.add()
        val.timestamp = time_ms
        try:
          if var.GetLabel('datatype') == 'string':
            val.string_value = value
          else:
            val.double_value = float(value)
        except AttributeError:
          continue

  def OIDToString(self, oid):
    return ".".join(map(str, oid))

  def AddValue(self, addrequest, variable, value, time_ms):
    var = StoreClient.Variable(variable)
    stream = addrequest.stream.add()
    var.ToProtobuf(stream.variable)
    val = stream.value.add()
    val.timestamp = time_ms
    try:
      if var.GetLabel('datatype') == 'string':
        val.string_value = value
      else:
        val.double_value = float(value)
    except AttributeError:
      pass

  def CollectFilesystemStats(self, addrequest):
    if self.config.Get(self.host, "poll_filesystems") != "1":
      return
    time_ms = int(time.time() * 1000)
    table = self.SnmpTable((1,3,6,1,2,1,25,2,3), "hrStorageDescr")  # HOST-RESOURCES-MIB::hrStorageTable
    for filesystem, values in table.items():
      if self.OIDToString(values['hrStorageType']) != "1.3.6.1.2.1.25.2.1.4":  # hrStorageFixedDisk
        continue
      block_size = int(values['hrStorageAllocationUnits'])
      self.AddValue(addrequest,
          "/system/filesystem/size{hostname=%s, srchost=%s, device=%s}" % (self.host, platform.node(), filesystem),
          int(values['hrStorageSize']) * block_size,
          time_ms)
      self.AddValue(addrequest,
          "/system/filesystem/used{hostname=%s, srchost=%s, device=%s}" % (self.host, platform.node(), filesystem),
          int(values['hrStorageUsed']) * block_size,
          time_ms)
      self.AddValue(addrequest,
          "/system/filesystem/available{hostname=%s, srchost=%s, device=%s}" % (self.host, platform.node(), filesystem),
          (int(values['hrStorageSize']) - int(values['hrStorageUsed'])) * block_size,
          time_ms)


  def CollectSocketStats(self, addrequest):
    if self.config.Get(self.host, "poll_socket_stats") != "1":
      return

  def CollectSystemStats(self, addrequest):
    if self.config.Get(self.host, "poll_system_stats") != "1":
      return
    time_ms = int(time.time() * 1000)
    stats = {}
    for stat, value in self.SnmpWalk((1,3,6,1,2,1,25,1)):
      key = self.OIDToString(stat).replace("iso.org.dod.internet.mgmt.mib-2.host.hrSystem.", "")
      stats[key] = value
    try:
      self.AddValue(addrequest, "/system/uptime{hostname=%s, srchost=%s, datatype=gauge}" % (self.host, platform.node()),
          int(stats['hrSystemUptime.0'] / 100.0), time_ms)
    except KeyError:
      pass
    try:
      self.AddValue(addrequest, "/system/boot/kernel-commandline{hostname=%s, srchost=%s, datatype=string}" % (self.host, platform.node()),
          str(stats['hrSystemInitialLoadParameters.0']), time_ms)
    except KeyError:
      pass
    try:
      self.AddValue(addrequest, "/system/num_users/{hostname=%s, srchost=%s, datatype=gauge}" % (self.host, platform.node()),
          int(stats['hrSystemNumUsers.0']), time_ms)
    except KeyError:
      pass
    try:
      self.AddValue(addrequest, "/system/num_processes/{hostname=%s, srchost=%s, datatype=gauge}" % (self.host, platform.node()),
          int(stats['hrSystemProcesses.0']), time_ms)
    except KeyError:
      pass

    table = self.SnmpTable((1,3,6,1,2,1,25,2,3), "hrStorageDescr")  # HOST-RESOURCES-MIB::hrStorageTable
    for filesystem, values in table.items():
      if self.OIDToString(values['hrStorageType']) in ['1.3.6.1.2.1.25.2.1.2', '1.3.6.1.2.1.25.2.1.3']:
        block_size = int(values['hrStorageAllocationUnits'])
        args = "hostname=%s, srchost=%s, datatype=gauge, space=\"%s\"" % (self.host, platform.node(),
                                                                          str(values['hrStorageDescr']))
        self.AddValue(addrequest, "/system/ram/size{%s}" % args, int(values['hrStorageSize']) * block_size, time_ms)
        self.AddValue(addrequest, "/system/ram/used{%s}" % args, int(values['hrStorageUsed']) * block_size, time_ms)
        self.AddValue(addrequest, "/system/ram/available{%s}" % args,
                      (int(values['hrStorageSize']) - int(values['hrStorageUsed'])) * block_size, time_ms)


  def Run(self):
    if self.config.Get(self.host, "skip") == "1":
      return
    poll_start_time = time.time()
    print "Polling %s at %s with community %s, storing results at %s" % (
        self.host, self.ip, self.community, self.config.Get(self.host, "store"))
    addrequest = proto.AddRequest()

    self.CollectSystemStats(addrequest)
    self.CollectInterfaceStats(addrequest)
    self.CollectFilesystemStats(addrequest)
    self.CollectSocketStats(addrequest)

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
    last_poll[host] = 0

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


