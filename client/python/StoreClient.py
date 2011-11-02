#!/usr/bin/python

"""Client library for the OpenInstrument datastore."""

__author__ = 'david@dparrish.com (David Parrish)'

import base64
import re
import shlex
import sys
import urllib
import urllib2
import urlparse
import openinstrument_pb2 as proto

def StringVariable(variable):
  str = variable.name
  return str


def SetVariable(variable, str):
  matches = re.match(r"([^{]+)(?:{(.*)})?", str)
  if not matches:
    raise ValueError("Invalid variable %s" % str)
  variable.name = matches.group(1)
  if matches.group(2):
    splitter = shlex.shlex(matches.group(2), posix=True)
    splitter.whitespace += ','
    splitter.whitespace_split = True
    for label in list(splitter):
      vlabel = variable.label.add()
      vlabel.name, vlabel.value = label.split("=", 1)


def NewVariable(str):
  variable = proto.Variable()
  SetVariable(variable, str)
  return variable


def CopyVariable(from_var, to_var):
  to_var.name = from_var.name
  for label in from_var.label:
    vlabel = to_var.label.add()
    vlabel.name = label.name
    vlabel.value = label.value


class StoreClient:
  address = None
  def __init__(self, address, port=None):
    if address.startswith("http://"):
      address.rep("http://", "")
    if port is not None:
      # Address and port specified separately
      self.address = "http://%s:%d" % (address, port)
    else:
      # Address contains host:port
      self.address = "http://%s" % address

  def SendRequest(self, path, request, response):
    url = urlparse.urlparse("%s%s" % (self.address, path))
    req = urllib2.Request(url=url.geturl(),
                          data=base64.b64encode(request.SerializeToString()))
    req.add_header('Content-Type', 'application/base64')
    f = urllib2.urlopen(req)
    response.ParseFromString(base64.b64decode(f.read()))

  def Add(self, req):
    response = proto.AddResponse()
    self.SendRequest("/add", req, response)
    return response

  def List(self, req):
    response = proto.ListResponse()
    self.SendRequest("/list", req, response)
    return response

  def Get(self, req):
    response = proto.GetResponse()
    self.SendRequest("/get", req, response)
    return response


def main(argv):
  pass


if __name__ == '__main__':
  sys.exit(main(sys.argv))

