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

#ifndef NDN_PRODUCER_D_H
#define NDN_PRODUCER_D_H

#include "ndn-app.h"

#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"

#include "ns3/random-variable.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink applia simple Interest-sink application,
 * which replying every incoming Interest with Data packet with a specified
 * size and name same as in Interest.cation, which replying every incoming Interest
 * with Data packet with a specified size and name same as in Interest.
 */
class ProducerD : public App
{
public:
  static TypeId
  GetTypeId (void);

  ProducerD ();
  virtual ~ProducerD ();

  // inherited from NdnApp
  void OnInterest (Ptr<const Interest> interest);

  void OnDetectInterest(Name & name);

  // senddata with seq
  void
  SendData(const uint32_t &seq);

protected:
  // inherited from Application base class.

  uint32_t        m_seq;  // currently generated sequence number
  uint32_t        m_seqMax;    // maximum number of sequence number
  double m_frequency;  // frequency of data packet gererating in 1 second
  RandomVariable *m_random;  // random
  std::string m_randomType; //  random type: uniform or exponential
  EventId m_generateEvent; // EventId of generate data event

  // added to record information by using tracers
  TracedCallback<Ptr<App> , std::string , uint32_t , std::string , uint32_t , int32_t , int32_t , Time>
    m_PacketRecord;

  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop

  // generate data
  virtual void
  GenerateData();

  // schedule next generate
  virtual void
  ScheduleNextData();

  /**
   * Set type of frequency randomization
   * value Either 'none', 'uniform', or 'exponential'
   */
  void
  SetRandomize (const std::string &value);

  /**
   * Get type of frequency randomization
   * either 'none', 'uniform', or 'exponential'
   */
  std::string
  GetRandomize () const;

private:
  Name m_prefix;
  // Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  uint32_t c_seq;

  uint32_t m_signature;
  Name m_keyLocator;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_PRODUCER_D_H
