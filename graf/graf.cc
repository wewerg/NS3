/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
//
// This program configures a grid (default 5x5) of nodes on an 
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000 
// (application) bytes to node 1.
//
// The default layout is like this, on a 2-D grid.
//
// n20  n21  n22  n23  n24
// n15  n16  n17  n18  n19
// n10  n11  n12  n13  n14
// n5   n6   n7   n8   n9
// n0   n1   n2   n3   n4
//
// the layout is affected by the parameters given to GridPositionAllocator;
// by default, GridWidth is 5 and numNodes is 25..
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc-grid --help"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the ns-3 documentation.
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when distance increases beyond
// the default of 500m.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --distance=500"
// ./waf --run "wifi-simple-adhoc --distance=1000"
// ./waf --run "wifi-simple-adhoc --distance=1500"
// 
// The source node and sink node can be changed like this:
// 
// ./waf --run "wifi-simple-adhoc --sourceNode=20 --sinkNode=10"
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
// 
// ./waf --run "wifi-simple-adhoc-grid --verbose=1"
//
// By default, trace file writing is off-- to enable it, try:
// ./waf --run "wifi-simple-adhoc-grid --tracing=1"
//
// When you are done tracing, you will notice many pcap trace files 
// in your directory.  If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-grid-0-0.pcap -nn -tt
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/config-store-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

float odchylka_priemer(vector<double> vys, double& priemer)
{
	double mean = 0.0, sum_deviation = 0.0;
	int counter;
	counter = 0;
	unsigned int i;
	for (i = 0; i < vys.size(); i++)
	{
	mean += vys[i];
	counter++;
	}
        
	mean /= counter;
	priemer = mean;
	for (i = 0; i < vys.size(); i++) 
            sum_deviation += (vys[i] - mean)*(vys[i] - mean);
        
	return sqrt(sum_deviation / counter);
}
void Graf(Gnuplot2dDataset orig)
{
	string graphicsFileName;
	string plotFileName;
	string fileNameWithNoExtension;
	string dataTitle;
	string plotTitle;

	
		fileNameWithNoExtension = "Uspesnost";
		graphicsFileName = fileNameWithNoExtension + ".svg";
		plotFileName = fileNameWithNoExtension + ".plt";
		plotTitle = "Uspesnost";
		dataTitle = "Uspesnost";

	

	Gnuplot plot(graphicsFileName);
	plot.SetTitle(plotTitle);

	
		plot.SetLegend("Pocet uzlov", "Uspesnost Rx:Tx ");
		plot.AppendExtra("set xrange [+0:+100]");
	

	plot.SetTerminal("svg");

	orig.SetTitle(dataTitle);
	orig.SetStyle(Gnuplot2dDataset::LINES_POINTS);

	// Add the dataset to the plot.
	plot.AddDataset(orig);
	// Open the plot file.
	ofstream plotFile(plotFileName.c_str());

	// Write the plot file.
	plot.GenerateOutput(plotFile);

	// Close the plot file.
	plotFile.close();
}
void vytvorGraf(map<int, double> vysledky, map<int, double> odchylky, int pocetOpakovani)
{
    Gnuplot2dDataset orig;
    orig.SetErrorBars(Gnuplot2dDataset::Y);

    for (map<int, double>::iterator it = vysledky.begin(); it != vysledky.end(); ++it)
		orig.Add(it->first, it->second, odchylky[it->first]);

    Graf(orig);
}


int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double distance = 50;  // m
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 1;
  uint32_t numNodes = 25;  // by default, 5x5
  uint32_t sinkNode = 0;
  uint32_t sourceNode = 24;
  double interval = 1.0; // seconds
  bool verbose = false;
  bool tracing = false;
  map<int, double> vysledkyUspesnotOdchylka;
  map<int, double> vysledkyUspesnot;
  CommandLine cmd;
  for(int op=0; op< 10; op++){
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("numNodes", "number of nodes", numNodes);
  cmd.AddValue ("sinkNode", "Receiver node number", sinkNode);
  cmd.AddValue ("sourceNode", "Sender node number", sourceNode);

  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", 
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (numNodes);
     //Polnohospodar
    NodeContainer hosp;// = wifiApNode
    hosp.Create(1);
    
    NodeContainer senzorSiet; //= allNodes;
    senzorSiet.Add(c);
    senzorSiet.Add(hosp);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10) ); 
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, senzorSiet);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (distance),
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator","X", StringValue ("ns3::UniformRandomVariable[Min=300.0|Max=500.0]"),"Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=30.0]"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (-100, 200, -100, 200)));
  mobility.Install (hosp);

  // Enable OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.Install (senzorSiet);

  Ipv4AddressHelper address;
  NS_LOG_INFO ("Assign IP Addresses.");
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = address.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (sinkNode), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (c.Get (sourceNode), tid);
  InetSocketAddress remote = InetSocketAddress (i.GetAddress (sinkNode, 0), 80);
  source->Connect (remote);



  // Give OLSR time to converge-- 30 seconds perhaps
  Simulator::Schedule (Seconds (15.0), &GenerateTraffic, 
                       source, packetSize, numPackets, interPacketInterval);

  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);
  FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  Simulator::Stop (Seconds (16.0));
  
  AnimationInterface anim ("wireless-animation-ps2.xml"); // Mandatory
  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
         ostringstream stringStream;
         stringStream << "Senz" << i;
         anim.UpdateNodeDescription (c.Get (i), stringStream.str()); // Optional
         anim.UpdateNodeColor (c.Get (i), 255, 0, 0); // Optional
    }

  anim.UpdateNodeDescription (hosp.Get (0), "Hosp"); // Optional
  anim.UpdateNodeColor (hosp.Get (0), 0, 255, 0); // Optional
  anim.EnablePacketMetadata (); // Optional
  anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
  anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional
  
  /*vector<pair<ns3::Ipv4Address, ns3::Ipv4Address> > usedAddresses;
		
	for (unsigned i = 0; i < 5; ++i)
		{
		

			//	cout << ifcont.GetAddress(node1) << " " << ifcont.GetAddress(node2) << endl;
	usedAddresses.push_back(address.NewAddress, address.NewAddress());
        	address.		
		}*/
  
  
  Simulator::Run ();
  Simulator::Destroy ();
  vector<double> vysledky;
  uint32_t sumaT = 0;
		uint32_t sumaR = 0;
		
		monitor->CheckForLostPackets();
		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
		map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
		
		for (map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
		{
							NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
					NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
					NS_LOG_UNCOND("Tx Bytes = " << iter->second.txBytes);
					NS_LOG_UNCOND("Rx Bytes = " << iter->second.rxBytes);
					NS_LOG_UNCOND("Delay sum = " << iter->second.delaySum.GetSeconds());
					NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024 << " Kbps");
					NS_LOG_UNCOND("Uspesne prenesene: " << ((double)iter->second.rxPackets / (double)iter->second.txPackets) * 100 << " %");
					NS_LOG_UNCOND("Uspesne prenesene bytes: " << ((double)iter->second.rxBytes / (double)iter->second.txBytes) * 100 << " %");
					sumaT += iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024;
					sumaR += iter->second.rxBytes;
                                        
					
		

		}

		double vys = (sumaR / (double)sumaT) * 100;
		
		vysledky.push_back(vys);
                double odchylka, priemer;
                odchylka = odchylka_priemer(vysledky, priemer);
                
                vysledkyUspesnotOdchylka[op] = odchylka;
                vysledkyUspesnot[op] = priemer;
  }
                vytvorGraf(vysledkyUspesnot, vysledkyUspesnotOdchylka, 10);
  return 0;
}
