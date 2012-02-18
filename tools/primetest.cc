/*
 * Calculate prime numbers as fast as possible forever.
 * Log current statistics to the data store.
 *
 * This program is only useful as a demonstration of how to record real-time statistics.
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <gflags/gflags.h>
#include <math.h>
#include "lib/common.h"
#include "lib/socket.h"
#include "lib/exported_vars.h"

DEFINE_string(datastore, "localhost:8020", "Address and port of data store");
DEFINE_int32(record_interval, 300, "How often to record statistics (in seconds)");
DEFINE_int64(max_check, static_cast<uint64_t>(-1), "Maximum number to check. Default is to run effectively forever.");

namespace openinstrument {

class PrimeCalculator {
 public:
  PrimeCalculator()
    : num_primes_("/openinstrument/test/primetest/primes_found"),
      num_checked_("/openinstrument/test/primetest/numbers_tested") {
    VariableExporter::GetGlobalExporter()->SetExportLabel("job", "primecalculator");
    VariableExporter::GetGlobalExporter()->SetExportLabel("hostname", Socket::Hostname());
    num_primes_.mutable_variable()->set_rate();
    num_checked_.mutable_variable()->set_rate();
  };

  void Run() {
    // Export stats every second
    VariableExporter::GetGlobalExporter()->StartExportThread(FLAGS_datastore, FLAGS_record_interval);
    for (uint64_t i = 0; ; i++) {
      ++num_checked_;
      if (IsPrime(i))
        ++num_primes_;
    }
  }

  inline bool IsPrime(uint64_t num) {
    if (num <=1)
      return false;
    else if (num == 2)
      return true;
    else if (num % 2 == 0)
      return false;
    else {
      uint64_t divisor = 3;
      double num_d = static_cast<double>(num);
      uint64_t upperLimit = static_cast<uint64_t>(sqrt(num_d) + 1);

      while (divisor <= upperLimit) {
        if (num % divisor == 0)
          return false;
        divisor +=2;
      }
      return true;
    }
  }

 private:
  ExportedInteger num_primes_;
  ExportedInteger num_checked_;
};

}

int main(int argc, char *argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  openinstrument::PrimeCalculator calculator;
  calculator.Run();
}
