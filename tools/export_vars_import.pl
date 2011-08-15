#!/usr/bin/perl -w

use strict;
use HTTP::Response;
use HTTP::Status;
use LWP::UserAgent;
use Time::HiRes qw( gettimeofday );
use URI::URL;

if (scalar @ARGV < 3) {
  exit 1;
}
my $jobname = $ARGV[0];
my $host = $ARGV[1];
my $port = $ARGV[2];

my $instance = "datastore";
my $store = "192.168.1.5:8020";
my $client = "/home/dparrish/src/openinstrument/client/cpp/add";
my $maxargs = 50;

my($code, $body) = simple_get("http://$host:$port/export_vars");
if ($code != 200) {
  print "Error from http://$host:$port/export_vars\n";
  exit(1);
}
chomp(my @output = split /[\n\r]+/, $body);

my ($seconds, $microseconds) = gettimeofday;
my $time = int(($seconds * 1000) + ($microseconds / 1000));

my @args;
foreach (@output) {
  my($key, $value) = split /\s+/, $_, 2;
  $value = int($value);
  next if $key eq '' or $value eq '';
  push @args, "$key\{hostname=$host,port=$port,job=$jobname}:$value\@$time";
}

for (my $i = 0; $i < scalar @args; $i += $maxargs) {
  my $max = ($i + 50 >= scalar @args) ? scalar @args : $i + $maxargs;
  my @argslice = @args[$i..$max - 1];
  #print "$client --logtostderr $store ". join(" ", @argslice). "\n";
  system($client, "--logtostderr", $store, @argslice);
  exit 1 unless $? == 0;
}

sub simple_get {
  my ($path) = @_;
  my $ua = new LWP::UserAgent;
  $ua->agent("OpenInstrumentFetcher/1.0");
  my $request = new HTTP::Request('GET', $path);
  my $response = $ua->request($request);
  return ($response->code, $response->content);
}

