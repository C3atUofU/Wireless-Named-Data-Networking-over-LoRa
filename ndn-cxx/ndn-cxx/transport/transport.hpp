/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#ifndef NDN_TRANSPORT_TRANSPORT_HPP
#define NDN_TRANSPORT_TRANSPORT_HPP

#include "ndn-cxx/detail/asio-fwd.hpp"
#include "ndn-cxx/detail/common.hpp"
#include "ndn-cxx/encoding/block.hpp"

#include <boost/system/error_code.hpp>

namespace ndn {

/** \brief Provides TLV-block delivery service.
 */
class Transport : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;

    Error(const boost::system::error_code& code, const std::string& msg);
  };

  using ReceiveCallback = std::function<void(const Block& wire)>;
  using ErrorCallback = std::function<void()>;

  virtual
  ~Transport() = default;

  /** \brief Asynchronously open the connection.
   *  \param ioService io_service to create socket on
   *  \param receiveCallback callback function when a TLV block is received; must not be empty
   *  \throw boost::system::system_error connection cannot be established
   */
  virtual void
  connect(boost::asio::io_service& ioService, ReceiveCallback receiveCallback);

  /** \brief Close the connection.
   */
  virtual void
  close() = 0;

  /** \brief send a TLV block through the transport
   */
  virtual void
  send(const Block& wire) = 0;

  /** \brief send two memory blocks through the transport
   *
   *  Scatter/gather API is utilized to send two non-consecutive memory blocks together
   *  (as part of the same message in datagram-oriented transports).
   */
  virtual void
  send(const Block& header, const Block& payload) = 0;

  /** \brief pause the transport
   *  \post the receive callback will not be invoked
   *  \note This operation has no effect if transport has been paused,
   *        or when connection is being established.
   */
  virtual void
  pause() = 0;

  /** \brief resume the transport
   *  \post the receive callback will be invoked
   *  \note This operation has no effect if transport is not paused,
   *        or when connection is being established.
   */
  virtual void
  resume() = 0;

  /** \retval true connection has been established
   *  \retval false connection is not yet established or has been closed
   */
  bool
  isConnected() const noexcept
  {
    return m_isConnected;
  }

  /** \retval true incoming packets are expected, the receive callback will be invoked
   *  \retval false incoming packets are not expected, the receive callback will not be invoked
   */
  bool
  isReceiving() const noexcept
  {
    return m_isReceiving;
  }

protected:
  boost::asio::io_service* m_ioService = nullptr;
  ReceiveCallback m_receiveCallback;
  bool m_isConnected = false;
  bool m_isReceiving = false;
};

} // namespace ndn

#endif // NDN_TRANSPORT_TRANSPORT_HPP
