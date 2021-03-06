#pragma once
#include <config.hpp>
#include "logger.hpp"
#include "cycletimer.hpp"
#include <string>
#include <boost/current_function.hpp> 
#include <stdint.h>

#define SCOPED_TIMER \
	ScopedTimer __s(BOOST_CURRENT_FUNCTION);

#define SCOPED_TIMER_NOLOCK \
	ScopedTimerLockfree __s(BOOST_CURRENT_FUNCTION);


class PerfTracker {
	public:
		static PerfTracker &instance();
		void add_time(const std::string &func, double time);
		void add_time_nolock(const std::string &func, double time);
		std::map<std::string, std::pair<double, uint64_t> > &times();
    	bool save(const std::string &file_path);
    	void clear();
		
	private:
		PerfTracker();
		~PerfTracker();

		static PerfTracker *s_instance;
		std::map<std::string, std::pair<double, uint64_t> > _times;
};

class ScopedTimer {
public:
  ScopedTimer(const std::string &f) : func(f), st(CycleTimer::currentSeconds()) { }
  ~ScopedTimer() {
  	PerfTracker::instance().add_time(func, CycleTimer::currentSeconds() - st);
  }

  std::string func;
  double st;
};

class ScopedTimerLockfree {
public:
  ScopedTimerLockfree(const std::string &f) : func(f), st(CycleTimer::currentSeconds()) { }
  ~ScopedTimerLockfree() {
  	PerfTracker::instance().add_time_nolock(func, CycleTimer::currentSeconds() - st);
  }

  std::string func;
  double st;
};

std::ostream& operator<< (std::ostream &out, PerfTracker &pt);


namespace misc {
	std::string get_machine_name();
};
