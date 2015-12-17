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

#include "ndn-producer-realtime-detect.h"
#include "ns3/log.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-fib.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <iostream>
namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerD");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerD);

TypeId
ProducerD::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerD")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerD> ()
    .AddAttribute ("Prefix","Prefix, for which realtime producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerD::m_prefix),
                   MakeNameChecker ())
    // .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
    //                StringValue ("/"),
    //                MakeNameAccessor (&ProducerD::m_postfix),
    //                MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerD::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&ProducerD::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerD::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerD::m_keyLocator),
                   MakeNameChecker ())

    .AddAttribute ("Frequency", "Frequency of data packet generate",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ProducerD::m_frequency),
                   MakeDoubleChecker<double> ())

    .AddAttribute ("MaxSeq", "Max sequence number that can generate",
                   IntegerValue (1000),
                   MakeIntegerAccessor(&ProducerD::m_seqMax),
                   MakeIntegerChecker<int32_t>())

    .AddAttribute ("Randomize", "Type of send time randomization: none (default), uniform, exponential",
                   StringValue ("none"),
                   MakeStringAccessor (&ProducerD::SetRandomize, &ProducerD::GetRandomize),
                   MakeStringChecker ())

    .AddTraceSource ("PacketRecord", "Record data send and receive in file",
                     MakeTraceSourceAccessor (&ProducerD::m_PacketRecord))

    ;
  return tid;
}

ProducerD::ProducerD ()
  : m_seq (0)
  , m_frequency (1.0)
  , m_random (0)
{
  // NS_LOG_FUNCTION_NOARGS ();
  m_seqMax = std::numeric_limits<uint32_t>::max ();
}

ProducerD::~ProducerD ()
{
  if (m_random)
    delete m_random;
}

// inherited from Application base class.
void
ProducerD::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);

  App::StartApplication ();

  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());

  Ptr<Fib> fib = GetNode ()->GetObject<Fib> ();

  Ptr<fib::Entry> fibEntry = fib->Add (m_prefix, m_face, 0);

  fibEntry->UpdateStatus (m_face, fib::FaceMetric::NDN_FIB_GREEN);

  // // make face green, so it will be used primarily
  // StaticCast<fib::FibImpl> (fib)->modify (fibEntry,
  //                                        ll::bind (&fib::Entry::UpdateStatus,
  //                                                  ll::_1, m_face, fib::FaceMetric::NDN_FIB_GREEN));

  ScheduleNextData();
}

void
ProducerD::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);

  App::StopApplication ();
}


void
ProducerD::OnInterest (Ptr<const Interest> interest)
{
  App::OnInterest (interest); // tracing inside

  NS_LOG_FUNCTION (this << interest);

  if (!m_active) return;

  Name dataName(interest->GetName());

  if (dataName.get(-2).toUri() == "detect") {
    OnDetectInterest(dataName);
    return;
  }

  uint32_t seq = dataName.get(-1).toSeqNum();

  std::cout << "[producer]receive comsumer request: " << seq ;

  // record in file
  m_PacketRecord(this, interest->GetName().toUri(), seq, "P_Interest", 0, 
                 0, 0, Time(0));

  if ( seq > m_seq ) {
    std::cout << "  but i don`t have..." << std::endl;
    return;
  }

  SendData(seq);

}

void 
ProducerD::OnDetectInterest(Name & name)
{
  name.appendSeqNum(m_seq);
  Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
  data->SetName (name);
  data->SetFreshness (m_freshness);
  data->SetTimestamp (Simulator::Now());

  data->SetSignature (m_signature);
  if (m_keyLocator.size () > 0)
    {
      data->SetKeyLocator (Create<Name> (m_keyLocator));
    }

  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());

  m_face->ReceiveData (data);
  m_transmittedDatas (data, this, m_face);

  // record in file
  // m_PacketRecord(this, dataName->toUri(), seq, "P_Data", 0, 
  //                0, 0, Time(0));

  std::cout << " reply detect data " << m_seq << std::endl;
}

// send data with given seq
void
ProducerD::SendData(const uint32_t &seq)
{
  Ptr<Name> dataName = Create<Name> (m_prefix);
  dataName->appendSeqNum (seq);
  Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
  data->SetName (dataName);
  data->SetFreshness (m_freshness);
  data->SetTimestamp (Simulator::Now());

  data->SetSignature (m_signature);
  if (m_keyLocator.size () > 0)
    {
      data->SetKeyLocator (Create<Name> (m_keyLocator));
    }

  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());

  m_face->ReceiveData (data);
  m_transmittedDatas (data, this, m_face);

  // record in file
  m_PacketRecord(this, dataName->toUri(), seq, "P_Data", 0, 
                 0, 0, Time(0));

  std::cout << "  send data  " << seq << std::endl;
}

// Attention! not really generate data, just add seq number pretend to generate data
void
ProducerD::GenerateData()
{
  if (m_seq != std::numeric_limits<uint32_t>::max()) {
    // generate data and plus seq number
    m_seq++;
    std::cout << "[producer]generate data:" << m_seq << std::endl;

    Ptr<Name> dataName = Create<Name> (m_prefix);
    dataName->appendSeqNum (m_seq);

    // record in file
    m_PacketRecord(this, dataName->toUri(), m_seq, "P_GData", 0, 
                   0, 0, Time(0));

    // schedule to generate next data
    ScheduleNextData();
  }
}

void
ProducerD::ScheduleNextData()
{
  if (!m_seq)
    {
      m_generateEvent = Simulator::Schedule (Seconds (0.0),
                                         &ProducerD::GenerateData, this);
    }
  else if (!m_generateEvent.IsRunning () && m_seq < m_seqMax)
    m_generateEvent = Simulator::Schedule (
                                       (m_random == 0) ?
                                         Seconds(1.0 / m_frequency)
                                       :
                                         Seconds(m_random->GetValue ()),
                                       &ProducerD::GenerateData, this);
}

void
ProducerD::SetRandomize(const std::string &value)
{
  if (m_random)
    delete m_random;

  if (value == "uniform") {
    m_random = new UniformVariable(0.0, 2 * 1.0 / m_frequency);
  }
  else if (value == "exponential") {
    m_random = new ExponentialVariable(1.0 / m_frequency, 50 * 1.0 / m_frequency);
  }
  else
    m_random = 0;
  m_randomType = value;
}

std::string
ProducerD::GetRandomize() const
{
  return m_randomType;
}

} // namespace ndn
} // namespace ns3
