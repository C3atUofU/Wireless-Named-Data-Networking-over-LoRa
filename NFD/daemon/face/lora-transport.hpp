/**
 * Header file used for creating the Lo-Ra transmission, which
 * will be used as another medium for NDN transmission
 */


#ifndef NFD_DAEMON_FACE_LORA_TRANSPORT_HPP
#define NFD_DAEMON_FACE_LORA_TRANSPORT_HPP

#include "transport.hpp"
#include <ndn-cxx/net/network-interface.hpp>
#include <string>
#include <pthread.h>
#include <queue>
#include "pcap-helper.hpp"
#include <fstream>
#include <unordered_set>

namespace nfd
{
namespace face
{

class LoRaTransport : public Transport
{
    class Error : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
   * @brief Processes the payload of an incoming frame
   */
    void
    receivePayload();

protected:

    void
    doClose() final;

private:
    void
    doSend(const Block &packet, const EndpointId& endpoint) final;

    void
    handleError(const std::string &errorMessage);

// Variables
private:
    // Flag used for reading in a certain network topology
    bool readTopology = false;
    std::string topologyFilename = "/home/pi/NDN/lora-configs/daisy-chain.topology";

    // ID and connections used for implementing a network topology
    std::pair<uint8_t, uint8_t> idAndSendAddr;
    std::unordered_set<uint8_t> send = std::unordered_set<uint8_t>();
    std::unordered_set<uint8_t> recv = std::unordered_set<uint8_t>();

    // Block to store stuff in
    std::queue<std::pair<std::pair<uint8_t, uint8_t>*, ndn::encoding::EncodingBuffer *>*>* sendBufferQueue;
    pthread_mutex_t* threadLock;

public:
    LoRaTransport(  std::pair<uint8_t, uint8_t> ids,
                    std::queue<std::pair<std::pair<uint8_t, uint8_t>*, ndn::encoding::EncodingBuffer *>*>* packetQueue, 
                    pthread_mutex_t* queueMutex);

    void
    receiveData(ndn::Block data);

};

} // namespace face
} // namespace nfd

#endif
