/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019, University of Memphis,
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
 */

#include "tools/dump/ndndump.hpp"

#include "tests/identity-management-fixture.hpp"
#include "tests/test-common.hpp"

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <boost/endian/conversion.hpp>

#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/net/ethernet.hpp>

namespace ndn {
namespace dump {
namespace tests {

namespace endian = boost::endian;
using namespace ndn::tests;

class StdCoutRedirector
{
public:
  StdCoutRedirector(std::ostream& os)
  {
    // Redirect std::cout to the specified output stream
    originalBuffer = std::cout.rdbuf(os.rdbuf());
  }

  ~StdCoutRedirector()
  {
    // Revert state for std::cout
    std::cout.rdbuf(originalBuffer);
  }

private:
  std::streambuf* originalBuffer;
};

class NdnDumpFixture : public IdentityManagementFixture
{
protected:
  NdnDumpFixture()
  {
    dump.m_dataLinkType = DLT_EN10MB;
  }

  template<typename Packet>
  void
  receive(const Packet& packet)
  {
    EncodingBuffer buffer(packet.wireEncode());
    receiveEthernet(buffer);
  }

  void
  receiveEthernet(EncodingBuffer& buffer, uint16_t ethertype = s_ethertypeNdn)
  {
    ethernet::Address host;

    // Ethernet header
    buffer.prependByteArray(reinterpret_cast<const uint8_t*>(&ethertype), ethernet::TYPE_LEN);
    buffer.prependByteArray(host.data(), host.size());
    buffer.prependByteArray(host.data(), host.size());

    // pcap header
    pcap_pkthdr pkthdr{};
    pkthdr.caplen = pkthdr.len = buffer.size();

    {
      StdCoutRedirector redirect(output);
      dump.printPacket(&pkthdr, buffer.buf());
    }
  }

  void
  receiveIp4(EncodingBuffer& buffer, const ip* ipHeader)
  {
    buffer.prependByteArray(reinterpret_cast<const uint8_t*>(ipHeader), sizeof(ip));
    receiveEthernet(buffer, s_ethertypeIp4);
  }

  void
  receiveIp6(EncodingBuffer& buffer, const ip6_hdr* ip6Header)
  {
    buffer.prependByteArray(reinterpret_cast<const uint8_t*>(ip6Header), sizeof(ip6_hdr));
    receiveEthernet(buffer, s_ethertypeIp6);
  }

  void
  receiveTcp4(EncodingBuffer& buffer, const tcphdr* tcpHeader)
  {
    buffer.prependByteArray(reinterpret_cast<const uint8_t*>(tcpHeader), sizeof(tcphdr));

    ip ipHeader{};
    ipHeader.ip_v = 4;
    ipHeader.ip_hl = 5;
    ipHeader.ip_len = endian::native_to_big(static_cast<uint16_t>(buffer.size() + sizeof(ipHeader)));
    ipHeader.ip_ttl = 1;
    ipHeader.ip_p = IPPROTO_TCP;

    receiveIp4(buffer, &ipHeader);
  }

  void
  receiveUdp4(EncodingBuffer& buffer, const udphdr* udpHeader)
  {
    buffer.prependByteArray(reinterpret_cast<const uint8_t*>(udpHeader), sizeof(udphdr));

    ip ipHeader{};
    ipHeader.ip_v = 4;
    ipHeader.ip_hl = 5;
    ipHeader.ip_len = endian::native_to_big(static_cast<uint16_t>(buffer.size() + sizeof(ipHeader)));
    ipHeader.ip_ttl = 1;
    ipHeader.ip_p = IPPROTO_UDP;

    receiveIp4(buffer, &ipHeader);
  }

  void
  readFile(const std::string& filename)
  {
    StdCoutRedirector redirect(output);
    dump.inputFile = filename;
    dump.run();
  }

protected:
  NdnDump dump;
  boost::test_tools::output_test_stream output;

  static const uint16_t s_ethertypeNdn;
  static const uint16_t s_ethertypeIp4;
  static const uint16_t s_ethertypeIp6;
};

const uint16_t NdnDumpFixture::s_ethertypeNdn = endian::native_to_big(ethernet::ETHERTYPE_NDN);
const uint16_t NdnDumpFixture::s_ethertypeIp4 = endian::native_to_big(uint16_t(ETHERTYPE_IP));
const uint16_t NdnDumpFixture::s_ethertypeIp6 = endian::native_to_big(uint16_t(ETHERTYPE_IPV6));

BOOST_AUTO_TEST_SUITE(Dump)
BOOST_FIXTURE_TEST_SUITE(TestNdnDump, NdnDumpFixture)

BOOST_AUTO_TEST_CASE(Interest)
{
  this->receive(*makeInterest("/test", true, DEFAULT_INTEREST_LIFETIME, 1));
  BOOST_CHECK(output.is_equal("0.000000 Ethernet, INTEREST: /test?ndn.Nonce=1\n"));
}

BOOST_AUTO_TEST_CASE(Data)
{
  this->receive(*makeData("/test"));
  BOOST_CHECK(output.is_equal("0.000000 Ethernet, DATA: /test\n"));
}

BOOST_AUTO_TEST_CASE(Nack)
{
  auto interest = makeInterest("/test", true, DEFAULT_INTEREST_LIFETIME, 1);
  auto nack = makeNack(*interest, lp::NackReason::DUPLICATE);
  lp::Packet lpPacket(interest->wireEncode());
  lpPacket.add<lp::NackField>(nack.getHeader());

  this->receive(lpPacket);
  BOOST_CHECK(output.is_equal("0.000000 Ethernet, NDNLPv2, NACK (Duplicate): /test?ndn.Nonce=1\n"));
}

BOOST_AUTO_TEST_CASE(LpFragment)
{
  const uint8_t data[10] = {
    0x06, 0x08, // Data packet
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  };

  Buffer buffer(data, 4);
  lp::Packet lpPacket;
  lpPacket.add<lp::FragmentField>(std::make_pair(buffer.begin(), buffer.end()));
  lpPacket.add<lp::FragIndexField>(0);
  lpPacket.add<lp::FragCountField>(2);
  lpPacket.add<lp::SequenceField>(1000);

  this->receive(lpPacket);
  BOOST_CHECK(output.is_equal("0.000000 Ethernet, NDNLPv2 fragment\n"));
}

BOOST_AUTO_TEST_CASE(LpIdle)
{
  lp::Packet lpPacket;
  this->receive(lpPacket);
  BOOST_CHECK(output.is_equal("0.000000 Ethernet, NDNLPv2 idle\n"));
}

BOOST_AUTO_TEST_CASE(IncompleteNdnPacket)
{
  const uint8_t interest[] = {
  0x05, 0x0E, // Interest
    0x07, 0x06, // Name
      0x08, 0x04, // NameComponent
        0x74, 0x65, 0x73, 0x74,
    0x0a, 0x04, // Nonce
      0x00, 0x00, 0x00, 0x01
  };
  EncodingBuffer buffer;
  buffer.prependByteArray(interest, 4);

  this->receiveEthernet(buffer);
  BOOST_CHECK(output.is_equal("0.000000 Ethernet, NDN truncated packet, length 4\n"));
}

BOOST_AUTO_TEST_CASE(UnsupportedNdnPacket)
{
  EncodingBuffer buffer(encoding::makeEmptyBlock(tlv::Name));
  this->receiveEthernet(buffer);
  BOOST_CHECK(output.is_equal("0.000000 Ethernet, [Unsupported NDN packet type 7]\n"));
}

BOOST_AUTO_TEST_CASE(UnsupportedEtherType)
{
  EncodingBuffer pkt;
  uint16_t type = ETHERTYPE_ARP;
  endian::native_to_big_inplace(type);

  this->receiveEthernet(pkt, type);
  BOOST_CHECK(output.is_equal("0.000000 [Unsupported ethertype 0x806]\n"));
}

BOOST_AUTO_TEST_CASE(MalformedIpv4Header)
{
  dump.wantTimestamp = false;

  uint8_t theAnswer = 42;

  EncodingBuffer pkt1;
  pkt1.prependByte(theAnswer);
  this->receiveEthernet(pkt1, s_ethertypeIp4);
  BOOST_CHECK(output.is_equal("IP truncated header, length 1\n"));

  ip ipHdr2{};
  ipHdr2.ip_v = 7;

  EncodingBuffer pkt2;
  this->receiveIp4(pkt2, &ipHdr2);
  BOOST_CHECK(output.is_equal("IP bad version 7\n"));

  ip ipHdr3{};
  ipHdr3.ip_v = 4;
  ipHdr3.ip_hl = 2;

  EncodingBuffer pkt3;
  this->receiveIp4(pkt3, &ipHdr3);
  BOOST_CHECK(output.is_equal("IP bad header length 8\n"));

  ip ipHdr4{};
  ipHdr4.ip_v = 4;
  ipHdr4.ip_hl = 5;
  ipHdr4.ip_len = 10;
  endian::native_to_big_inplace(ipHdr4.ip_len);

  EncodingBuffer pkt4;
  this->receiveIp4(pkt4, &ipHdr4);
  BOOST_CHECK(output.is_equal("IP bad length 10\n"));

  ip ipHdr5{};
  ipHdr5.ip_v = 4;
  ipHdr5.ip_hl = 5;
  ipHdr5.ip_len = 1000;
  endian::native_to_big_inplace(ipHdr5.ip_len);

  EncodingBuffer pkt5;
  this->receiveIp4(pkt5, &ipHdr5);
  BOOST_CHECK(output.is_equal("IP truncated packet, 980 bytes missing\n"));
}

BOOST_AUTO_TEST_CASE(MalformedIpv6Header)
{
  dump.wantTimestamp = false;

  uint8_t theAnswer = 42;

  EncodingBuffer pkt1;
  pkt1.prependByte(theAnswer);
  this->receiveEthernet(pkt1, s_ethertypeIp6);
  BOOST_CHECK(output.is_equal("IP6 truncated header, length 1\n"));

  ip6_hdr ip6Hdr2{};
  ip6Hdr2.ip6_vfc = 10 << 4;

  EncodingBuffer pkt2;
  this->receiveIp6(pkt2, &ip6Hdr2);
  BOOST_CHECK(output.is_equal("IP6 bad version 10\n"));

  ip6_hdr ip6Hdr3{};
  ip6Hdr3.ip6_vfc = 6 << 4;
  ip6Hdr3.ip6_plen = 1400;
  endian::native_to_big_inplace(ip6Hdr3.ip6_plen);

  EncodingBuffer pkt3;
  this->receiveIp6(pkt3, &ip6Hdr3);
  BOOST_CHECK(output.is_equal("IP6 truncated payload, 1400 bytes missing\n"));
}

BOOST_AUTO_TEST_CASE(UnsupportedIpProto)
{
  dump.wantTimestamp = false;

  ip ipHdr{};
  ipHdr.ip_v = 4;
  ipHdr.ip_hl = 5;
  ipHdr.ip_len = sizeof(ipHdr);
  endian::native_to_big_inplace(ipHdr.ip_len);
  ipHdr.ip_p = IPPROTO_SCTP;

  EncodingBuffer pkt1;
  this->receiveIp4(pkt1, &ipHdr);
  BOOST_CHECK(output.is_equal("IP 0.0.0.0 > 0.0.0.0, [Unsupported IP proto 132]\n"));

  ip6_hdr ip6Hdr{};
  ip6Hdr.ip6_vfc = 6 << 4;
  ip6Hdr.ip6_nxt = IPPROTO_NONE;

  EncodingBuffer pkt2;
  this->receiveIp6(pkt2, &ip6Hdr);
  BOOST_CHECK(output.is_equal("IP6 :: > ::, [No next header]\n"));
}

BOOST_AUTO_TEST_CASE(MalformedTcpHeader)
{
  dump.wantTimestamp = false;

  tcphdr tcpHdr1{};
  tcpHdr1.TH_OFF = 0x2;

  EncodingBuffer pkt1;
  this->receiveTcp4(pkt1, &tcpHdr1);
  BOOST_CHECK(output.is_equal("IP 0.0.0.0 > 0.0.0.0, TCP bad header length 8\n"));

  tcphdr tcpHdr2{};
  tcpHdr2.TH_OFF = 0xf;

  EncodingBuffer pkt2;
  this->receiveTcp4(pkt2, &tcpHdr2);
  BOOST_CHECK(output.is_equal("IP 0.0.0.0 > 0.0.0.0, TCP truncated header, 40 bytes missing\n"));
}

BOOST_AUTO_TEST_CASE(MalformedUdpHeader)
{
  dump.wantTimestamp = false;

  udphdr udpHdr1{};
  udpHdr1.UH_LEN = 3;
  endian::native_to_big_inplace(udpHdr1.UH_LEN);

  EncodingBuffer pkt1;
  this->receiveUdp4(pkt1, &udpHdr1);
  BOOST_CHECK(output.is_equal("IP 0.0.0.0 > 0.0.0.0, UDP bad length 3\n"));

  udphdr udpHdr2{};
  udpHdr2.UH_LEN = 1000;
  endian::native_to_big_inplace(udpHdr2.UH_LEN);

  EncodingBuffer pkt2;
  this->receiveUdp4(pkt2, &udpHdr2);
  BOOST_CHECK(output.is_equal("IP 0.0.0.0 > 0.0.0.0, UDP truncated packet, 992 bytes missing\n"));
}

BOOST_AUTO_TEST_CASE(InvalidTlvLength)
{
  dump.wantTimestamp = false;
  this->readFile("tests/dump/invalid-tlv-length.pcap");

  const std::string expected =
    "IP 128.196.203.36 > 128.187.81.12, TCP, length 147, NDNLPv2 invalid packet: "
    "TLV-LENGTH of sub-element of type 5 exceeds TLV-VALUE boundary of parent block\n";
  BOOST_CHECK(output.is_equal(expected));
}

BOOST_AUTO_TEST_CASE(UnrecognizedLpField)
{
  dump.wantTimestamp = false;
  this->readFile("tests/dump/unrecognized-lp-field.pcap");

  const std::string expected = "IP 128.196.203.36 > 128.187.81.12, TCP, length 800, "
                               "NDNLPv2 invalid packet: unrecognized field 4 cannot be ignored\n";
  BOOST_CHECK(output.is_equal(expected));
}

BOOST_AUTO_TEST_CASE(NoTimestamp)
{
  dump.wantTimestamp = false;

  lp::Packet lpPacket;
  this->receive(lpPacket);

  BOOST_CHECK(output.is_equal("Ethernet, NDNLPv2 idle\n"));
}

BOOST_AUTO_TEST_CASE(FromFile)
{
  dump.pcapFilter = "";
  this->readFile("tests/dump/nack.pcap");

  const std::string expected =
    "1456768916.467099 IP 1.0.0.1 > 1.0.0.2, UDP, length 42, "
    "INTEREST: /producer/nack/congestion?ndn.MustBeFresh=1&ndn.Nonce=2581361680\n"
    "1456768916.567099 IP 1.0.0.1 > 1.0.0.2, UDP, length 41, "
    "INTEREST: /producer/nack/duplicate?ndn.MustBeFresh=1&ndn.Nonce=4138343109\n"
    "1456768916.667099 IP 1.0.0.1 > 1.0.0.2, UDP, length 41, "
    "INTEREST: /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=4034910304\n"
    "1456768916.767099 IP 1.0.0.2 > 1.0.0.1, UDP, length 55, "
    "NDNLPv2, NACK (Congestion): /producer/nack/congestion?ndn.MustBeFresh=1&ndn.Nonce=2581361680\n"
    "1456768916.867099 IP 1.0.0.2 > 1.0.0.1, UDP, length 54, "
    "NDNLPv2, NACK (Duplicate): /producer/nack/duplicate?ndn.MustBeFresh=1&ndn.Nonce=4138343109\n"
    "1456768916.967099 IP 1.0.0.2 > 1.0.0.1, UDP, length 54, "
    "NDNLPv2, NACK (None): /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=4034910304\n"
    "1456768917.067099 IP 1.0.0.1 > 1.0.0.2, TCP, length 42, "
    "INTEREST: /producer/nack/congestion?ndn.MustBeFresh=1&ndn.Nonce=3192497423\n"
    "1456768917.267099 IP 1.0.0.2 > 1.0.0.1, TCP, length 55, "
    "NDNLPv2, NACK (Congestion): /producer/nack/congestion?ndn.MustBeFresh=1&ndn.Nonce=3192497423\n"
    "1456768917.367099 IP 1.0.0.1 > 1.0.0.2, TCP, length 82, "
    "INTEREST: /producer/nack/duplicate?ndn.MustBeFresh=1&ndn.Nonce=522390724\n"
    "1456768917.567099 IP 1.0.0.2 > 1.0.0.1, TCP, length 54, "
    "NDNLPv2, NACK (Duplicate): /producer/nack/duplicate?ndn.MustBeFresh=1&ndn.Nonce=522390724\n"
    "1456768917.767099 IP 1.0.0.2 > 1.0.0.1, TCP, length 54, "
    "NDNLPv2, NACK (None): /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=2002441365\n"
    "1456768917.967099 IP 1.0.0.1 > 1.0.0.2, TCP, length 41, "
    "INTEREST: /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=3776824408\n"
    "1456768918.067099 IP 1.0.0.2 > 1.0.0.1, TCP, length 54, "
    "NDNLPv2, NACK (None): /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=3776824408\n";
  BOOST_CHECK(output.is_equal(expected));
}

BOOST_AUTO_TEST_CASE(LinuxSllTcp4)
{
  dump.wantTimestamp = false;
  this->readFile("tests/dump/linux-sll-tcp4.pcap");

  const std::string expected =
    "IP 162.211.64.84 > 131.179.196.46, TCP, length 57, "
    "INTEREST: /ndn/edu/ucla/ping/4436024701616433461?ndn.MustBeFresh=1&ndn.Nonce=1827520902\n"
    "IP 162.211.64.84 > 131.179.196.46, TCP, length 41, "
    "INTEREST: /ndn/edu/arizona/ping/8202?ndn.Nonce=1059849935\n"
    "IP 131.179.196.46 > 162.211.64.84, TCP, length 403, "
    "DATA: /ndn/edu/arizona/ping/8202\n"
    "IP 131.179.196.46 > 162.211.64.84, TCP, length 57, "
    "INTEREST: /ndn/edu/ucla/ping/4436024701616433462?ndn.MustBeFresh=1&ndn.Nonce=4082468009\n"
    "IP 162.211.64.84 > 131.179.196.46, TCP, length 57, "
    "INTEREST: /ndn/edu/ucla/ping/4436024701616433462?ndn.MustBeFresh=1&ndn.Nonce=4082468009\n";
  BOOST_CHECK(output.is_equal(expected));
}

BOOST_AUTO_TEST_CASE(LinuxSllUdp4)
{
  dump.wantTimestamp = false;
  this->readFile("tests/dump/linux-sll-udp4.pcap");

  const std::string expected =
    "IP 162.211.64.84 > 131.179.196.46, UDP, length 42, "
    "INTEREST: /ndn/edu/arizona/ping/31044?ndn.Nonce=3171630323\n"
    "IP 131.179.196.46 > 162.211.64.84, UDP, length 404, "
    "DATA: /ndn/edu/arizona/ping/31044\n";
  BOOST_CHECK(output.is_equal(expected));
}

BOOST_AUTO_TEST_CASE(LinuxSllTcp6)
{
  dump.wantTimestamp = false;
  this->readFile("tests/dump/linux-sll-tcp6.pcap");

  const std::string expected =
    "IP6 2602:fff6:d:b317::39f8 > 2001:660:3302:282c:160::163, TCP, length 42, "
    "INTEREST: /ndn/edu/arizona/ping/19573?ndn.Nonce=777756283\n"
    "IP6 2001:660:3302:282c:160::163 > 2602:fff6:d:b317::39f8, TCP, length 404, "
    "DATA: /ndn/edu/arizona/ping/19573\n"
    "IP6 2001:660:3302:282c:160::163 > 2602:fff6:d:b317::39f8, TCP, length 56, "
    "INTEREST: /ndn/fr/lip6/ping/7847878851635149046?ndn.MustBeFresh=1&ndn.Nonce=1836363210\n";
  BOOST_CHECK(output.is_equal(expected));
}

BOOST_AUTO_TEST_CASE(LinuxSllUdp6)
{
  dump.wantTimestamp = false;
  this->readFile("tests/dump/linux-sll-udp6.pcap");

  const std::string expected =
    "IP6 2602:fff6:d:b317::39f8 > 2001:660:3302:282c:160::163, UDP, length 39, "
    "INTEREST: /ndn/edu/arizona/ping/18?ndn.Nonce=571618686\n"
    "IP6 2001:660:3302:282c:160::163 > 2602:fff6:d:b317::39f8, UDP, length 401, "
    "DATA: /ndn/edu/arizona/ping/18\n"
    "IP6 2001:660:3302:282c:160::163 > 2602:fff6:d:b317::39f8, UDP, length 56, "
    "INTEREST: /ndn/fr/lip6/ping/7847878851635149038?ndn.MustBeFresh=1&ndn.Nonce=192371114\n";
  BOOST_CHECK(output.is_equal(expected));
}

BOOST_AUTO_TEST_SUITE_END() // TestNdnDump
BOOST_AUTO_TEST_SUITE_END() // Dump

} // namespace tests
} // namespace dump
} // namespace ndn
