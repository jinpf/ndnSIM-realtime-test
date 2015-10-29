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

#include "ndn-producer-schedule-sync.h"
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

#include <fstream>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerSS");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerSS);

TypeId
ProducerSS::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerSS")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerSS> ()
    .AddAttribute ("Prefix","Prefix, for which realtime producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerSS::m_prefix),
                   MakeNameChecker ())
    // .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
    //                StringValue ("/"),
    //                MakeNameAccessor (&ProducerSS::m_postfix),
    //                MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerSS::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&ProducerSS::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerSS::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerSS::m_keyLocator),
                   MakeNameChecker ())

    .AddAttribute ("MaxSeq", "Max sequence number that can generate",
                   IntegerValue (1000),
                   MakeIntegerAccessor(&ProducerSS::m_seqMax),
                   MakeIntegerChecker<int32_t>())

    .AddAttribute ("FileName", "load file name, from which to schedule generate packet",
                   StringValue ("scratch/subdir/source_data/test.txt"),
                   MakeStringAccessor(&ProducerSS::m_filename),
                   MakeStringChecker ())

    .AddTraceSource ("PacketRecord", "Record data send and receive in file",
                     MakeTraceSourceAccessor (&ProducerSS::m_PacketRecord))

    ;
  return tid;
}

ProducerSS::ProducerSS ()
  : m_seq (0)
{
  // NS_LOG_FUNCTION_NOARGS ();
  m_seqMax = std::numeric_limits<uint32_t>::max ();

}

ProducerSS::~ProducerSS ()
{
}

// inherited from Application base class.
void
ProducerSS::StartApplication ()
{
  // load schedule infomation from file
  std::ifstream indata(m_filename.c_str());
  if (!indata.is_open()) {
    std::cout << "Unable to open file! " << m_filename << std::endl ;
    exit(0);
  }
  std::string line;
  std::getline(indata,line);
  uint32_t seq = 1;
  while (std::getline(indata,line)) {
    std::string a = "", b = "";
    bool flag = true;
    for (unsigned i = 0; i < line.size(); ++i) {
      if (line[i] == '\t') {
        flag = false;
        continue;
      }
      if (flag)
        a += line[i];
      else
        b += line[i];
    }
    a += "s";
    m_schedule_data[seq].time = Time(a);
    m_schedule_data[seq].size = std::atoi(b.c_str());
    seq++;
  }
  indata.close();

  // m_schedule_data[Time("1s")] = 1024;
  // m_schedule_data[Time("2s")] = 1024;
  // m_schedule_data[Time("3s")] = 1024;

  m_time_value_it = m_schedule_data.begin();

  // end schedule

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
ProducerSS::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);

  App::StopApplication ();
}


void
ProducerSS::OnInterest (Ptr<const Interest> interest)
{
  App::OnInterest (interest); // tracing inside

  NS_LOG_FUNCTION (this << interest);

  if (!m_active) return;

  if (interest->GetPushTag() == Interest::PUSH_SUB_INTEREST) {

    uint32_t seq = interest->GetPushSeq();

    std::cout << "[producer]receive sync information, consumer seq = " << seq << std::endl;

    SendSyncAck();

    // record in file
    m_PacketRecord(this, interest->GetName().toUri(), seq, "P_Sync_Interest", 0, 
                   0, 0, interest->GetInterestLifetime());
    
  } else if (interest->GetPushTag() == Interest::PULL_INTEREST) {

    Name dataName(interest->GetName());
    uint32_t seq = dataName.get(-1).toSeqNum();

    std::cout << "[producer]receive comsumer request: " << seq << " ";

    // record in file
    m_PacketRecord(this, interest->GetName().toUri(), seq, "P_Pull_Interest", 0, 
                   0, 0, interest->GetInterestLifetime());

    if ( seq > m_seq ) {
      std::cout << " but i don`t have..." << std::endl;
      return;
    }

    SendData(seq,false);

  }

}

void
ProducerSS::SendSyncAck()
{
  Ptr<Name> dataName = Create<Name> (m_prefix);
  Ptr<Data> data = Create<Data> (Create<Packet> (0));
  data->SetName (dataName);
  data->SetPushTag(Data::PUSH_SUB_ACK);
  data->SetPushSeq(m_seq);
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
  m_PacketRecord(this, dataName->toUri(), m_seq, "P_Sync_Ack", 0, 
                   0, 0, Time(0));

  std::cout << "[producer]send Sync ack: " << m_seq << std::endl;
}

// send data with given seq
void
ProducerSS::SendData(const uint32_t &seq, bool push)
{
  Ptr<Name> dataName = Create<Name> (m_prefix);
  dataName->appendSeqNum (seq);
  Ptr<Data> data = Create<Data> (Create<Packet> (m_schedule_data[seq].size));
  data->SetName (dataName);
  if (push)
    data->SetPushTag(Data::PUSH_DATA);
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
  if (push)
    m_PacketRecord(this, dataName->toUri(), seq, "P_Push_Data", 0, 
                   0, 0, Time(0));
  else
    m_PacketRecord(this, dataName->toUri(), seq, "P_Pull_Data", 0, 
                   0, 0, Time(0));

  std::cout << "[producer]send data: " << seq << std::endl;
}

// Attention! not really generate data, just add seq number pretend to generate data
void
ProducerSS::GenerateData()
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
ProducerSS::ScheduleNextData()
{
  if (!m_generateEvent.IsRunning () && m_seq < m_seqMax && m_time_value_it != m_schedule_data.end()) {
    m_generateEvent = Simulator::Schedule (m_time_value_it->second.time.Compare(Simulator::Now()) >= 0 ?
                                           m_time_value_it->second.time - Simulator::Now() : Seconds (0.0), 
                                           &ProducerSS::GenerateData, this);
    m_time_value_it++;
  }
}

} // namespace ndn
} // namespace ns3
