/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2012 University of California, Los Angeles
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
// line-topo-realtime.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/ndnSIM/utils/tracers/app-packet-tracer.h"

using namespace ns3;

/**
 * This scenario simulates a grid topology (using topology reader module)
 *
 * (consumer) -- ( ) ----- ( )  -- (producer)
 *
 * All links are 1Mbps with propagation 10ms delay. 
 *
 * FIB is populated using NdnGlobalRoutingHelper.
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=line-topo-realtime --vis
 */

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("scratch/subdir/topo/line-topo.txt");
  topologyReader.Read ();


  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
  ndnHelper.InstallAll ();

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  // Getting containers for the consumer/producer
  Ptr<Node> producer = Names::Find<Node> ("P");
  NodeContainer consumerNodes;
  consumerNodes.Add (Names::Find<Node> ("C"));

  // Install NDN applications
  std::string prefix = "/prefix";

  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerR");
  consumerHelper.SetPrefix (prefix);
  consumerHelper.SetAttribute("RetxTimer", StringValue ("500ms"));
  // consumerHelper.SetAttribute ("Frequency", StringValue ("2")); // 100 interests a second
  consumerHelper.Install (consumerNodes);

  ndn::AppHelper producerHelper ("ns3::ndn::ProducerR");
  producerHelper.SetPrefix (prefix);
  producerHelper.SetAttribute("Frequency", StringValue ("1")); // 100 data a second
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Randomize", StringValue ("exponential"));
  producerHelper.SetAttribute("MaxSeq", IntegerValue (100));
  producerHelper.Install (producer);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins (prefix, producer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes ();

  // add tracer to record in file
  ndn::AppPacketTracer::InstallAll ("scratch/subdir/record/line-packet-record.txt");

  Simulator::Stop (Seconds (20.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
