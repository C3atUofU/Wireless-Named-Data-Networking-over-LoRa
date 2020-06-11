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
 * This file is part of ndn-tools (Named Data Networking Essential Tools).
 * See AUTHORS.md for complete list of ndn-tools authors and contributors.
 *
 * ndn-tools is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndn-tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndn-tools, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 * @author Zhuo Li <zhuoli@email.arizona.edu>
 * @author Davide Pesavento <davidepesa@gmail.com>
 */

#include "ndnpeek.hpp"

namespace ndn {
namespace peek {

NdnPeek::NdnPeek(Face& face, const PeekOptions& options)
  : m_options(options)
  , m_face(face)
  , m_scheduler(m_face.getIoService())
{
}

void
NdnPeek::start()
{
  m_pendingInterest = m_face.expressInterest(createInterest(),
                                             [this] (auto&&, const auto& data) { this->onData(data); },
                                             [this] (auto&&, const auto& nack) { this->onNack(nack); },
                                             [this] (auto&&) { this->onTimeout(); });

  if (m_options.timeout) {
    m_timeoutEvent = m_scheduler.schedule(*m_options.timeout, [this] {
      m_pendingInterest.cancel();
      onTimeout();
    });
  }

  m_sendTime = time::steady_clock::now();
}

Interest
NdnPeek::createInterest() const
{
  Interest interest(m_options.name);
  interest.setCanBePrefix(m_options.canBePrefix);
  interest.setMustBeFresh(m_options.mustBeFresh);
  if (m_options.link) {
    interest.setForwardingHint(m_options.link->getDelegationList());
  }
  interest.setInterestLifetime(m_options.interestLifetime);
  interest.setHopLimit(m_options.hopLimit);
  if (m_options.applicationParameters) {
    interest.setApplicationParameters(m_options.applicationParameters);
  }

  if (m_options.isVerbose) {
    std::cerr << "INTEREST: " << interest << std::endl;
  }

  return interest;
}

void
NdnPeek::onData(const Data& data)
{
  m_result = Result::DATA;
  m_timeoutEvent.cancel();

  if (m_options.isVerbose) {
    std::cerr << "DATA: " << data.getName() << "\nRTT: "
              << time::duration_cast<time::milliseconds>(time::steady_clock::now() - m_sendTime).count()
              << " ms" << std::endl;
  }

  if (m_options.wantPayloadOnly) {
    const Block& block = data.getContent();
    std::cout.write(reinterpret_cast<const char*>(block.value()), block.value_size());
  }
  else {
    const Block& block = data.wireEncode();
    std::cout.write(reinterpret_cast<const char*>(block.wire()), block.size());
  }
}

void
NdnPeek::onNack(const lp::Nack& nack)
{
  m_result = Result::NACK;
  m_timeoutEvent.cancel();

  lp::NackHeader header = nack.getHeader();
  if (m_options.isVerbose) {
    std::cerr << "NACK: " << header.getReason() << "\nRTT: "
              << time::duration_cast<time::milliseconds>(time::steady_clock::now() - m_sendTime).count()
              << " ms" << std::endl;
  }

  if (m_options.wantPayloadOnly) {
    std::cout << header.getReason() << std::endl;
  }
  else {
    const Block& block = header.wireEncode();
    std::cout.write(reinterpret_cast<const char*>(block.wire()), block.size());
  }
}

void
NdnPeek::onTimeout()
{
  m_result = Result::TIMEOUT;
  m_timeoutEvent.cancel();

  if (m_options.isVerbose) {
    std::cerr << "TIMEOUT" << std::endl;
  }
}

} // namespace peek
} // namespace ndn
