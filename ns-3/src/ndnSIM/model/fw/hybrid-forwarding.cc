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
 * Author:  Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *          Ilya Moiseenko <iliamo@cs.ucla.edu>
 */

#include "hybrid-forwarding.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"

#include "ns3/assert.h"
#include "ns3/log.h"

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
HybridForwording::OnInterest (Ptr<Face> face,
                              Ptr<Interest> interest)
{
  if (interest->GetPushTag() == Interest::PUSH_SUB_INTEREST) {

    std::cout << "[forwarder]receive subscribe interest" << std::endl;

    
  } else if (interest->GetPushTag() == Interest::PULL_INTEREST) {

    std::cout << "[forwarder]receive pull interest" << std::endl;

  }

  super::OnInterest(face, interest);

}


void
HybridForwording::OnData (Ptr<Face> face,
                          Ptr<Data> data)
{
  uint32_t seq = data->GetName ().get (-1).toSeqNum ();

  if (data->GetPushTag() == Data::PUSH_DATA) {

    std::cout << "[forwarder]recive push data: " << seq << std::endl;

  } else if (data->GetPushTag() == Data::PULL_DATA) {

    std::cout << "[forwarder]recive pull data: " << seq << std::endl;
  }

  super::OnData(face,data);

}

} // namespace fw
} // namespace ndn
} // namespace ns3

