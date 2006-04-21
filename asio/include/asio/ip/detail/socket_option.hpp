//
// socket_option.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2006 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IP_DETAIL_SOCKET_OPTION_HPP
#define ASIO_IP_DETAIL_SOCKET_OPTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/push_options.hpp"

#include "asio/detail/push_options.hpp"
#include <cstddef>
#include <cstring>
#include <boost/config.hpp>
#include "asio/detail/pop_options.hpp"

#include "asio/ip/address.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/detail/socket_types.hpp"

namespace asio {
namespace ip {
namespace detail {
namespace socket_option {

// Helper template for implementing boolean-based options.
template <int IPv4_Level, int IPv4_Name, int IPv6_Level, int IPv6_Name>
class boolean
{
public:
  // Default constructor.
  boolean()
    : value_(0)
  {
  }

  // Construct with a specific option value.
  boolean(bool value)
    : value_(value ? 1 : 0)
  {
  }

  // Set the value of the boolean.
  void set(bool value)
  {
    value_ = value ? 1 : 0;
  }

  // Get the current value of the boolean.
  bool get() const
  {
    return value_;
  }

  // Get the level of the socket option.
  template <typename Protocol>
  int level(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Level;
    return IPv4_Level;
  }

  // Get the name of the socket option.
  template <typename Protocol>
  int name(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Name;
    return IPv4_Name;
  }

  // Get the address of the boolean data.
  template <typename Protocol>
  int* data(const Protocol&)
  {
    return &value_;
  }

  // Get the address of the boolean data.
  template <typename Protocol>
  const int* data(const Protocol&) const
  {
    return &value_;
  }

  // Get the size of the boolean data.
  template <typename Protocol>
  std::size_t size(const Protocol&) const
  {
    return sizeof(value_);
  }

private:
  int value_;
};

// Helper template for implementing integer options.
template <int IPv4_Level, int IPv4_Name, int IPv6_Level, int IPv6_Name>
class integer
{
public:
  // Default constructor.
  integer()
    : value_(0)
  {
  }

  // Construct with a specific option value.
  integer(int value)
    : value_(value)
  {
  }

  // Set the value of the int option.
  void set(int value)
  {
    value_ = value;
  }

  // Get the current value of the int option.
  int get() const
  {
    return value_;
  }

  // Get the level of the socket option.
  template <typename Protocol>
  int level(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Level;
    return IPv4_Level;
  }

  // Get the name of the socket option.
  template <typename Protocol>
  int name(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Name;
    return IPv4_Name;
  }

  // Get the address of the int data.
  template <typename Protocol>
  int* data(const Protocol&)
  {
    return &value_;
  }

  // Get the address of the int data.
  template <typename Protocol>
  const int* data(const Protocol&) const
  {
    return &value_;
  }

  // Get the size of the int data.
  template <typename Protocol>
  std::size_t size(const Protocol&) const
  {
    return sizeof(value_);
  }

private:
  int value_;
};

// Helper template for implementing ip_mreq-based options.
template <int IPv4_Level, int IPv4_Name, int IPv6_Level, int IPv6_Name>
class multicast_request
{
public:
  // Default constructor.
  multicast_request()
  {
    ipv4_value_.imr_multiaddr.s_addr =
      asio::detail::socket_ops::host_to_network_long(
          asio::ipv4::address::any().to_ulong());
    ipv4_value_.imr_interface.s_addr =
      asio::detail::socket_ops::host_to_network_long(
          asio::ipv4::address::any().to_ulong());

    in6_addr tmp_addr = IN6ADDR_ANY_INIT;
    ipv6_value_.ipv6mr_multiaddr = tmp_addr;
    ipv6_value_.ipv6mr_interface = 0;
  }

  // Construct with multicast address only.
  multicast_request(const asio::ip::address& multicast_address)
  {
    if (multicast_address.is_ipv6())
    {
      ipv4_value_.imr_multiaddr.s_addr =
        asio::detail::socket_ops::host_to_network_long(
            asio::ipv4::address::any().to_ulong());
      ipv4_value_.imr_interface.s_addr =
        asio::detail::socket_ops::host_to_network_long(
            asio::ipv4::address::any().to_ulong());

      using namespace std; // For memcpy.
      asio::ipv6::address ipv6_address = multicast_address.to_ipv6();
      asio::ipv6::address::bytes_type bytes = ipv6_address.to_bytes();
      memcpy(ipv6_value_.ipv6mr_multiaddr.s6_addr, bytes.elems, 16);
      ipv6_value_.ipv6mr_interface = 0;
    }
    else
    {
      ipv4_value_.imr_multiaddr.s_addr =
        asio::detail::socket_ops::host_to_network_long(
            multicast_address.to_ipv4().to_ulong());
      ipv4_value_.imr_interface.s_addr =
        asio::detail::socket_ops::host_to_network_long(
            asio::ipv4::address::any().to_ulong());

      in6_addr tmp_addr = IN6ADDR_ANY_INIT;
      ipv6_value_.ipv6mr_multiaddr = tmp_addr;
      ipv6_value_.ipv6mr_interface = 0;
    }
  }

  // Get the level of the socket option.
  template <typename Protocol>
  int level(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Level;
    return IPv4_Level;
  }

  // Get the name of the socket option.
  template <typename Protocol>
  int name(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Name;
    return IPv4_Name;
  }

  // Get the address of the option data.
  template <typename Protocol>
  void* data(const Protocol& protocol)
  {
    if (protocol.family() == PF_INET6)
      return &ipv6_value_;
    return &ipv4_value_;
  }

  // Get the address of the option data.
  template <typename Protocol>
  const void* data(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return &ipv6_value_;
    return &ipv4_value_;
  }

  // Get the size of the option data.
  template <typename Protocol>
  std::size_t size(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return sizeof(ipv6_value_);
    return sizeof(ipv4_value_);
  }

private:
  ip_mreq ipv4_value_;
  ipv6_mreq ipv6_value_;
};

} // namespace socket_option
} // namespace detail
} // namespace ip
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IP_DETAIL_SOCKET_OPTION_HPP
