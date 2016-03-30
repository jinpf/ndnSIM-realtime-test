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

#include "ndn-consumer-realtime-detect.h"
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
// #include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
// #include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"

#include <boost/ref.hpp>

#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerD");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ConsumerD);

TypeId
ConsumerD::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerD")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ConsumerD> ()

    .AddAttribute ("StartSeq", "Initial sequence number",
                   IntegerValue (0),
                   MakeIntegerAccessor(&ConsumerD::m_seq),
                   MakeIntegerChecker<int32_t>())

    .AddAttribute ("MaxSeq", "Max sequence number",
                   IntegerValue (1000),
                   MakeIntegerAccessor(&ConsumerD::m_seqMax),
                   MakeIntegerChecker<int32_t>())

    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ConsumerD::m_frequency),
                   MakeDoubleChecker<double> ())

    .AddAttribute ("Prefix","Name of the Interest",
                   StringValue ("/"),
                   MakeNameAccessor (&ConsumerD::m_interestName),
                   MakeNameChecker ())
    .AddAttribute ("LifeTime", "LifeTime for interest packet",
                   StringValue ("2s"),
                   MakeTimeAccessor (&ConsumerD::m_interestLifeTime),
                   MakeTimeChecker ())

    .AddAttribute ("RetxTimer",
                   "Timeout defining how frequent retransmission timeouts should be checked",
                   StringValue ("500ms"),
                   MakeTimeAccessor (&ConsumerD::GetRetxTimer, &ConsumerD::SetRetxTimer),
                   MakeTimeChecker ())

    .AddAttribute ("Window", "window size",
                   StringValue ("4"),
                   MakeIntegerAccessor (&ConsumerD::GetWindow, &ConsumerD::SetWindow),
                   MakeIntegerChecker<int32_t> ())

    .AddTraceSource ("PacketRecord", "Record data send and receive in file",
                     MakeTraceSourceAccessor (&ConsumerD::m_PacketRecord))

    ;

  return tid;
}

ConsumerD::ConsumerD ()
  : m_rand (0, std::numeric_limits<uint32_t>::max ())
  , m_firstTime (true)
  , m_seq (0)
{
  NS_LOG_FUNCTION_NOARGS ();

  p_seq_a = 0;
  p_seq_b = 0;
  p_latest_seq = 0;
  p_detect_time = Time(0);

  // m_rtt = CreateObject<RttMeanDeviation> ();
}

void
ConsumerD::SetRetxTimer (Time retxTimer)
{
  m_retxTimer = retxTimer;
}

Time
ConsumerD::GetRetxTimer () const
{
  return m_retxTimer;
}

void
ConsumerD::Timeout (uint32_t seq)
{
  // std::cout << "check time out!" << std::endl;
  // std::cout << "[consumer] " << seq << "wait" 
  //           << Simulator::Now () - In_flight_Seq[seq].last_retx
  //           << " time out!" << std::endl;

  SendPacket(seq);
}

void
ConsumerD::SetWindow (int32_t window)
{
  m_MaxWindow = window;
}

uint32_t
ConsumerD::GetWindow () const
{
  return m_MaxWindow;
}

// Application Methods
void
ConsumerD::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();

  // do base stuff
  App::StartApplication ();

  ScheduleNextDetect ();

  ScheduleNextPacket ();
}

void
ConsumerD::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS ();

  // cancel periodic packet generation
  Simulator::Cancel (m_sendEvent);

  std::map<uint32_t,Seq_Info>::iterator it = In_flight_Seq.begin();
  // 'it' is pair pointer, it->first: key, it->second: value
  while (it != In_flight_Seq.end()) {
    if (it->second.retxEvent.IsRunning()) {
      it->second.retxEvent.Cancel();
      Simulator::Remove(it->second.retxEvent);
      it++;
    } 
  }

  // cleanup base stuff
  App::StopApplication ();
}

void
ConsumerD::ScheduleNextDetect ()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime)
    {
      m_sendEvent = Simulator::Schedule (Seconds (0.0),
                                         &ConsumerD::SendDetect, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning () )
    m_sendEvent = Simulator::Schedule (Seconds(1.0 / m_frequency),
                                       &ConsumerD::SendDetect, this);
}

void
ConsumerD::ScheduleNextPacket ()
{
  if (!m_active) return;
  NS_LOG_FUNCTION_NOARGS ();

  while (m_seq < p_latest_seq + m_MaxWindow && m_seq < m_seqMax) {
    m_seq++;
    std::cout << "[consumer] p latest seq:" << p_latest_seq << " ";
    SendPacket(m_seq);
  }
}

void
ConsumerD::SendDetect()
{
  if (!m_active) return;

  NS_LOG_FUNCTION_NOARGS ();

  uint32_t tag = m_rand.GetValue ();

  Ptr<Name> nameWithTag = Create<Name> (m_interestName+Name("/detect"));
  nameWithTag->appendSeqNum (tag);
  //

  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (nameWithTag);
  interest->SetInterestLifetime    (m_interestLifeTime);
  std::cout << "[consumer] send detect packet:" << nameWithTag->get(-2).toUri() << std::endl;
  // if (nameWithTag->get(-2).toUri() == "detect")
  //   std::cout << "test right!" << std::endl;
  // else
  //   std::cout << "test wrong!" << std::endl;
  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);

  // record in m_detect_send_time
  m_detect_send_time[tag] = Simulator::Now();

  // record in file
  m_PacketRecord(this, nameWithTag->toUri(), p_latest_seq, "C_Detect_Interest", 0, 
                 m_MaxWindow, m_MaxWindow, m_interestLifeTime);

  ScheduleNextDetect();
}

void
ConsumerD::SendPacket (uint32_t seq)
{
  if (!m_active) return;

  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Name> nameWithSequence = Create<Name> (m_interestName);
  nameWithSequence->appendSeqNum (seq);
  //

  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (nameWithSequence);
  interest->SetInterestLifetime    (m_interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO ("> Interest for " << seq);

  std::cout << "[consumer] send " << seq << std::endl;

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);

  // record in In_flight_seq
  if (In_flight_Seq.find(seq) == In_flight_Seq.end()) {
    In_flight_Seq[seq].start_time = Simulator::Now();
    In_flight_Seq[seq].retx_count = 0;
  } else {
    In_flight_Seq[seq].retx_count ++ ;
  }

  // record in file
  m_PacketRecord(this, nameWithSequence->toUri(), seq, "C_Interest", In_flight_Seq[seq].retx_count, 
                 m_MaxWindow, m_MaxWindow, m_interestLifeTime);

  SetRetxProcess (seq);

}

void
ConsumerD::SetRetxProcess (uint32_t seq)
{
  // NS_LOG_DEBUG ("Trying to add " << seq << " with " << Simulator::Now () << ". already " << m_seqTimeouts.size () << " items");

  In_flight_Seq[seq].last_retx = Simulator::Now();

  if (In_flight_Seq[seq].retxEvent.IsRunning()) {
    In_flight_Seq[seq].retxEvent.Cancel();
    Simulator::Remove(In_flight_Seq[seq].retxEvent);
  }
  In_flight_Seq[seq].retxEvent = Simulator::Schedule (m_retxTimer,
                                     &ConsumerD::Timeout, this, seq);
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////


void
ConsumerD::OnData (Ptr<const Data> data)
{
  if (!m_active) return;

  App::OnData (data); // tracing inside

  NS_LOG_FUNCTION (this << data);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  if (data->GetName().size() >= 3 && data->GetName().get(-3).toUri() == "detect") {
    OnDetectData (data);
    return;
  }
    
  uint32_t seq = data->GetName ().get (-1).toSeqNum ();
  NS_LOG_INFO ("< DATA for " << seq);

  if (seq > p_latest_seq) {
    p_latest_seq = seq;
  }

  std::map<uint32_t,Seq_Info>::iterator receSeq = In_flight_Seq.find(seq);
  if (receSeq == In_flight_Seq.end())
    return;

  if (receSeq->second.retxEvent.IsRunning()) {
    receSeq->second.retxEvent.Cancel();
    Simulator::Remove(receSeq->second.retxEvent);
  }

  // record in file
  m_PacketRecord(this, data->GetName().toUri(), seq, "C_Data", In_flight_Seq[seq].retx_count, 
                 m_MaxWindow, m_MaxWindow, m_interestLifeTime);

  In_flight_Seq.erase(seq);

  ScheduleNextPacket();
}

void
ConsumerD::OnDetectData (Ptr<const Data> data)
{
  uint32_t tag = data->GetName ().get (-2).toSeqNum ();
  uint32_t seq = data->GetName ().get (-1).toSeqNum ();

  if (m_detect_send_time.find(tag) == m_detect_send_time.end())
    return;
  Time rtt = Simulator::Now() - m_detect_send_time[tag];

  if (seq >= p_seq_b) {
    p_seq_a = p_seq_b;
    p_seq_b = seq;

    if (seq > p_latest_seq)
      p_latest_seq = seq;
  }
  if(p_seq_a && p_seq_b) {
    Time delta = Simulator::Now() - p_detect_time;

    m_MaxWindow = (p_seq_b - p_seq_a) / delta.ToDouble(Time::MS) * rtt.ToDouble(Time::MS);
    m_MaxWindow = (m_MaxWindow+1) * 2;
  }

  p_detect_time = Simulator::Now();

  std::cout << "[consumer] receive detect reply, rtt:" << rtt.As(Time::S) << " last seq:" << seq  << " win :" << m_MaxWindow << std::endl;

  m_detect_send_time.erase(tag);

  m_PacketRecord(this, data->GetName().toUri(), seq, "C_Detect_Data", 0, 
                 m_MaxWindow, m_MaxWindow, m_interestLifeTime);

  ScheduleNextPacket();
}

void
ConsumerD::OnNack (Ptr<const Interest> interest)
{
  if (!m_active) return;

  App::OnNack (interest); // tracing inside

  // NS_LOG_DEBUG ("Nack type: " << interest->GetNack ());

  // NS_LOG_FUNCTION (interest->GetName ());

  // NS_LOG_INFO ("Received NACK: " << boost::cref(*interest));
  uint32_t seq = interest->GetName ().get (-1).toSeqNum ();
  NS_LOG_INFO ("< NACK for " << seq);
  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s -> " << "NACK for " << seq << "\n";

  std::map<uint32_t,Seq_Info>::iterator receSeq = In_flight_Seq.find(seq);
  if (receSeq == In_flight_Seq.end())
    return;

  // record in file
  m_PacketRecord(this, interest->GetName().toUri(), seq, "C_Nack", In_flight_Seq[seq].retx_count, 
                 m_MaxWindow, m_MaxWindow, m_interestLifeTime);

  SendPacket (seq);
}

} // namespace ndn
} // namespace ns3
