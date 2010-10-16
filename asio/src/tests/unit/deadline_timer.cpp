//
// deadline_timer.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Disable autolinking for unit tests.
#if !defined(BOOST_ALL_NO_LIB)
#define BOOST_ALL_NO_LIB 1
#endif // !defined(BOOST_ALL_NO_LIB)

// Test that header file is self-contained.
#include "asio/deadline_timer.hpp"

#include <boost/bind.hpp>
#include "asio/io_service.hpp"
#include "asio/placeholders.hpp"
#include "asio/thread.hpp"
#include "unit_test.hpp"

using namespace boost::posix_time;

void increment(int* count)
{
  ++(*count);
}

void decrement_to_zero(asio::deadline_timer* t, int* count)
{
  if (*count > 0)
  {
    --(*count);

    int before_value = *count;

    t->expires_at(t->expires_at() + seconds(1));
    t->async_wait(boost::bind(decrement_to_zero, t, count));

    // Completion cannot nest, so count value should remain unchanged.
    BOOST_CHECK(*count == before_value);
  }
}

void increment_if_not_cancelled(int* count,
    const asio::error_code& ec)
{
  if (!ec)
    ++(*count);
}

void cancel_timer(asio::deadline_timer* t)
{
  std::size_t num_cancelled = t->cancel();
  BOOST_CHECK(num_cancelled == 1);
}

ptime now()
{
#if defined(BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK)
  return microsec_clock::universal_time();
#else // defined(BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK)
  return second_clock::universal_time();
#endif // defined(BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK)
}

void deadline_timer_test()
{
  asio::io_service ios;
  int count = 0;

  ptime start = now();

  asio::deadline_timer t1(ios, seconds(1));
  t1.wait();

  // The timer must block until after its expiry time.
  ptime end = now();
  ptime expected_end = start + seconds(1);
  BOOST_CHECK(expected_end < end || expected_end == end);

  start = now();

  asio::deadline_timer t2(ios, seconds(1) + microseconds(500000));
  t2.wait();

  // The timer must block until after its expiry time.
  end = now();
  expected_end = start + seconds(1) + microseconds(500000);
  BOOST_CHECK(expected_end < end || expected_end == end);

  t2.expires_at(t2.expires_at() + seconds(1));
  t2.wait();

  // The timer must block until after its expiry time.
  end = now();
  expected_end += seconds(1);
  BOOST_CHECK(expected_end < end || expected_end == end);

  start = now();

  t2.expires_from_now(seconds(1) + microseconds(200000));
  t2.wait();

  // The timer must block until after its expiry time.
  end = now();
  expected_end = start + seconds(1) + microseconds(200000);
  BOOST_CHECK(expected_end < end || expected_end == end);

  start = now();

  asio::deadline_timer t3(ios, seconds(5));
  t3.async_wait(boost::bind(increment, &count));

  // No completions can be delivered until run() is called.
  BOOST_CHECK(count == 0);

  ios.run();

  // The run() call will not return until all operations have finished, and
  // this should not be until after the timer's expiry time.
  BOOST_CHECK(count == 1);
  end = now();
  expected_end = start + seconds(1);
  BOOST_CHECK(expected_end < end || expected_end == end);

  count = 3;
  start = now();

  asio::deadline_timer t4(ios, seconds(1));
  t4.async_wait(boost::bind(decrement_to_zero, &t4, &count));

  // No completions can be delivered until run() is called.
  BOOST_CHECK(count == 3);

  ios.reset();
  ios.run();

  // The run() call will not return until all operations have finished, and
  // this should not be until after the timer's final expiry time.
  BOOST_CHECK(count == 0);
  end = now();
  expected_end = start + seconds(3);
  BOOST_CHECK(expected_end < end || expected_end == end);

  count = 0;
  start = now();

  asio::deadline_timer t5(ios, seconds(10));
  t5.async_wait(boost::bind(increment_if_not_cancelled, &count,
        asio::placeholders::error));
  asio::deadline_timer t6(ios, seconds(1));
  t6.async_wait(boost::bind(cancel_timer, &t5));

  // No completions can be delivered until run() is called.
  BOOST_CHECK(count == 0);

  ios.reset();
  ios.run();

  // The timer should have been cancelled, so count should not have changed.
  // The total run time should not have been much more than 1 second (and
  // certainly far less than 10 seconds).
  BOOST_CHECK(count == 0);
  end = now();
  expected_end = start + seconds(2);
  BOOST_CHECK(end < expected_end);

  // Wait on the timer again without cancelling it. This time the asynchronous
  // wait should run to completion and increment the counter.
  t5.async_wait(boost::bind(increment_if_not_cancelled, &count,
        asio::placeholders::error));

  ios.reset();
  ios.run();

  // The timer should not have been cancelled, so count should have changed.
  // The total time since the timer was created should be more than 10 seconds.
  BOOST_CHECK(count == 1);
  end = now();
  expected_end = start + seconds(10);
  BOOST_CHECK(expected_end < end || expected_end == end);
}

void timer_handler(const asio::error_code&)
{
}

void deadline_timer_cancel_test()
{
  static asio::io_service io_service;
  struct timer
  {
    asio::deadline_timer t;
    timer() : t(io_service) { t.expires_at(boost::posix_time::pos_infin); }
  } timers[50];

  timers[2].t.async_wait(timer_handler);
  timers[41].t.async_wait(timer_handler);
  for (int i = 10; i < 20; ++i)
    timers[i].t.async_wait(timer_handler);

  BOOST_CHECK(timers[2].t.cancel() == 1);
  BOOST_CHECK(timers[41].t.cancel() == 1);
  for (int i = 10; i < 20; ++i)
    BOOST_CHECK(timers[i].t.cancel() == 1);
}

struct custom_allocation_timer_handler
{
  custom_allocation_timer_handler(int* count) : count_(count) {}
  void operator()(const asio::error_code&) {}
  int* count_;
};

void* asio_handler_allocate(std::size_t size,
    custom_allocation_timer_handler* handler)
{
  ++(*handler->count_);
  return ::operator new(size);
}

void asio_handler_deallocate(void* pointer, std::size_t,
    custom_allocation_timer_handler* handler)
{
  --(*handler->count_);
  ::operator delete(pointer);
}

void deadline_timer_custom_allocation_test()
{
  static asio::io_service io_service;
  struct timer
  {
    asio::deadline_timer t;
    timer() : t(io_service) {}
  } timers[100];

  int allocation_count = 0;

  for (int i = 0; i < 50; ++i)
  {
    timers[i].t.expires_at(boost::posix_time::pos_infin);
    timers[i].t.async_wait(custom_allocation_timer_handler(&allocation_count));
  }

  for (int i = 50; i < 100; ++i)
  {
    timers[i].t.expires_at(boost::posix_time::neg_infin);
    timers[i].t.async_wait(custom_allocation_timer_handler(&allocation_count));
  }

  for (int i = 0; i < 50; ++i)
    timers[i].t.cancel();

  io_service.run();

  BOOST_CHECK(allocation_count == 0);
}

void deadline_timer_thread_test()
{
  asio::io_service ios;
  asio::io_service::work w(ios);
  asio::deadline_timer t1(ios);
  asio::deadline_timer t2(ios);
  int count = 0;

  asio::thread th(boost::bind(&asio::io_service::run, &ios));

  t2.expires_from_now(boost::posix_time::seconds(2));
  t2.wait();

  t1.expires_from_now(boost::posix_time::seconds(2));
  t1.async_wait(boost::bind(increment, &count));

  t2.expires_from_now(boost::posix_time::seconds(4));
  t2.wait();

  ios.stop();
  th.join();

  BOOST_CHECK(count == 1);
}

test_suite* init_unit_test_suite(int, char*[])
{
  test_suite* test = BOOST_TEST_SUITE("deadline_timer");
  test->add(BOOST_TEST_CASE(&deadline_timer_test));
  test->add(BOOST_TEST_CASE(&deadline_timer_cancel_test));
  test->add(BOOST_TEST_CASE(&deadline_timer_custom_allocation_test));
  test->add(BOOST_TEST_CASE(&deadline_timer_thread_test));
  return test;
}
