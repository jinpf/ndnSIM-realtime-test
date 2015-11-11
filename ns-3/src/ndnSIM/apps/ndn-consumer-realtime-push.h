/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jin Pengfei <jinpengfei@cstnet.cn>
 */

#ifndef NDN_CONSUMER_PUSH_H
#define NDN_CONSUMER_PUSH_H

#include "ndn-app.h"
#include "ns3/random-variable.h"
#include "ns3/ndn-name.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include <set>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief NDN application for sending out Interest packets
 */
class ConsumerP: public App
{
public:
  static TypeId GetTypeId ();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  ConsumerP ();
  virtual ~ConsumerP () {};

  // From App
  // virtual void
  // OnInterest (const Ptr<const Interest> &interest);

  virtual void
  OnNack (Ptr<const Interest> interest);

  virtual void
  OnData (Ptr<const Data> contentObject);

  /**
   * @brief Actually send packet
   */
  void
  SendSubscribePacket ();

  void
  SendPacket (const uint32_t &seq);
  
protected:
  // from App
  virtual void
  StartApplication ();

  virtual void
  StopApplication ();

  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN protocol
   */
  virtual void
  ScheduleNextPacket ();

  void
  Timeout (uint32_t seq);

  /**
   * \brief Modifies the frequency of checking the retransmission timeouts
   * \param retxTimer Timeout defining how frequent retransmission timeouts should be checked
   */
  void
  SetRetxTimer (Time retxTimer);

  /**
   * \brief Returns the frequency of checking the retransmission timeouts
   * \return Timeout defining how frequent retransmission timeouts should be checked
   */
  Time
  GetRetxTimer () const;

  void
  SetRetxProcess (uint32_t seq);

protected:
  UniformVariable m_rand; ///< @brief nonce generator

  uint32_t        m_seq;  ///< @brief currently requested sequence number
  EventId         m_sendEvent; ///< @brief EventId of pending "send packet" event
  double          m_frequency; // Frequency of interest packets (in hertz)
  bool            m_firstTime;

  Time            m_retxTimer; ///< @brief Currently estimated retransmission timer
  Name            m_interestName;        ///< \brief NDN Name of the Interest (use Name)
  Time            m_interestLifeTime;    ///< \brief LifeTime for interest packet

  // added to record information by using tracers
  TracedCallback<Ptr<App> , std::string , uint32_t , std::string , uint32_t , int32_t , int32_t , Time>
    m_PacketRecord;

private:

  struct Seq_Info
  {
    Time       start_time;  // seq first send time
    Time       last_retx;   // seq last send time
    uint32_t   retx_count;  // retransmit time
    EventId    retxEvent;
  };

  std::map <uint32_t,Seq_Info> In_flight_Seq; // record in windows seq

};

} // namespace ndn
} // namespace ns3

#endif
