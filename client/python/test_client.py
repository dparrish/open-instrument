#!/usr/bin/python

__author__ = 'david@dparrish.com (David Parrish)'

import sys
import time
import openinstrument_pb2 as proto
import StoreClient

def main(argv):
  # Request the names of 5 variables
  listrequest = proto.ListRequest()
  StoreClient.SetVariable(listrequest.prefix, "/openinstrument/*")
  listrequest.max_variables = 5

  client = StoreClient.StoreClient("localhost", 8020)
  response = client.List(listrequest)
  print response

  # Get the last hour for the first variable returned
  getrequest = proto.GetRequest()
  StoreClient.CopyVariable(response.stream[0].variable, getrequest.variable)
  getrequest.min_timestamp = int((time.time() - 3600)) * 1000;
  getrequest.max_variables = 1
  mutation = getrequest.mutation.add()
  mutation.sample_type = proto.StreamMutation.RATE

  response = client.Get(getrequest)
  print response

if __name__ == '__main__':
  sys.exit(main(sys.argv))

