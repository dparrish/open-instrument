#!/usr/bin/perl -w

sub BEGIN {
  use vars qw( @INC );
  my @parts = split /\//, $0;
  pop @parts;
  my $dirname = join "/", @parts;
  push @INC, $dirname;
}
use strict;
use StoreClient;
use Sys::Hostname;

my $host = $ARGV[0];
my $community = $ARGV[1];
my $interval = $ARGV[2] || 300;

die "Specify <host> <community> [interval]" unless $host && $community;

my $client = new StoreClient("localhost:8020");
my $localhost = hostname;

my $inode = (stat($0))[1];
my $mtime = (stat(_))[9];

while (1) {
  my $newinode = (stat($0))[1];
  my $newmtime = (stat(_))[9];
  if ($inode != $newinode || $mtime != $newmtime) {
    # File has changed, restart
    print STDERR "File $0 has changed, re-executing\n";
    exec($0, @ARGV) || die "Can't re-execute: $!";
  }

  my @labels = ( "hostname=$host", "srchost=$localhost" );
  # Network interface stats
  CollectInterfaceStats($host, $community, $client, \@labels);

  # Filesystem (local and network) stats
  CollectFilesystemStats($host, $community, $client, \@labels);

  # Stats on active and total sockets
  CollectSocketStats($host, $community, $client, \@labels);

  # System stats (memory, cpu, etc)
  CollectSystemStats($host, $community, $client, \@labels);

  sleep($interval);
}

sub CollectInterfaceStats {
  my($host, $community, $client, $all_labels) = @_;
  my $table = GetSnmpTable($host, $community, "RFC1213-MIB::ifTable", "ifDescr", {
    ifIndex => undef,
    ifType => undef,
    ifPhysAddress => undef,
    ifLastChange => undef,
    ifSpecific => undef,
  });
  my %datatypes = (
    ifInDiscards => "counter",
    ifInErrors => "counter",
    ifInOctets => "counter",
    ifInUcastPkts => "counter",
    ifInUnknownProtos => "counter",
    ifMtu => "gauge",
    ifOutDiscards => "counter",
    ifOutErrors => "counter",
    ifOutOctets => "counter",
    ifOutUcastPkts => "counter",
    ifSpeed => "gauge",
  );
  my %args;
  while (my($name, $int) = each %$table) {
    next unless $int->{ifMtu} && $int->{ifSpeed} && defined $int->{ifOutOctets};
    next if $int->{ifOperStatus} eq 'down';
    next unless $int->{ifAdminStatus} eq 'up';
    while (my($key, $value) = each %$int) {
      next unless $int->{$key} =~ /^\d+(?:\.\d+)?$/;
      my $datatype = $datatypes{$key} || "counter";
      my $labels = labels(@$all_labels, "interface=$name", "datatype=$datatype");
      $args{"/network/interface/stats/${key}{$labels}"} = $value;
    }
  }
  $client->Add(%args);
}

sub CollectFilesystemStats {
  my($host, $community, $client, $all_labels) = @_;
  my $table = GetSnmpTable($host, $community, "HOST-RESOURCES-MIB::hrStorageTable", "hrStorageDescr");
  my %args;
  while (my($name, $dev) = each %$table) {
    next unless $dev->{hrStorageType} eq 'HOST-RESOURCES-TYPES::hrStorageFixedDisk' ||
                $dev->{hrStorageType} eq 'HOST-RESOURCES-TYPES::hrStorageNetworkDisk';
    my($block_size) = $dev->{hrStorageAllocationUnits} =~ /^(\d+)/;
    next unless $block_size && $dev->{hrStorageSize};
    my $labels = labels(@$all_labels, "device=$name", "datatype=gauge", "units=bytes");
    $args{"/system/filesystem/size{$labels}"} = $block_size * $dev->{hrStorageSize};
    $args{"/system/filesystem/used{$labels}"} = $block_size * $dev->{hrStorageUsed};
    $args{"/system/filesystem/available{$labels}"} = $block_size * ($dev->{hrStorageSize} - $dev->{hrStorageUsed});
  }
  $client->Add(%args);
}


sub CollectSocketStats {
  my($host, $community, $client, $all_labels) = @_;
  my $list = GetSnmpList($host, $community, "TCP-MIB::tcp");

  # Disable warnings so we don't have to check that each of these values exists
  # first. It's perfectly valid to not have some of these.
  local $^W = 0;
  my $labels = labels(@$all_labels);
  my %args;
  $args{"/network/stats/tcp/tcpCurrEstab{$labels,datatype=gauge}"} = $list->{"tcpCurrEstab.0"};
  $args{"/network/stats/tcp/tcpActiveOpens{$labels,datatype=counter}"} = $list->{"tcpActiveOpens.0"};
  $args{"/network/stats/tcp/tcpPassiveOpens{$labels,datatype=counter}"} = $list->{"tcpPassiveOpens.0"};
  $args{"/network/stats/tcp/tcpAttemptFails{$labels,datatype=counter}"} = $list->{"tcpAttemptFails.0"};
  $args{"/network/stats/tcp/tcpEstabResets{$labels,datatype=counter}"} = $list->{"tcpEstabResets.0"};
  $args{"/network/stats/tcp/tcpInSegs{$labels,datatype=counter}"} = $list->{"tcpInSegs.0"};
  $args{"/network/stats/tcp/tcpOutSegs{$labels,datatype=counter}"} = $list->{"tcpOutSegs.0"};
  $args{"/network/stats/tcp/tcpRetransSegs{$labels,datatype=counter}"} = $list->{"tcpRetransSegs.0"};
  $args{"/network/stats/tcp/tcpInErrs{$labels,datatype=counter}"} = $list->{"tcpInErrs.0"};
  $args{"/network/stats/tcp/tcpOutRsts{$labels,datatype=counter}"} = $list->{"tcpOutRsts.0"};
  $args{"/network/stats/tcp/tcpOutRsts{$labels,datatype=counter}"} = $list->{"yuck"};
  while (my($key, $value) = each %$list) {
    if ($key =~ /^tcpConnState\./) {
      next unless $value =~ /^(.+)\(/;
      $args{"/network/stats/tcp/tcpConnState{$labels,state=$1,datatype=gauge}"} ||= 0;
      $args{"/network/stats/tcp/tcpConnState{$labels,state=$1,datatype=gauge}"}++;
    }
  }

  $list = GetSnmpList($host, $community, "UDP-MIB::udp");
  $args{"/network/stats/udp/udpInDatagrams{$labels,datatype=gauge}"} = $list->{"udpInDatagrams.0"};
  $args{"/network/stats/udp/udpNoPorts{$labels,datatype=counter}"} = $list->{"udpNoPorts.0"};
  $args{"/network/stats/udp/udpInErrors{$labels,datatype=counter}"} = $list->{"udpInErrors.0"};
  $args{"/network/stats/udp/udpOutDatagrams{$labels,datatype=counter}"} = $list->{"udpOutDatagrams.0"};

  $client->Add(%args);
}

sub CollectSystemStats {
  my($host, $community, $client, $all_labels) = @_;
  my $list = GetSnmpList($host, $community, "HOST-RESOURCES-MIB::hrSystem");

  local $^W = 0;
  my $labels = labels(@$all_labels);
  my %args;
  my($uptime) = $list->{"hrSystemUptime.0"} =~ /^\((\d+)\)/;
  $args{"/system/stats/uptime{$labels,datatype=gauge,units=seconds}"} = $uptime / 100
    if $uptime;
  $args{"/system/stats/num_users{$labels,datatype=gauge}"} = $list->{"hrSystemNumUsers.0"};
  $args{"/system/stats/num_processes{$labels,datatype=gauge}"} = $list->{"hrSystemProcesses.0"};

  my $table = GetSnmpTable($host, $community, "HOST-RESOURCES-MIB::hrStorageTable", "hrStorageDescr");
  while (my($name, $dev) = each %$table) {
    my($block_size) = $dev->{hrStorageAllocationUnits} =~ /^(\d+)/;
    next unless $block_size && $dev->{hrStorageSize};
    if ($dev->{hrStorageType} eq 'HOST-RESOURCES-TYPES::hrStorageRam') {
      $args{"/system/stats/ram/size{$labels,datatype=gauge,units=bytes}"} = $block_size * $dev->{hrStorageSize};
      $args{"/system/stats/ram/available{$labels,datatype=gauge,units=bytes}"} = $block_size * ($dev->{hrStorageSize} -
          $dev->{hrStorageUsed});
    }
  }

  $client->Add(%args);
}

sub GetSnmpList {
  my($host, $community, $oid, $field_changes) = @_;
  chomp(my @output = `snmpbulkwalk -v2c -c "$community" "$host" "$oid"`);
  my %output;
  foreach (@output) {
    local $^W = 0;
    my ($key, $valuepart) = split / = /, $_, 2;
    $key =~ s/^.+:://;
    my ($type, $value) = split /: /, $valuepart, 2;
    next unless $key and defined $value;
    $output{$key} = $value;
  }
  return \%output;
}

sub GetSnmpTable {
  my($host, $community, $table, $identity_field, $field_changes) = @_;
  my %table;
  $field_changes ||= {};
  my @output = `snmptable -Cf "\t" -Cl -v2c -c "$community" "$host" "$table"`;
  shift @output while (@output && $output[0] =~ /^(SNMP table:|$)/m);
  return unless @output;
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
  my %args;
  foreach my $row (@rows) {
    my $name = $row->{$identity_field};
    next unless $name;
    $name =~ s/[^A-Za-z0-9\-_\/:]//g;
    next unless $name;
    while (my($from, $to) = each(%$field_changes)) {
      next unless defined $row->{$from};
      $row->{$to} = $row->{$from} if $to;
      delete $row->{$from};
    }
    $table{$name} ||= {};
    foreach my $stat (keys %$row) {
      $table{$name}->{$stat} = $row->{$stat};
    }
  }
  return \%table;
}

sub labels {
  my(@labels) = @_;
  return join(",", grep { $_ ne '' } @labels);
}

