#!/usr/bin/python

__author__ = 'david@dparrish.com (David Parrish)'

import sys
import openinstrument_pb2 as proto
import StoreClient

def main(argv):
  listrequest = proto.ListRequest()
  listrequest.prefix = "/network/interface/stats/ifInUcastPkts"
  listrequest.max_variables = 5

  client = StoreClient.StoreClient("localhost", 8020)
  response = client.List(listrequest)
  print response

  getrequest = proto.GetRequest()
  getrequest.variable = "/network/interface/stats/ifInUcastPkts{hostname=gw1,interface=Dialer0}"
  getrequest.max_variables = 1
  mutation = getrequest.mutation.add()
  mutation.sample_type = proto.StreamMutation.RATE

  response = client.Get(getrequest)
  print response

if __name__ == '__main__':
  sys.exit(main(sys.argv))

