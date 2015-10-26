/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
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
 * Author:  Jin Pengfei <jinpengfei@cstnet.cn>
 */

#include "hybrid-forwarding.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"

#include "ns3/ndn-content-store.h"


#include "ns3/assert.h"
#include "ns3/log.h"

#include <boost/foreach.hpp>

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (HybridForwording);

std::string
HybridForwording::GetLogName ()
{
  return super::GetLogName ()+".HybridForwording";
}

LogComponent HybridForwording::g_log = LogComponent (HybridForwording::GetLogName ().c_str ());

TypeId
HybridForwording::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::HybridForwording")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <HybridForwording> ()
    ;
  return tid;
}

HybridForwording::HybridForwording ()
{
}

HybridForwording::~HybridForwording ()
{
}

void
HybridForwording::OnInterest (Ptr<Face> inFace,
                              Ptr<Interest> interest)
{
  if (interest->GetPushTag() == Interest::PUSH_SUB_INTEREST) {
    // std::cout << "[forwarder]receive subscribe interest" << std::endl;
    OnSubscribe(inFace,interest);    

  } else if (interest->GetPushTag() == Interest::PULL_INTEREST) {
    // std::cout << "[forwarder]receive pull interest" << std::endl;
    super::OnInterest(inFace, interest);
  }
  
}

void
HybridForwording::OnSubscribe (Ptr<Face> inFace,
                               Ptr<Interest> interest)
{
  NS_LOG_FUNCTION (inFace << interest->GetName ());
  m_inInterests (interest, inFace);

  Ptr<pit::Entry> pitEntry = m_pit->Lookup (*interest);
  bool similarInterest = true;
  if (pitEntry == 0)
    {
      similarInterest = false;
      pitEntry = m_pit->Create (interest);
      if (pitEntry != 0)
        {
          pitEntry->SetpushTag();
          DidCreatePitEntry (inFace, interest, pitEntry);
        }
      else
        {
          FailedToCreatePitEntry (inFace, interest);
          return;
        }
    }

  bool isDuplicated = true;
  if (!pitEntry->IsNonceSeen (interest->GetNonce ()))
    {
      pitEntry->AddSeenNonce (interest->GetNonce ());
      isDuplicated = false;
    }

  if (isDuplicated)
    {
      DidReceiveDuplicateInterest (inFace, interest, pitEntry);
      return;
    }

  if (similarInterest && ShouldSuppressIncomingInterest (inFace, interest, pitEntry))
    {
      pitEntry->AddIncoming (inFace/*, interest->GetInterestLifetime ()*/);
      // update PIT entry lifetime
      pitEntry->UpdateLifetime (interest->GetInterestLifetime ());

      // Suppress this interest if we're still expecting data from some other face
      NS_LOG_DEBUG ("Suppress interests");
      m_dropInterests (interest, inFace);

      DidSuppressSimilarInterest (inFace, interest, pitEntry);
      return;
    }

  if (similarInterest)
    {
      pitEntry->AddIncoming (inFace);
      // update PIT entry lifetime
      pitEntry->UpdateLifetime (interest->GetInterestLifetime ());

      DidForwardSimilarInterest (inFace, interest, pitEntry);
    }

  PropagateInterest (inFace, interest, pitEntry);
}


void
HybridForwording::OnData (Ptr<Face> inFace,
                          Ptr<Data> data)
{
  NS_LOG_FUNCTION (inFace << data->GetName ());
  m_inData (data, inFace);

  if (data->GetPushTag() == Data::PUSH_DATA) {
    // uint32_t seq = data->GetName ().get (-1).toSeqNum ();
    // std::cout << "[forwarder]recive push data: " << seq << std::endl;
    // std::cout << "[forwarder]recive push data: " << data->GetName() << "  "<< data->GetName().getPrefix(data->GetName().size()-1) << std::endl;

    OnPushData(inFace,data);

  } else if (data->GetPushTag() == Data::PUSH_SUB_ACK) {

    OnPushAck(inFace,data);

  } else if (data->GetPushTag() == Data::PULL_DATA) {
    // std::cout << "[forwarder]recive pull data: " << data->GetName().toUri() << std::endl;
    OnPullData(inFace,data);
  }

}

void
HybridForwording::OnPushData (Ptr<Face> inFace,
                              Ptr<Data> data)
{
  // Find PIT entry
  Ptr<pit::Entry> pitEntry = m_pit->Find (data->GetName().getPrefix(data->GetName().size()-1));
  if (pitEntry == 0)
    {
      bool cached = false;

      if (m_cacheUnsolicitedData || (m_cacheUnsolicitedDataFromApps && (inFace->GetFlags () & Face::APPLICATION)))
        {
          // Optimistically add or update entry in the content store
          cached = m_contentStore->Add (data);
        }
      else
        {
          // Drop data packet if PIT entry is not found
          // (unsolicited data packets should not "poison" content store)

          //drop dulicated or not requested data packet
          m_dropData (data, inFace);
        }

      DidReceiveUnsolicitedData (inFace, data, cached);
      return;
    }
  else
    {
      bool cached = m_contentStore->Add (data);
      DidReceiveSolicitedData (inFace, data, cached);
    }

    // std::cout << "[forwarder] pit entry push tag: " << pitEntry->GetpushTag() << " seq: " 
    //           << pitEntry->GetSeq() << std::endl; 

  if (pitEntry != 0 && pitEntry->GetpushTag())
    {
      if (data->GetName().get(-1).toSeqNum() <= pitEntry->GetSeq())
      {
        super::OnData(inFace,data);
        return;
      }

      pitEntry->SetSeq(data->GetName().get(-1).toSeqNum());

      // Do data plane performance measurements
      WillSatisfyPendingInterest (inFace, pitEntry);

      //satisfy all pending incoming Interests
      BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
      {
        bool ok = incoming.m_face->SendData (data);

        DidSendOutData (inFace, incoming.m_face, data, pitEntry);
        NS_LOG_DEBUG ("Satisfy " << *incoming.m_face);

        if (!ok)
        {
          m_dropData (data, incoming.m_face);
          NS_LOG_DEBUG ("Cannot satisfy data to " << *incoming.m_face);
        }
      }
    }
}

void
HybridForwording::OnPushAck(Ptr<Face> inFace,
                              Ptr<Data> data)
{
  // Find PIT entry
  Ptr<pit::Entry> pitEntry = m_pit->Find (data->GetName());
  if (pitEntry == 0)
    {
      bool cached = false;

      if (m_cacheUnsolicitedData || (m_cacheUnsolicitedDataFromApps && (inFace->GetFlags () & Face::APPLICATION)))
        {
          // Optimistically add or update entry in the content store
          cached = m_contentStore->Add (data);
        }
      else
        {
          // Drop data packet if PIT entry is not found
          // (unsolicited data packets should not "poison" content store)

          //drop dulicated or not requested data packet
          m_dropData (data, inFace);
        }

      DidReceiveUnsolicitedData (inFace, data, cached);
      return;
    }
  else
    {
      bool cached = m_contentStore->Add (data);
      DidReceiveSolicitedData (inFace, data, cached);
    }

    // std::cout << "[forwarder] pit entry push tag: " << pitEntry->GetpushTag() << " seq: " 
    //           << pitEntry->GetSeq() << std::endl; 

  if (pitEntry != 0 && pitEntry->GetpushTag())
    {
      pitEntry->SetSeq(data->GetPushSeq());

      // Do data plane performance measurements
      WillSatisfyPendingInterest (inFace, pitEntry);

      //satisfy all pending incoming Interests
      BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
      {
        bool ok = incoming.m_face->SendData (data);

        DidSendOutData (inFace, incoming.m_face, data, pitEntry);
        NS_LOG_DEBUG ("Satisfy " << *incoming.m_face);

        if (!ok)
        {
          m_dropData (data, incoming.m_face);
          NS_LOG_DEBUG ("Cannot satisfy data to " << *incoming.m_face);
        }
      }
    }
}

void
HybridForwording::OnPullData (Ptr<Face> inFace,
                              Ptr<Data> data)
{
  // Find PIT entry
  Ptr<pit::Entry> pitEntry = m_pit->Find (data->GetName());
  if (pitEntry == 0)
    {
      bool cached = false;

      if (m_cacheUnsolicitedData || (m_cacheUnsolicitedDataFromApps && (inFace->GetFlags () & Face::APPLICATION)))
        {
          // Optimistically add or update entry in the content store
          cached = m_contentStore->Add (data);
        }
      else
        {
          // Drop data packet if PIT entry is not found
          // (unsolicited data packets should not "poison" content store)

          //drop dulicated or not requested data packet
          m_dropData (data, inFace);
        }

      DidReceiveUnsolicitedData (inFace, data, cached);
      return;
    }
  else
    {
      bool cached = m_contentStore->Add (data);
      DidReceiveSolicitedData (inFace, data, cached);
    }

  if (pitEntry != 0)
    {
      // Do data plane performance measurements
      WillSatisfyPendingInterest (inFace, pitEntry);

      // Actually satisfy pending interest
      SatisfyPendingInterest (inFace, data, pitEntry);
    }
}

} // namespace fw
} // namespace ndn
} // namespace ns3
