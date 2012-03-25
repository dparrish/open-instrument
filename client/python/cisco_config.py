#!/usr/bin/python

"""A one line summary of the module or script, terminated by a period.

Leave one blank line.  The rest of this doc string should contain an
overall description of the module or script.  Optionally, it may also
contain a brief description of exported classes and functions.

  ClassFoo: One line summary.
  FunctionBar(): One line summary.
"""

__author__ = 'david@dparrish.com (David Parrish)'

import optparse
import os
import re
import sys
import tempfile
import time
import StoreClient
import openinstrument_pb2 as proto


def telnet_call(host, user, password, command):
  import telnetlib
  telnet = telnetlib.Telnet(host)
  telnet.read_until('sername:', 3)
  telnet.write(user + '\r')
  telnet.read_until("assword:", 3)
  telnet.write(password + '\r')
  telnet.write("term length 0\r")
  telnet.write(command + "\r")
  telnet.write("exit\r")
  lines = telnet.read_all().split("\r\n")
  try:
    while re.match(r"^ *$", lines[-1]) or lines[-1].endswith("#exit"):
      lines.pop()
    while re.match(r"^ *$", lines[0]) or re.search(r"(#term length 0|#%s|^Building configuration\.\.\.)$" % command, lines[0]):
      lines.pop(0)
  except IndexError, e:
    print e
  return lines


def main(argv):
  parser = optparse.OptionParser(usage="usage: %prog <store hostname:port> args...")
  parser.add_option("-T", dest="telnet", action="store_true", help="Use telnet to retrieve the config")
  parser.add_option("-S", dest="snmp", action="store_true", help="Use SNMP to retrieve the config")

  (options, args) = parser.parse_args(args=argv)

  config = ""
  host = ""
  dest = ""
  if options.telnet:
    _, dest, host, user, password = args
    if not host or not user or not password:
      parser.error("Required arguments: datastore host username password")
    config = telnet_call(host, user, password, "show running-config")
    if not config or not len(config) > 5:
      return 1
  elif options.snmp:
    _, dest, host, community, tftp_server, tftp_path = args
    if not host or not community or not tftp_server or not tftp_path:
      parser.error("Required options: datastore host community ttfp_server local_tftp_path")
    if not os.path.exists(tftp_path):
      parser.error("tftp path %s must exist" % tftp_path)

    from pysnmp.entity.rfc3413.oneliner import cmdgen
    from pysnmp.proto import rfc1902

    filename = tempfile.mktemp(dir=tftp_path)
    errorIndication, errorStatus, errorIndex, varBinds = cmdgen.CommandGenerator().setCmd(
        cmdgen.CommunityData('my-agent', community),
        cmdgen.UdpTransportTarget((host, 161)),
        # Plain OID
        ((1,3,6,1,4,1,9,2,1,55) + tuple(map(int, tftp_server.split("."))), rfc1902.OctetString(os.path.basename(filename)))
    )
    if errorIndication:
      print errorIndication
      print errorStatus.prettyPrint()
      return 1

    config = ""
    for line in file(filename):
      config += line

    os.unlink(filename)
  else:
    print "You must specify either -T or -S"
    return 1

  if config:
    # Send the config to the data store
    addrequest = proto.AddRequest()
    var = StoreClient.Variable("/network/device/configuration{device_type=cisco}")
    var.SetLabel("hostname", host)
    stream = addrequest.stream.add()
    var.ToProtobuf(stream.variable)
    value = stream.value.add()
    value.string_value = "".join(config)
    value.timestamp = int(time.time() * 1000)
    hostname, port = dest.split(":")
    port = int(port)
    client = StoreClient.StoreClient(hostname, port)
    response = client.Add(addrequest)
  else:
    print "No config retrieved"

if __name__ == '__main__':
  sys.exit(main(sys.argv))

