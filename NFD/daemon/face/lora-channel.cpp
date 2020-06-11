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

#include "face.hpp"
#include "generic-link-service.hpp"
#include "common/global.hpp"
#include "lora-transport.hpp"
#include "lora-channel.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT(LoRaChannel);

LoRaChannel::LoRaChannel(std::string URI){
  setUri(ndn::FaceUri(URI));
  NFD_LOG_CHAN_INFO("Creating channel");
}


void
LoRaChannel::createFace(std::queue<std::pair<std::pair<uint8_t, uint8_t>*, ndn::encoding::EncodingBuffer *>*>* sendBufferQueue,
                        pthread_mutex_t* queueMutex,
                        std::pair<uint8_t, uint8_t> ids,
                        const FaceParams& params,
                        const FaceCreatedCallback& onFaceCreated,
                        const FaceCreationFailedCallback& onFailure)
{
  try
  {
    NFD_LOG_CHAN_INFO("Creating face");
    std::shared_ptr<Face> face;
    // Create the link service (we want to include fragmentation)
    GenericLinkService::Options options;
    options.allowFragmentation = true;
    options.allowReassembly = true;
    options.reliabilityOptions.isEnabled = params.wantLpReliability;
    auto linkService = make_unique<GenericLinkService>(options);

    // Create the transport alyer associated with this channel
    auto transport = make_unique<LoRaTransport>(ids, sendBufferQueue, queueMutex);

    // Create the face with this link service and transport layer (default face since each
    // channel will just have 1 face, due to their only being 1 protocol for LoRa)
    face = make_shared<Face>(std::move(linkService), std::move(transport));
    m_channelFaces["default"] = face;
    // Created successfully
    onFaceCreated(face);
  }
  catch(const std::exception& e)
  {
    std::string failMsg = "Unable to create transport and link service: ";
    failMsg += e.what();
    onFailure(504, failMsg);
  }
}

void
LoRaChannel::handleReceive(ndn::Block data){
  auto it = m_channelFaces.find("default");   // Change this if there multiple faces to a channel for lora
  static_cast<LoRaTransport*>(it->second->getTransport())->receiveData(data);
}

}
}
