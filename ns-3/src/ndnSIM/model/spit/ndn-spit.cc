/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Jin Pengfei <jinpengfei@cstnet.cn>
 */

#include "ndn-spit.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

NS_LOG_COMPONENT_DEFINE ("ndn.SPit");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (SPit);

TypeId
SPit::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::SPit")
    .SetGroupName ("Ndn")
    .SetParent<Object> ()

    .AddAttribute ("SPitEntryPruningTimout",
                   "Timeout for SPIT entry to live after being satisfied. To make sure recently satisfied interest will not be satisfied again",
                   TimeValue (), // by default, SPIT entries are removed instantly
                   MakeTimeAccessor (&SPit::m_SPitEntryPruningTimout),
                   MakeTimeChecker ())

    .AddAttribute ("MaxSPitEntryLifetime",
                   "Maximum amount of time for which a router is willing to maintain a SPIT entry. "
                   "Actual SPIT lifetime should be minimum of MaxSPitEntryLifetime and InterestLifetime specified in the Interest packet",
                   TimeValue (), // by default, SPIT entries are kept for the time, specified by the InterestLifetime
                   MakeTimeAccessor (&SPit::GetMaxSPitEntryLifetime, &SPit::SetMaxSPitEntryLifetime),
                   MakeTimeChecker ())
    ;

  return tid;
}

SPit::SPit ()
{
}

SPit::~SPit ()
{
}

} // namespace ndn
} // namespace ns3
