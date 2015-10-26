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

#include "ndn-consumer-realtime-push.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
// #include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"

#include <boost/ref.hpp>

#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerP");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ConsumerP);

TypeId
ConsumerP::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerP")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ConsumerP> ()
    .AddAttribute ("StartSeq", "Initial sequence number",
                   IntegerValue (0),
                   MakeIntegerAccessor(&ConsumerP::m_seq),
                   MakeIntegerChecker<int32_t>())

    .AddAttribute ("Prefix","Name of the Interest",
                   StringValue ("/"),
                   MakeNameAccessor (&ConsumerP::m_interestName),
                   MakeNameChecker ())
    .AddAttribute ("LifeTime", "LifeTime for interest packet",
                   StringValue ("2s"),
                   MakeTimeAccessor (&ConsumerP::m_interestLifeTime),
                   MakeTimeChecker ())

    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ConsumerP::m_frequency),
                   MakeDoubleChecker<double> ())

    .AddTraceSource ("PacketRecord", "Record data send and receive in file",
                     MakeTraceSourceAccessor (&ConsumerP::m_PacketRecord))
    ;

  return tid;
}

ConsumerP::ConsumerP ()
  : m_rand (0, std::numeric_limits<uint32_t>::max ())
  , m_seq (0)
  , m_frequency (1.0)
  , m_firstTime (true)
{
  NS_LOG_FUNCTION_NOARGS ();
}

// Application Methods
void
ConsumerP::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();

  // do base stuff
  App::StartApplication ();

  ScheduleNextPacket ();
}

void
ConsumerP::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS ();

  // cancel periodic packet generation
  Simulator::Cancel (m_sendEvent);

  // cleanup base stuff
  App::StopApplication ();
}

void
ConsumerP::SendSubscribePacket ()
{
  if (!m_active) return;

  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Name> name = Create<Name> (m_interestName);
 
  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (name);
  interest->SetInterestLifetime    (m_interestLifeTime);
  interest->SetPushTag             (Interest::PUSH_SUB_INTEREST);
  interest->SetPushSeq             (m_seq);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO ("> Subscribe Interest ");

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);

  // std::cout << "[consumer]:Subscribe!" << std::endl;

  if (interest->GetPushTag() == Interest::PUSH_SUB_INTEREST) {
    std::cout << "[consumer]send subscribe interest, now seq = " << interest->GetPushSeq() << std::endl;
  } else if (interest->GetPushTag() == Interest::PULL_INTEREST) {
    std::cout << "[consumer]send pull interest" << std::endl;
  }

  // record in file
  m_PacketRecord(this, name->toUri(), m_seq, "C_Sub_Interest", 0, 
                 0, 0, m_interestLifeTime);

  ScheduleNextPacket ();
}

void
ConsumerP::SendPacket (const uint32_t &seq)
{
  if (!m_active) return;

  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Name> nameWithSequence = Create<Name> (m_interestName);
  nameWithSequence->appendSeqNum (seq);

  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (nameWithSequence);
  interest->SetInterestLifetime    (m_interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO ("> Interest for " << seq);

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);

  std::cout << "[consumer]request: " << seq << std::endl;

  // record in file
  m_PacketRecord(this, nameWithSequence->toUri(), seq, "C_Pull_Interest", 0, 
                 0, 0, m_interestLifeTime);

}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////


void
ConsumerP::OnData (Ptr<const Data> data)
{
  if (!m_active) return;

  App::OnData (data); // tracing inside

  NS_LOG_FUNCTION (this << data);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  uint32_t seq;
  
  if (data->GetPushTag() == Data::PUSH_SUB_ACK) {
    seq = data->GetPushSeq();
    std::cout << "[consumer]recive sub ack, producer seq = " << seq << std::endl;
    m_PacketRecord(this, data->GetName().toUri(), seq, "C_Push_Ack", 0, 
                 0, 0, m_interestLifeTime);
    CheckGetLostData(seq+1);

  } else {
    seq = data->GetName ().get (-1).toSeqNum ();
    if (data->GetPushTag() == Data::PUSH_DATA) {
      std::cout << "[consumer]recive push data: " << seq << std::endl;
    } else if (data->GetPushTag() == Data::PULL_DATA) {
      std::cout << "[consumer]recive pull data: " << seq << std::endl;
    }
    m_PacketRecord(this, data->GetName().toUri(), seq, "C_Data", 0, 
                 0, 0, m_interestLifeTime);
    CheckGetLostData(seq);
    m_seq = seq;
  }

  NS_LOG_INFO ("< DATA for " << seq);

}

void
ConsumerP::OnNack (Ptr<const Interest> interest)
{
  if (!m_active) return;

  App::OnNack (interest); // tracing inside

  m_PacketRecord(this, interest->GetName().toUri(), 0, "C_NACK", 0, 
                 0, 0, m_interestLifeTime);

  ScheduleNextPacket ();
}

void
ConsumerP::ScheduleNextPacket ()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime)
    {
      m_sendEvent = Simulator::Schedule (Seconds (0.0),
                                         &ConsumerP::SendSubscribePacket, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning ())
    m_sendEvent = Simulator::Schedule (Seconds(1.0 / m_frequency),
                                       &ConsumerP::SendSubscribePacket, this);
}

void
ConsumerP::CheckGetLostData (const uint32_t &seq)
{
  if (seq - m_seq > 1)
  {
    for (uint32_t i=m_seq+1; i<seq; ++i)
      SendPacket(i);
  }
}


} // namespace ndn
} // namespace ns3
