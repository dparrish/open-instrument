#!/usr/bin/perl -w

use strict;
use Time::HiRes qw( gettimeofday );

my $host = $ARGV[0];
my $community = $ARGV[1];

exit(1) if (!$host || !$community);

my $store = "192.168.1.5:8020";
my $client = "/home/dparrish/src/openinstrument/client/cpp/add";
my $maxargs = 50;
chomp(my $localhost = `hostname`);

my %stats = (
  "IP-MIB::ipInReceives.0" => "ipInReceives",
  "IP-MIB::ipInHdrErrors.0" => "ipInHdrErrors",
  "IP-MIB::ipInAddrErrors.0" => "ipInAddrErrors",
  "IP-MIB::ipForwDatagrams.0" => "ipForwDatagrams",
  "IP-MIB::ipInUnknownProtos.0" => "ipInUnknownProtos",
  "IP-MIB::ipInDiscards.0" => "ipInDiscards",
  "IP-MIB::ipInDelivers.0" => "ipInDelivers",
  "IP-MIB::ipOutRequests.0" => "ipOutRequests",
  "IP-MIB::ipOutDiscards.0" => "ipOutDiscards",
  "IP-MIB::ipOutNoRoutes.0" => "ipOutNoRoutes",
  "IP-MIB::ipReasmReqds.0" => "ipReasmReqds",
  "IP-MIB::ipReasmOKs.0" => "ipReasmOKs",
  "IP-MIB::ipReasmFails.0" => "ipReasmFails",
  "IP-MIB::ipFragOKs.0" => "ipFragOKs",
  "IP-MIB::ipFragFails.0" => "ipFragFails",
  "IP-MIB::ipFragCreates.0" => "ipFragCreates",
  "IP-MIB::icmpInMsgs.0" => "icmpInMsgs",
  "IP-MIB::icmpInErrors.0" => "icmpInErrors",
  "IP-MIB::icmpInDestUnreachs.0" => "icmpInDestUnreachs",
  "IP-MIB::icmpInTimeExcds.0" => "icmpInTimeExcds",
  "IP-MIB::icmpInParmProbs.0" => "icmpInParmProbs",
  "IP-MIB::icmpInSrcQuenchs.0" => "icmpInSrcQuenchs",
  "IP-MIB::icmpInRedirects.0" => "icmpInRedirects",
  "IP-MIB::icmpInEchos.0" => "icmpInEchos",
  "IP-MIB::icmpInEchoReps.0" => "icmpInEchoReps",
  "IP-MIB::icmpInTimestamps.0" => "icmpInTimestamps",
  "IP-MIB::icmpInTimestampReps.0" => "icmpInTimestampReps",
  "IP-MIB::icmpInAddrMasks.0" => "icmpInAddrMasks",
  "IP-MIB::icmpInAddrMaskReps.0" => "icmpInAddrMaskReps",
  "IP-MIB::icmpOutMsgs.0" => "icmpOutMsgs",
  "IP-MIB::icmpOutErrors.0" => "icmpOutErrors",
  "IP-MIB::icmpOutDestUnreachs.0" => "icmpOutDestUnreachs",
  "IP-MIB::icmpOutTimeExcds.0" => "icmpOutTimeExcds",
  "IP-MIB::icmpOutParmProbs.0" => "icmpOutParmProbs",
  "IP-MIB::icmpOutSrcQuenchs.0" => "icmpOutSrcQuenchs",
  "IP-MIB::icmpOutRedirects.0" => "icmpOutRedirects",
  "IP-MIB::icmpOutEchos.0" => "icmpOutEchos",
  "IP-MIB::icmpOutEchoReps.0" => "icmpOutEchoReps",
  "IP-MIB::icmpOutTimestamps.0" => "icmpOutTimestamps",
  "IP-MIB::icmpOutTimestampReps.0" => "icmpOutTimestampReps",
  "IP-MIB::icmpOutAddrMasks.0" => "icmpOutAddrMasks",
  "IP-MIB::icmpOutAddrMaskReps.0" => "icmpOutAddrMaskReps",
);

my @args;
{
  chomp(my @output = `snmpbulkwalk -v2c -c "$community" "$host" .1.3.6.1.2.1.4`);
  my ($seconds, $microseconds) = gettimeofday;
  my $time = int(($seconds * 1000) + ($microseconds / 1000));

  foreach (@output) {
    my ($key, $valuepart) = split / = /, $_, 2;
    my ($type, $value) = split /: /, $valuepart, 2;
    next unless $key and defined $value;
    next unless $stats{$key};
    next unless $value =~ /^\d+(?:\.\d+)?$/;
    push @args, "/network/stats/ip/$stats{$key}". "{hostname=$host,srchost=$localhost}:$value\@$time";
  }
}

{
  chomp(my @output = `snmpbulkwalk -v2c -c "$community" "$host" .1.3.6.1.2.1.5`);
  my ($seconds, $microseconds) = gettimeofday;
  my $time = int(($seconds * 1000) + ($microseconds / 1000));

  foreach (@output) {
    my ($key, $valuepart) = split / = /, $_, 2;
    my ($type, $value) = split /: /, $valuepart, 2;
    next unless $key and defined $value;
    next unless $stats{$key};
    next unless $value =~ /^\d+(?:\.\d+)?$/;
    push @args, "/network/stats/icmp/$stats{$key}". "{hostname=$host,srchost=$localhost}:$value\@$time";
  }
}

for (my $i = 0; $i < scalar @args; $i += $maxargs) {
  my $max = ($i + 50 >= scalar @args) ? scalar @args : $i + $maxargs;
  my @argslice = @args[$i..$max - 1];
  #print "$client --logtostderr --hostname $host $store ". join(" ", @argslice). "\n";
  system($client, "--logtostderr", "--hostname", $host, $store, @argslice);
  exit 1 unless $? == 0;
}
