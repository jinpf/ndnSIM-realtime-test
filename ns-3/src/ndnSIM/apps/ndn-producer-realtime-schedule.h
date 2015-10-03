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

#ifndef NDN_PRODUCER_SCH_H
#define NDN_PRODUCER_SCH_H

#include "ndn-app.h"

#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"
#include "ns3/nstime.h"

#include <map>

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
class ProducerS : public App
{
public:
  static TypeId
  GetTypeId (void);

  ProducerS ();
  virtual ~ProducerS ();

  // inherited from NdnApp
  void OnInterest (Ptr<const Interest> interest);

  // senddata with seq
  void
  SendData(const uint32_t &seq, bool subscribe);

protected:
  // inherited from Application base class.

  uint32_t        m_seq;  // currently generated sequence number
  uint32_t        m_seqMax;    // maximum number of sequence number
  bool            m_subscribe;
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

private:
  Name m_prefix;
  // Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;
  std::string m_filename;

  std::map <Time,uint32_t> m_schedule_data;
  // 'm_time_value_it' is pair pointer, it->first: key, it->second: value
  std::map <Time,uint32_t>::iterator m_time_value_it;
  
};

} // namespace ndn
} // namespace ns3

#endif // NDN_PRODUCER_SCH_H
