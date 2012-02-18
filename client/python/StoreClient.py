#!/usr/bin/python

"""Client library for the OpenInstrument datastore."""

__author__ = 'david@dparrish.com (David Parrish)'

import base64
import sys
import urllib
import urllib2
import urlparse
import openinstrument_pb2 as proto


class StoreClient(object):
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


class Variable(object):
  def __init__(self, var):
    self.var = var
    self.labels = {}

  def __str__(self):
    out = self.var
    if len(self.labels):
      labels = []
      for key, value in self.labels.items():
        labels.append('%s="%s"' % (key, value))
      out += "{%s}" % (",".join(labels))
    return out

  def SetLabel(self, label, value):
    self.labels[label] = value

  def GetLabel(self, label):
    return self.labels[label]

  def ClearLabel(self, label):
    del self.labels[label]


def main(argv):
  pass


if __name__ == '__main__':
  sys.exit(main(sys.argv))

