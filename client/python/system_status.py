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
import urllib
import openinstrument_pb2 as proto


def HZ():
  return float(os.sysconf("SC_CLK_TCK"))


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


def GetCpuStats(addrequest):
  timestamp = int(time.time() * 1000)
  for line in file("/proc/stat"):
    key, value = re.split(r"\s+", line.strip(), 1)
    if key[0:3] == "cpu":
      labels = { 'datatype': 'counter', 'units': 'seconds' }
      if len(key) > 3:
        labels['cpu'] = key[3:]
      else:
        labels['cpu'] = 'total'
      parts = value.split(" ")
      if len(parts) == 7:
        user, nice, system, idle, iowait, irq, softirq = value.split(" ")
      else:
        user, nice, system, idle, iowait, irq, softirq, _ = value.split(" ", 7)
      AddVar(addrequest, "/system/stats/cpu_stats/user", float(user) / HZ(), labels, timestamp)
      AddVar(addrequest, "/system/stats/cpu_stats/system", float(system) / HZ(), labels, timestamp)
      AddVar(addrequest, "/system/stats/cpu_stats/nice", float(nice) / HZ(), labels, timestamp)
      AddVar(addrequest, "/system/stats/cpu_stats/idle", float(idle) / HZ(), labels, timestamp)
      AddVar(addrequest, "/system/stats/cpu_stats/iowait", float(iowait) / HZ(), labels, timestamp)
      AddVar(addrequest, "/system/stats/cpu_stats/irq", float(irq) / HZ(), labels, timestamp)
      AddVar(addrequest, "/system/stats/cpu_stats/softirq", float(softirq) / HZ(), labels, timestamp)
    elif key == "intr":
      interrupt_counters = value.split(" ")
      total_interrupts = int(interrupt_counters.pop(0))
      labels = { 'interrupt': 'total', 'datatype': 'counter' }
      AddVar(addrequest, "/system/stats/cpu_stats/interrupts", total_interrupts, labels, timestamp)
      #for i in xrange(len(interrupt_counters)):
        #labels = { 'interrupt': str(i), 'datatype': 'counter' }
        #AddVar(addrequest, "/system/stats/cpu_stats/interrupts", interrupt_counters[i], labels, timestamp)
    elif key == "ctxt":
      AddVar(addrequest, "/system/stats/cpu_stats/context_switches", value, { 'datatype': 'counter' }, timestamp)
    elif key == "btime":
      AddVar(addrequest, "/system/stats/uptime", time.time() - int(value), { 'datatype': 'gauge' }, timestamp)
    elif key == "processes":
      AddVar(addrequest, "/system/stats/processes_started", value, { 'datatype': 'counter' }, timestamp)
    elif key == "procs_blocked":
      AddVar(addrequest, "/system/stats/processes_blocked", value, { 'datatype': 'gauge' }, timestamp)
    elif key == "procs_running":
      AddVar(addrequest, "/system/stats/processes_running", value, { 'datatype': 'gauge' }, timestamp)
    elif key == "softirq":
      irq_counters = value.split(" ")
      total_irqs = int(irq_counters.pop(0))
      labels = { 'irq': 'total', 'datatype': 'counter' }
      AddVar(addrequest, "/system/stats/softirq", total_irqs, labels, timestamp)
      for i in xrange(len(irq_counters)):
        labels = { 'irq': str(i), 'datatype': 'counter' }
        AddVar(addrequest, "/system/stats/softirq", irq_counters[i], labels, timestamp)


def GetDfStats(addrequest):
  timestamp = int(time.time() * 1000)
  output = subprocess.Popen(["df", "-P", "-l"], stdout=subprocess.PIPE).communicate()[0]
  for line in output.split("\n"):
    try:
      filesystem, size, used, available, capacity, mountpoint = re.split(r"\s+", line.strip(), 5)
    except ValueError:
      continue
    try:
      labels = {
          'device': filesystem,
          'mountpoint': mountpoint,
          'datatype': 'gauge',
          'units': 'bytes',
      }
      AddVar(addrequest, "/system/filesystem/size", float(size) * 1024.0, labels, timestamp)
      AddVar(addrequest, "/system/filesystem/used", float(used) * 1024.0, labels, timestamp)
      AddVar(addrequest, "/system/filesystem/available", float(available) * 1024.0, labels, timestamp)
    except ValueError:
      continue

  timestamp = int(time.time() * 1000)
  output = subprocess.Popen(["df", "-P", "-l", "-i"], stdout=subprocess.PIPE).communicate()[0]
  for line in output.split("\n"):
    try:
      filesystem, inodes, used, available, capacity, mountpoint = re.split(r"\s+", line.strip(), 5)
      if int(inodes) == 0:
        continue
    except ValueError:
      continue
    try:
      labels = {
          'device': filesystem,
          'mountpoint': mountpoint,
          'datatype': 'gauge',
          'units': 'inodes',
      }
      AddVar(addrequest, "/system/filesystem/inodes_total", float(inodes), labels, timestamp)
      AddVar(addrequest, "/system/filesystem/inodes_used", float(used), labels, timestamp)
      AddVar(addrequest, "/system/filesystem/inodes_available", float(available), labels, timestamp)
    except ValueError:
      continue


def GetDiskStats(addrequest):
  timestamp = int(time.time() * 1000)
  for line in file("/proc/diskstats"):
    (major, minor, name, reads, reads_merged, sectors_read, ms_reading,
        writes, writes_merged, sectors_written, ms_writing,
        io_outstanding, ms_io, weighted_ms_io) = re.split(r"\s+", line.strip())
    if reads == '0' and writes == '0':
      continue
    labels = {
        'device': name,
        'datatype': 'counter',
    }
    AddVar(addrequest, "/system/disk_stats/reads", reads, labels, timestamp)
    AddVar(addrequest, "/system/disk_stats/reads_merged", reads_merged, labels, timestamp)
    AddVar(addrequest, "/system/disk_stats/writes", writes, labels, timestamp)
    AddVar(addrequest, "/system/disk_stats/writes_merged", writes_merged, labels, timestamp)
    AddVar(addrequest, "/system/disk_stats/sectors_read", sectors_read, labels, timestamp)
    AddVar(addrequest, "/system/disk_stats/sectors_written", sectors_written, labels, timestamp)
    AddVar(addrequest, "/system/disk_stats/ms_reading", ms_reading, labels, timestamp)
    AddVar(addrequest, "/system/disk_stats/ms_writing", ms_writing, labels, timestamp)


def GetLoadStats(addrequest):
  timestamp = int(time.time() * 1000)
  loadavg, _ = file("/proc/loadavg").read().split(' ', 1)
  AddVar(addrequest, "/system/load_average", loadavg, { 'datatype': 'gauge' }, timestamp)


def GetEntropyStats(addrequest):
  timestamp = int(time.time() * 1000)
  AddVar(addrequest, "/system/random/entropy_available", file("/proc/sys/kernel/random/entropy_avail").read().strip(), {
    'datatype': 'gauge'
    }, timestamp)


def GetInterfaceStats(addrequest):
  timestamp = int(time.time() * 1000)
  for line in file("/proc/net/dev"):
    if not re.match(r"^\s*\w+:.*", line.strip()):
      continue
    try:
      (iface, read_bytes, read_packets, read_errors, read_drop, read_fifo, read_frame, read_compressed, read_multicast,
          write_bytes, write_packets, write_errors, write_drop, write_fifo, write_collisions, write_carrier,
          write_compressed) = re.split(r"\s+", line.strip())
    except ValueError as e:
      print "Skipping line", line
      continue
    labels = { 'datatype': 'counter', 'interface': iface.strip(":") }
    AddVar(addrequest, "/network/interface/stats/read_bytes", read_bytes, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/read_packets", read_packets, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/read_errors", read_errors, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/read_drop", read_drop, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/read_frame", read_frame, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/read_compressed", read_compressed, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/read_multicast", read_multicast, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/write_bytes", write_bytes, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/write_packets", write_packets, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/write_errors", write_errors, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/write_drop", write_drop, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/write_collisions", write_collisions, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/write_compressed", write_compressed, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/write_carrier", write_carrier, labels, timestamp)
    labels.update({ 'datatype': 'gauge' })
    AddVar(addrequest, "/network/interface/stats/read_fifo", read_fifo, labels, timestamp)
    AddVar(addrequest, "/network/interface/stats/write_fifo", write_fifo, labels, timestamp)


def GetMemoryStats(addrequest):
  timestamp = int(time.time() * 1000)
  for line in file("/proc/meminfo"):
    try:
      key, value = re.split(r":\s+", line.strip())
      key = re.sub(r"[()]", "_", key.lower())
      matches = re.match(r"(\d+) (..)", value)
      if matches:
        if matches.group(2) == "kB":
          value = float(matches.group(1)) * 1024.0
        elif matches.group(2) == "MB":
          value = float(matches.group(1)) * 1024.0 * 1024.0
        else:
          value = float(matches.group(1))
      AddVar(addrequest, "/system/memory/" + key, value, { 'datatype': 'gauge' }, timestamp)
    except ValueError:
      continue


def GetNtpStats(addrequest):
  timestamp = int(time.time() * 1000)
  output = subprocess.Popen(["ntpq", "-n", "-p"], stdout=subprocess.PIPE).communicate()[0]
  for line in output.split("\n"):
    if not line:
      continue
    if line[0] != '*':
      continue
    line = line[1:]
    remote, refid, st, t, when, poll, reach, delay, offset, jitter = re.split(r"\s+", line)
    labels = { 'datatype': 'gauge', 'units': 'seconds', 'remote': remote }
    AddVar(addrequest, "/ntp/delay", float(delay) / 1000.0, labels, timestamp)
    AddVar(addrequest, "/ntp/offset", float(offset) / 1000.0, labels, timestamp)
    AddVar(addrequest, "/ntp/jitter", float(jitter) / 1000.0, labels, timestamp)


def GetVmStats(addrequest):
  timestamp = int(time.time() * 1000)
  for line in file("/proc/vmstat"):
    key, value = line.strip().split(" ")
    AddVar(addrequest, "/system/vmstat/" + key, value, {}, timestamp)


def main(argv):
  parser = optparse.OptionParser(usage="usage: %prog <store hostname:port> args...", add_help_option=False)
  (options, args) = parser.parse_args(args=argv)

  if len(args) != 2:
    parser.error("Required argument: datastore")
  _, dest = args
  if not dest:
    parser.error("Required argument: datastore")

  addrequest = proto.AddRequest()
  GetCpuStats(addrequest)
  GetDfStats(addrequest)
  GetDiskStats(addrequest)
  GetEntropyStats(addrequest)
  GetInterfaceStats(addrequest)
  GetLoadStats(addrequest)
  GetMemoryStats(addrequest)
  GetNtpStats(addrequest)
  GetVmStats(addrequest)

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

