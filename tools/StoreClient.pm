#!/usr/bin/perl -w

package StoreClient;

use strict;
use Time::HiRes qw( gettimeofday );
use Sys::Hostname;

sub new {
  my($class, $store) = @_;
  my $self = bless {}, $class;

  $self->{_add_tool} = "/home/dparrish/src/openinstrument/client/cpp/add";
  $self->{_get_tool} = "/home/dparrish/src/openinstrument/client/cpp/get";
  $self->{_list_tool} = "/home/dparrish/src/openinstrument/client/cpp/list";
  $self->{_max_args} = 50;

  $self->{hostname} = hostname;
  $self->{store} = $store;

  die "Specify a datastore host:port" unless $self->{store};

  return $self;
};

# Add a set of variables to the datastore.
# Arguments to this should be a list of variable => value items.
sub Add {
  my($self, @args) = @_;
  return unless @args;
  my $time = $self->TimeNow();
  if (scalar(@args) % 2 != 0) {
    die "Specify a list of variable => value items to Add()";
  }
  my @output;
  for (my $i = 0; $i < scalar @args; $i += 2) {
    my($variable, $value) = ($args[$i], $args[$i + 1]);
    next unless defined $value;
    die "Value \"$value\" is not a number" unless $value =~ /^\d+(?:\.\d+)?$/;
    push @output, "$variable:$value\@$time";
  }
  $self->RunCommand([$self->{_add_tool}, $self->{store}], @output);
  return unless @output;
}

sub Get {
  my($self) = @_;
}

sub List {
  my($self) = @_;
}

sub TimeNow {
  my($self) = @_;
  my ($seconds, $microseconds) = gettimeofday;
  return int(($seconds * 1000) + ($microseconds / 1000));
}

sub RunCommand {
  my($self, $repeat_args, @args) = @_;
  for (my $i = 0; $i < scalar @args; $i += $self->{_max_args}) {
    my $max = ($i + $self->{_max_args} >= scalar @args) ? scalar @args : $i + $self->{_max_args};
    my @run_args = @$repeat_args;
    push @run_args, @args[$i..$max - 1];
    #print join(" ", @run_args). "\n";
    system(@run_args);
    exit 1 unless $? == 0;
  }
}

1;
