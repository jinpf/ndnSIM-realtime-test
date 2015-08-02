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

#ifndef NDN_HYBRID_FORWARDING_H
#define NDN_HYBRID_FORWARDING_H

#include "best-route.h"
#include "ns3/log.h"

namespace ns3 {
namespace ndn {
namespace fw {

class HybridForwording: public BestRoute
{
private:
  typedef BestRoute super;

public:
  static TypeId
  GetTypeId ();

  static std::string
  GetLogName ();
  
  /**
   * @brief Default constructor
   */
  HybridForwording ();

  ~HybridForwording ();

  void
  OnInterest (Ptr<Face> face,
              Ptr<Interest> interest);

  void
  OnData (Ptr<Face> face,
          Ptr<Data> data);

protected:
  static LogComponent g_log;
};


} // namespace fw
} // namespace ndn
} // namespace ns3


#endif /* NDN_HYBRID_FORWARDING_H */
