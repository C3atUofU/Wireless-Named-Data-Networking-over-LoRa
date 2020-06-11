/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_FACE_LORA_CHANNEL_HPP
#define NFD_DAEMON_FACE_LORA_CHANNEL_HPP

#include "channel.hpp"
#include <queue>
#include <pthread.h>

 namespace nfd {
namespace face {


/**
 * \brief Class implementing LoRa-based channel to create faces
 */
class LoRaChannel : public Channel
{

public:

  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  LoRaChannel(std::string URI);

  void
  createFace( std::queue<std::pair<std::pair<uint8_t, uint8_t>*, ndn::encoding::EncodingBuffer *>*>* sendBufferQueue,
              pthread_mutex_t* queueMutex,
              std::pair<uint8_t, uint8_t> ids,
              const FaceParams& params,
              const FaceCreatedCallback& onFaceCreated,
              const FaceCreationFailedCallback& onFailure);

  bool
  isListening() const override
  {
    return true;
  }

  size_t
  size() const override
  {
    return m_size;
  }

  void
  handleReceive(ndn::Block);

private:
  std::map<std::string, shared_ptr<Face>> m_channelFaces;
  size_t m_size;

};

} // namespace face
} // namespace nfd

#endif