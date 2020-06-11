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
 * @author Andrea Tosatto
 */

#include "tools/chunks/catchunks/consumer.hpp"
#include "tools/chunks/catchunks/discover-version.hpp"
#include "tools/chunks/catchunks/pipeline-interests.hpp"

#include "tests/test-common.hpp"

#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace chunks {
namespace tests {

using namespace ndn::tests;
using boost::test_tools::output_test_stream;

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_AUTO_TEST_SUITE(TestConsumer)

BOOST_AUTO_TEST_CASE(OutputDataSequential)
{
  // Test sequential segments in the right order
  // Segment order: 0 1 2

  std::string name("/ndn/chunks/test");

  std::vector<std::string> testStrings {
      "",

      "a1b2c3%^&(#$&%^$$/><",

      "123456789123456789123456789123456789123456789123456789123456789"
      "123456789123456789123456789123456789123456789123456789123456789",

      "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. "
      "Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur "
      "ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla "
      "consequat massa Donec pede justo,"
  };

  util::DummyClientFace face;
  output_test_stream output("");
  Consumer cons(security::v2::getAcceptAllValidator(), output);

  auto interest = makeInterest(name, true);

  for (size_t i = 0; i < testStrings.size(); ++i) {
    output.flush();

    auto data = makeData(Name(name).appendVersion(1).appendSegment(i));
    data->setContent(reinterpret_cast<const uint8_t*>(testStrings[i].data()),
                     testStrings[i].size());

    cons.m_bufferedData[i] = data;
    cons.writeInOrderData();

    BOOST_CHECK(output.is_equal(testStrings[i]));
  }
}

BOOST_AUTO_TEST_CASE(OutputDataUnordered)
{
  // Test unordered segments
  // Segment order: 1 0 2

  std::string name("/ndn/chunks/test");

  std::vector<std::string> testStrings {
      "a1b2c3%^&(#$&%^$$/><",

      "123456789123456789123456789123456789123456789123456789123456789"
      "123456789123456789123456789123456789123456789123456789123456789",

      "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. "
      "Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur "
      "ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla "
      "consequat massa Donec pede justo,"
  };;

  util::DummyClientFace face;
  output_test_stream output("");
  Consumer cons(security::v2::getAcceptAllValidator(), output);

  auto interest = makeInterest(name, true);
  std::vector<shared_ptr<Data>> dataStore;

  for (size_t i = 0; i < testStrings.size(); ++i) {
    auto data = makeData(Name(name).appendVersion(1).appendSegment(i));
    data->setContent(reinterpret_cast<const uint8_t*>(testStrings[i].data()),
                     testStrings[i].size());

    dataStore.push_back(data);
  }

  output.flush();
  cons.m_bufferedData[1] = dataStore[1];
  cons.writeInOrderData();
  BOOST_CHECK(output.is_equal(""));

  output.flush();
  cons.m_bufferedData[0] = dataStore[0];
  cons.writeInOrderData();
  BOOST_CHECK(output.is_equal(testStrings[0] + testStrings[1]));

  output.flush();
  cons.m_bufferedData[2] = dataStore[2];
  cons.writeInOrderData();
  BOOST_CHECK(output.is_equal(testStrings[2]));
}

class PipelineInterestsDummy : public PipelineInterests
{
public:
  PipelineInterestsDummy(Face& face)
    : PipelineInterests(face)
    , isPipelineRunning(false)
  {
  }

private:
  void
  doRun() final
  {
    isPipelineRunning = true;
  }

  void
  doCancel() final
  {
  }

public:
  bool isPipelineRunning;
};

BOOST_FIXTURE_TEST_CASE(RunBasic, UnitTestTimeFixture)
{
  boost::asio::io_service io;
  util::DummyClientFace face(io);
  Consumer consumer(security::v2::getAcceptAllValidator());

  Name prefix = Name("/ndn/chunks/test").appendVersion(1);
  auto discover = make_unique<DiscoverVersion>(prefix, face, Options());
  auto pipeline = make_unique<PipelineInterestsDummy>(face);
  auto pipelinePtr = pipeline.get();

  consumer.run(std::move(discover), std::move(pipeline));

  this->advanceClocks(io, time::nanoseconds(1));
  // no discovery interest is issued
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 0);

  // this Data packet answers the discovery Interest, so it must end with a version number
  auto data = makeData(prefix.appendVersion(0));
  face.receive(*data);

  this->advanceClocks(io, time::nanoseconds(1));
  BOOST_CHECK_EQUAL(pipelinePtr->isPipelineRunning, true);
}

BOOST_AUTO_TEST_SUITE_END() // TestConsumer
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace chunks
} // namespace ndn
