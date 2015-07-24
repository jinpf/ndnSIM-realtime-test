/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013 University of California, Los Angeles
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
 * Author: Jin Pengfe <jinpengfei@cstnet.cn>
 */

#include "app-packet-tracer.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/callback.h"

#include "ns3/ndn-app.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/simulator.h"
#include "ns3/node-list.h"
#include "ns3/log.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include <fstream>

NS_LOG_COMPONENT_DEFINE ("ndn.AppPacketTracer");

using namespace std;

namespace ns3 {
namespace ndn {

static std::list< boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<AppPacketTracer> > > > g_tracers;

template<class T>
static inline void
NullDeleter (T *ptr)
{
}

void
AppPacketTracer::Destroy ()
{
  g_tracers.clear ();
}

void
AppPacketTracer::InstallAll (const std::string &file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<AppPacketTracer> > tracers;
  boost::shared_ptr<std::ostream> outputStream;
  if (file != "-")
    {
      boost::shared_ptr<std::ofstream> os (new std::ofstream ());
      os->open (file.c_str (), std::ios_base::out | std::ios_base::trunc);

      if (!os->is_open ())
        {
          NS_LOG_ERROR ("File " << file << " cannot be opened for writing. Tracing disabled");
          return;
        }

      outputStream = os;
    }
  else
    {
      outputStream = boost::shared_ptr<std::ostream> (&std::cout, NullDeleter<std::ostream>);
    }

  for (NodeList::Iterator node = NodeList::Begin ();
       node != NodeList::End ();
       node++)
    {
      Ptr<AppPacketTracer> trace = Install (*node, outputStream);
      tracers.push_back (trace);
    }

  if (tracers.size () > 0)
    {
      // *m_l3RateTrace << "# "; // not necessary for R's read.table
      tracers.front ()->PrintHeader (*outputStream);
      *outputStream << "\n";
    }

  g_tracers.push_back (boost::make_tuple (outputStream, tracers));
}

void
AppPacketTracer::Install (const NodeContainer &nodes, const std::string &file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<AppPacketTracer> > tracers;
  boost::shared_ptr<std::ostream> outputStream;
  if (file != "-")
    {
      boost::shared_ptr<std::ofstream> os (new std::ofstream ());
      os->open (file.c_str (), std::ios_base::out | std::ios_base::trunc);

      if (!os->is_open ())
        {
          NS_LOG_ERROR ("File " << file << " cannot be opened for writing. Tracing disabled");
          return;
        }

      outputStream = os;
    }
  else
    {
      outputStream = boost::shared_ptr<std::ostream> (&std::cout, NullDeleter<std::ostream>);
    }

  for (NodeContainer::Iterator node = nodes.Begin ();
       node != nodes.End ();
       node++)
    {
      Ptr<AppPacketTracer> trace = Install (*node, outputStream);
      tracers.push_back (trace);
    }

  if (tracers.size () > 0)
    {
      // *m_l3RateTrace << "# "; // not necessary for R's read.table
      tracers.front ()->PrintHeader (*outputStream);
      *outputStream << "\n";
    }

  g_tracers.push_back (boost::make_tuple (outputStream, tracers));
}

void
AppPacketTracer::Install (Ptr<Node> node, const std::string &file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<AppPacketTracer> > tracers;
  boost::shared_ptr<std::ostream> outputStream;
  if (file != "-")
    {
      boost::shared_ptr<std::ofstream> os (new std::ofstream ());
      os->open (file.c_str (), std::ios_base::out | std::ios_base::trunc);

      if (!os->is_open ())
        {
          NS_LOG_ERROR ("File " << file << " cannot be opened for writing. Tracing disabled");
          return;
        }

      outputStream = os;
    }
  else
    {
      outputStream = boost::shared_ptr<std::ostream> (&std::cout, NullDeleter<std::ostream>);
    }

  Ptr<AppPacketTracer> trace = Install (node, outputStream);
  tracers.push_back (trace);

  if (tracers.size () > 0)
    {
      // *m_l3RateTrace << "# "; // not necessary for R's read.table
      tracers.front ()->PrintHeader (*outputStream);
      *outputStream << "\n";
    }

  g_tracers.push_back (boost::make_tuple (outputStream, tracers));
}


Ptr<AppPacketTracer>
AppPacketTracer::Install (Ptr<Node> node,
                         boost::shared_ptr<std::ostream> outputStream)
{
  NS_LOG_DEBUG ("Node: " << node->GetId ());

  Ptr<AppPacketTracer> trace = Create<AppPacketTracer> (outputStream, node);

  return trace;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

AppPacketTracer::AppPacketTracer (boost::shared_ptr<std::ostream> os, Ptr<Node> node)
: m_nodePtr (node)
, m_os (os)
{
  m_node = boost::lexical_cast<string> (m_nodePtr->GetId ());

  Connect ();

  string name = Names::FindName (node);
  if (!name.empty ())
    {
      m_node = name;
    }
}

AppPacketTracer::AppPacketTracer (boost::shared_ptr<std::ostream> os, const std::string &node)
: m_node (node)
, m_os (os)
{
  Connect ();
}

AppPacketTracer::~AppPacketTracer ()
{
};


void
AppPacketTracer::Connect ()
{
  Config::ConnectWithoutContext ("/NodeList/"+m_node+"/ApplicationList/*/PacketRecord",
                                 MakeCallback (&AppPacketTracer::PacketRecord, this));
}

void
AppPacketTracer::PrintHeader (std::ostream &os) const
{
  os << "Time" << "\t"
     << "Node" << "\t"
     << "AppId" << "\t"
     << "Name" << "\t"
     << "SeqNo" << "\t"

     << "Type" << "\t"
     << "RetxCount" << "\t"
     << "WinSize" << "\t"
     << "MaxWinSize" << "\t"
     << "LifeTime" << "\n";
}

void
AppPacketTracer::PacketRecord (Ptr<App> app, std::string name , uint32_t seqno, std::string type, uint32_t RetxCount, 
                                  int32_t WinSize, int32_t MaxWinSize, Time lifetime)
{
  *m_os << Simulator::Now ().ToDouble (Time::S) << "\t"
        << m_node << "\t"
        << app->GetId () << "\t"
        << name << "\t"
        << seqno << "\t"
        << type << "\t"
        << RetxCount << "\t"
        << WinSize << "\t"
        << MaxWinSize << "\t"
        << lifetime.ToDouble (Time::S) << "\n";
}

} // namespace ndn
} // namespace ns3
