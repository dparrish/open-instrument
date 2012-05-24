#!/usr/bin/python

"""Client library for the OpenInstrument datastore."""

__author__ = 'david@dparrish.com (David Parrish)'

import base64
import shlex
import sys
import urllib
import urllib2
import urlparse
import openinstrument_pb2 as proto


class StoreClient(object):
  address = None
  def __init__(self, address, port=None):
    if address.startswith("http://"):
      address.replace("http://", "")
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


class Variable(object):
  labels = {}
  var = ""
  def __init__(self, var):
    if isinstance(var, proto.StreamVariable):
      self.FromProtobuf(var)
    else:
      self.ParseString(var.strip())

  def __str__(self):
    out = self.var
    if len(self.labels):
      labels = []
      for key, value in self.labels.items():
        labels.append('%s="%s"' % (key, value))
      out += "{%s}" % (",".join(labels))
    return out

  def FromProtobuf(self, proto):
    self.var = proto.name
    self.labels = {}
    for i in proto.label:
      self.SetLabel(i.label, i.value)

  def ToProtobuf(self, proto):
    proto.name = self.var
    for key, value in self.labels.items():
      label = proto.label.add()
      label.label = key
      label.value = value

  def ParseString(self, var):
    index = var.find("{")
    if index < 0:
      self.var = var
      return
    self.var = var[0:index]
    splitter = shlex.shlex(var[index + 1:-1], posix=True)
    splitter.shitespace = ","
    splitter.whitespace_split = True
    for label in splitter:
      try:
        (key, value) = label.split("=", 1)
        value = value.strip("\"',");
        self.SetLabel(key, value)
      except:
        continue

  def SetLabel(self, label, value):
    self.labels[label] = value
    return self

  def GetLabel(self, label):
    return self.labels[label]

  def ClearLabel(self, label):
    del self.labels[label]


def main(argv):
  pass


if __name__ == '__main__':
  sys.exit(main(sys.argv))

