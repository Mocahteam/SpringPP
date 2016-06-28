//
// stream_handle_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_WINDOWS_STREAM_HANDLE_SERVICE_HPP
#define BOOST_ASIO_WINDOWS_STREAM_HANDLE_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/push_options.hpp>

#include <boost/asio/detail/push_options.hpp>
#include <cstddef>
#include <boost/config.hpp>
#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/detail/service_base.hpp>
#include <boost/asio/detail/win_iocp_handle_service.hpp>

#if !defined(BOOST_ASIO_DISABLE_WINDOWS_STREAM_HANDLE)
# if defined(BOOST_ASIO_HAS_IOCP)
#  define BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE 1
# endif // defined(BOOST_ASIO_HAS_IOCP)
#endif // !defined(BOOST_ASIO_DISABLE_WINDOWS_STREAM_HANDLE)

#if defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE) \
  || defined(GENERATING_DOCUMENTATION)

namespace boost {
namespace asio {
namespace windows {

/// Default service implementation for a stream handle.
class stream_handle_service
#if defined(GENERATING_DOCUMENTATION)
  : public boost::asio::io_service::service
#else
  : public boost::asio::detail::service_base<stream_handle_service>
#endif
{
public:
#if defined(GENERATING_DOCUMENTATION)
  /// The unique service identifier.
  static boost::asio::io_service::id id;
#endif

private:
  // The type of the platform-specific implementation.
  typedef detail::win_iocp_handle_service service_impl_type;

public:
  /// The type of a stream handle implementation.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined implementation_type;
#else
  typedef service_impl_type::implementation_type implementation_type;
#endif

  /// The native handle type.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined native_type;
#else
  typedef service_impl_type::native_type native_type;
#endif

  /// Construct a new stream handle service for the specified io_service.
  explicit stream_handle_service(boost::asio::io_service& io_service)
    : boost::asio::detail::service_base<stream_handle_service>(io_service),
      service_impl_(boost::asio::use_service<service_impl_type>(io_service))
  {
  }

  /// Destroy all user-defined handler objects owned by the service.
  void shutdown_service()
  {
  }

  /// Construct a new stream handle implementation.
  void construct(implementation_type& impl)
  {
    service_impl_.construct(impl);
  }

  /// Destroy a stream handle implementation.
  void destroy(implementation_type& impl)
  {
    service_impl_.destroy(impl);
  }

  /// Assign an existing native handle to a stream handle.
  boost::system::error_code assign(implementation_type& impl,
      const native_type& native_handle, boost::system::error_code& ec)
  {
    return service_impl_.assign(impl, native_handle, ec);
  }

  /// Determine whether the handle is open.
  bool is_open(const implementation_type& impl) const
  {
    return service_impl_.is_open(impl);
  }

  /// Close a stream handle implementation.
  boost::system::error_code close(implementation_type& impl,
      boost::system::error_code& ec)
  {
    return service_impl_.close(impl, ec);
  }

  /// Get the native handle implementation.
  native_type native(implementation_type& impl)
  {
    return service_impl_.native(impl);
  }

  /// Cancel all asynchronous operations associated with the handle.
  boost::system::error_code cancel(implementation_type& impl,
      boost::system::error_code& ec)
  {
    return service_impl_.cancel(impl, ec);
  }

  /// Write the given data to the stream.
  template <typename ConstBufferSequence>
  std::size_t write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, boost::system::error_code& ec)
  {
    return service_impl_.write_some(impl, buffers, ec);
  }

  /// Start an asynchronous write.
  template <typename ConstBufferSequence, typename WriteHandler>
  void async_write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, WriteHandler handler)
  {
    service_impl_.async_write_some(impl, buffers, handler);
  }

  /// Read some data from the stream.
  template <typename MutableBufferSequence>
  std::size_t read_some(implementation_type& impl,
      const MutableBufferSequence& buffers, boost::system::error_code& ec)
  {
    return service_impl_.read_some(impl, buffers, ec);
  }

  /// Start an asynchronous read.
  template <typename MutableBufferSequence, typename ReadHandler>
  void async_read_some(implementation_type& impl,
      const MutableBufferSequence& buffers, ReadHandler handler)
  {
    service_impl_.async_read_some(impl, buffers, handler);
  }

private:
  // The service that provides the platform-specific implementation.
  service_impl_type& service_impl_;
};

} // namespace windows
} // namespace asio
} // namespace boost

#endif // defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
       //   || defined(GENERATING_DOCUMENTATION)

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_WINDOWS_STREAM_HANDLE_SERVICE_HPP
