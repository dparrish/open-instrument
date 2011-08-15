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

my @output = `snmptable -Cf "\t" -Cl -v2c -c "$community" "$host" "RFC1213-MIB::ifTable"`;
my ($seconds, $microseconds) = gettimeofday;
my $time = int(($seconds * 1000) + ($microseconds / 1000));

shift @output;
shift @output;
chomp(my $header = shift @output);
my @columns = split /\t/, $header;
my @rows;
foreach (@output) {
  chomp;
  my %row;
  my @values = split /\t/, $_;
  @row{@columns} = @values;
  push @rows, \%row;
}
my @args;
foreach my $row (@rows) {
  my $name = $row->{ifDescr};
  $name =~ s/ //g;
  next if $row->{ifOperStatus} eq 'down';
  next unless $row->{ifAdminStatus} eq 'up';
  foreach my $stat (keys %$row) {
    next unless $row->{$stat} =~ /^\d+(?:\.\d+)?$/;
    push @args, "/network/interface/stats/${stat}".
                "{interface=$name,hostname=$host,srchost=$localhost}:$row->{$stat}\@${time}";
  }
}

for (my $i = 0; $i < scalar @args; $i += $maxargs) {
  my $max = ($i + 50 >= scalar @args) ? scalar @args : $i + $maxargs;
  my @argslice = @args[$i..$max - 1];
  system($client, "--logtostderr", "--hostname", $host, $store, @argslice);
  exit 1 unless $? == 0;
}
