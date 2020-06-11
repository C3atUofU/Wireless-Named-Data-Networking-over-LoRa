#ifndef NFD_DAEMON_FACE_LORA_FACTORY_HPP
#define NFD_DAEMON_FACE_LORA_FACTORY_HPP

#include "protocol-factory.hpp"
#include "lora-channel.hpp"
#include "../../lora_libs/libraries/arduPiLoRa/arduPiLoRa.h"

namespace nfd {
namespace face {

class LoRaFactory : public ProtocolFactory
{
public:
  class Error : public ProtocolFactory::Error
  {
  public:
    using ProtocolFactory::Error::Error;
  };

  static const std::string&
  getId() noexcept;

  explicit
  LoRaFactory(const CtorParams& params);

    /*
    * @brief Helper funnction used for passing this along with pthread to spawn a thread. Otherwise compilation errors
    * https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
    * */
    static void *transmit_and_receive_helper(void *context)
    {
        return ((LoRaFactory *)context)->transmit_and_recieve();
    }

  using ProtocolFactory::ProtocolFactory;

  /**
   * \brief Create LoRa-based channel
   *
   * If this method is called twice the second time it won't do anything
   * because the lora will have been setup already
   *
   * \return always a valid pointer to a LoRaChannel object, an exception
   *         is thrown if it cannot be created.
   * \throw LoRaFactory::Error
   */
  std::shared_ptr<LoRaChannel>
  createChannel(std::string URI);

    /**
   * \brief Create LoRa-based multi-cast (broadcast) channel
   *
   *
   * \return always a valid pointer to a LoRaChannel object, an exception
   *         is thrown if it cannot be created.
   * \throw LoRaFactory::Error
   */
  std::shared_ptr<LoRaChannel>
  createMultiCastChannel(std::string URI);


private:
  /** \brief process face_system.udp config section
   */
  void
  doProcessConfig(OptionalConfigSection configSection,
                  FaceSystem::ConfigContext& context) override;

  void
  doCreateFace(const CreateFaceRequest& req,
               const FaceCreatedCallback& onCreated,
               const FaceCreationFailedCallback& onFailure) override;

  std::vector<std::shared_ptr<const Channel>>
  doGetChannels() const override;

  /**
   * @brief Sends the specified TLV block on the network wrapped in an Ethernet frame
   */
  void
  sendPacket();
  
  /**
   * Handle incoming data received on the lora module 
  */
  void
  handleRead();

private:
  // scheme is lora://
  const int numberOfCharsInScheme = 7;

  // Map for storing all the unicast channels
  std::map<std::string, std::shared_ptr<LoRaChannel>> m_channels;

  // Map for storing all the multicast channels
  std::map<std::string, std::shared_ptr<LoRaChannel>> mcast_channels;

  void
  setup();

  // Spawn transmit an receive threads used for lora
  void
  *transmit_and_recieve();

  // Used for lora
  int e;
  std::string m;
  char my_packet[2048];

  // Creating mutexes for shared queue and conditions for when data is produced from console
  pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER; 

  // Queue used to send messages out through LoRa
  std::queue<std::pair<std::pair<uint8_t, uint8_t>*, ndn::encoding::EncodingBuffer *>*> sendBufferQueue;
  ndn::encoding::EncodingBuffer *sendBuffer;

};

}
}

#endif // NFD_DAEMON_FACE_LORA_FACTORY_HPP
