/**
 *  @file	TransferSpeed.hpp
 *  @brief	Measures the transfer speed via TPC for a given chunk size.
 *  @author	J. Korinth, TU Darmstadt (jk@esa.cs.tu-darmstadt.de)
 **/
#ifndef __TRANSFER_SPEED_HPP__
#define __TRANSFER_SPEED_HPP__

#include <atomic>
#include <thread>
#include <future>
#include <vector>
#include <sstream>
#include <chrono>
#include <cmath>
#include <unistd.h>
#include <ncurses.h>
#include <tapasco_api.hpp>

using namespace std;
using namespace std::chrono;
using namespace tapasco;

/** Measurement class that can measure TPC memory transfer speeds. **/
class TransferSpeed {
public:
  TransferSpeed(Tapasco& tapasco) : tapasco(tapasco) {}
  virtual ~TransferSpeed() {}

  static constexpr long OP_ALLOCFREE = 0;
  static constexpr long OP_COPYFROM  = 1;
  static constexpr long OP_COPYTO    = 2;

  double operator()(size_t const chunk_sz, long const opmask = 3) {
    double const cs = chunk_sz / 1024.0;
    string const ms = maskToString(opmask);
    CumulativeAverage<double> cavg { 0 };
    bool stop = false;
    bytes = 0;
    initscr(); noecho(); curs_set(0); timeout(0);
    int x, y;
    getyx(stdscr, y, x);
    auto tstart = steady_clock::now();
    double b = 0.0;
    duration<double> d = steady_clock::now() - tstart;
    future<void> f = async(launch::async, [&]() { transfer(stop, chunk_sz, opmask); });
    do {
      mvprintw(y, x, "Chunk size: %8.2f KiB, Mask: %s, Speed: %8.2f MiB/s",
          cs, ms.c_str(), cavg());
      refresh();
      usleep(1000000);
      b = bytes.load() / (1024.0 * 1024.0);
      d = steady_clock::now() - tstart;
    } while (getch() == ERR && (fabs(cavg.update(b / d.count())) > 0.1 || cavg.size() < 30));
    stop = true;
    f.get();
    move(y+1, 0);
    endwin();
    return cavg();
  }

private:
  void transfer(volatile bool& stop, size_t const chunk_sz, long opmask) {
    tapasco_handle_t h;
    uint8_t *data = new (std::nothrow) uint8_t[chunk_sz];
    if (! data) return;
    for (size_t i = 0; i < chunk_sz; ++i)
      data[i] = rand();

    while (! stop) {
      if (tapasco.alloc(h, chunk_sz, TAPASCO_DEVICE_ALLOC_FLAGS_NONE) != TAPASCO_SUCCESS)
        throw "allocation failed";
      if (opmask & OP_COPYTO && tapasco.copy_to(data, h, chunk_sz, TAPASCO_DEVICE_COPY_BLOCKING) != TAPASCO_SUCCESS)
        throw "copy_to failed";
      if (opmask & OP_COPYFROM && tapasco.copy_from(h, data, chunk_sz, TAPASCO_DEVICE_COPY_BLOCKING) != TAPASCO_SUCCESS)
        throw "copy_from failed";
      if (opmask & OP_COPYFROM)
        bytes += chunk_sz;
      if (opmask & OP_COPYTO)
        bytes += chunk_sz;
      tapasco.free(h, TAPASCO_DEVICE_ALLOC_FLAGS_NONE);
    }
    delete[] data;
  }

  static const std::string maskToString(long const opmask) {
    stringstream tmp;
    tmp << (opmask & OP_COPYFROM ? "r" : " ") 
        << (opmask & OP_COPYTO   ? "w" : " ");
    return tmp.str();
  }

  atomic<uint64_t> bytes { 0 };
  Tapasco& tapasco;
};

#endif /* __TRANSFER_SPEED_HPP__ */
/* vim: set foldmarker=@{,@} foldlevel=0 foldmethod=marker : */
