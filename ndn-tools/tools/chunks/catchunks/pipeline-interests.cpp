/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2019, Regents of the University of California,
 *                          Colorado State University,
 *                          University Pierre & Marie Curie, Sorbonne University.
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
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 * @author Davide Pesavento
 * @author Weiwei Liu
 * @author Chavoosh Ghasemi
 */

#include "pipeline-interests.hpp"

namespace ndn {
namespace chunks {

PipelineInterests::PipelineInterests(Face& face)
  : m_face(face)
  , m_hasFinalBlockId(false)
  , m_lastSegmentNo(0)
  , m_nReceived(0)
  , m_receivedSize(0)
  , m_nextSegmentNo(0)
  , m_isStopping(false)
{
}

PipelineInterests::~PipelineInterests() = default;

void
PipelineInterests::run(const Name& versionedName, DataCallback dataCb, FailureCallback failureCb)
{
  BOOST_ASSERT(!versionedName.empty() && versionedName[-1].isVersion());
  BOOST_ASSERT(dataCb != nullptr);
  m_prefix = versionedName;
  m_onData = std::move(dataCb);
  m_onFailure = std::move(failureCb);

  // record the start time of the pipeline
  m_startTime = time::steady_clock::now();

  doRun();
}

void
PipelineInterests::cancel()
{
  if (m_isStopping)
    return;

  m_isStopping = true;
  doCancel();
}

bool
PipelineInterests::allSegmentsReceived() const
{
  return m_nReceived > 0 &&
         m_hasFinalBlockId &&
         static_cast<uint64_t>(m_nReceived - 1) >= m_lastSegmentNo;
}

uint64_t
PipelineInterests::getNextSegmentNo()
{
  return m_nextSegmentNo++;
}

void
PipelineInterests::onData(const Data& data)
{
  m_nReceived++;
  m_receivedSize += data.getContent().value_size();

  m_onData(data);
}

void
PipelineInterests::onFailure(const std::string& reason)
{
  if (m_isStopping)
    return;

  cancel();

  if (m_onFailure)
    m_face.getIoService().post([this, reason] { m_onFailure(reason); });
}

std::string
PipelineInterests::formatThroughput(double throughput)
{
  int pow = 0;
  while (throughput >= 1000.0 && pow < 4) {
    throughput /= 1000.0;
    pow++;
  }
  switch (pow) {
    case 0:
      return to_string(throughput) + " bit/s";
    case 1:
      return to_string(throughput) + " kbit/s";
    case 2:
      return to_string(throughput) + " Mbit/s";
    case 3:
      return to_string(throughput) + " Gbit/s";
    case 4:
      return to_string(throughput) + " Tbit/s";
  }
  return "";
}

void
PipelineInterests::printSummary() const
{
  using namespace ndn::time;
  duration<double, seconds::period> timeElapsed = steady_clock::now() - getStartTime();
  double throughput = 8 * m_receivedSize / timeElapsed.count();

  std::cerr << "\n\nAll segments have been received.\n"
            << "Time elapsed: " << timeElapsed << "\n"
            << "Segments received: " << m_nReceived << "\n"
            << "Transferred size: " << m_receivedSize / 1e3 << " kB" << "\n"
            << "Goodput: " << formatThroughput(throughput) << "\n";
}

} // namespace chunks
} // namespace ndn
