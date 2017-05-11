/*
 Polnohospodar zasial okrem plodiny aj senzorovu siet ktorou monitoruje prostredie a rast. 
 * Polnohospodar pritom obcas pride a chce stiahnut informacie zo senzorovej siete. 
 * Polnohospodar okrem udajov o prostredi ziskava informacie aj o stave baterie. 
 * Ak je pritomny na poli moze dobit bateriu senzora (dobit/nastavit na hodnotu),
 *  popripade simulujte ze niektore uzly maju solarne clanky (dobijaju sa priebezne).

    ak vstupy polnohospodar do dosahu, posle vsetkym HELLO, aby kazdy zo senzorov mohol poslat svoje udaje.
    vstup polnohospodara skuste nasimulovat nahodne na okraji pola.

    dopíste do programu kód pre grafickú vizualizáciu siete pre program "NetAnim"
    dopíste do programu kód pre vykreslenie grafu závislosti (mozete vyuzit kniznicu "gnuplot"), simulaciu spustite viac krat, aby ste ziskali aj standardnu odchylku pre vynesene body v grafe.
        parametra QoS od nastavujuceho sa parametra siete. 

Vhodne zvolte na rozsah x,y pre vynesenie do grafu. Simuláciu ukoncite tiez vhodným casom.
 
 
 
 */

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
  double distance = 100;  // m
  double Prss = -120; 
  double offset = 81;
  double harvestingUpdateInterval = 2;  // seconds
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 1;
  uint32_t numNodes = 25;  // by default, 5x5
  uint32_t sinkNode = 0;
  uint32_t sourceNode = 24;
  double interval = 1.0; // seconds
  bool verbose = false;
  bool tracing = false;
  SeedManager::SetSeed (10); // nastavit raz, neodporuca sa menit
  SeedManager::SetRun (1);   // pre zarucenie nezavislosti je lepsi
  CommandLine cmd;
  map<int, double> vysledkyUspesnotOdchylka;
  map<int, double> vysledkyUspesnot;
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
  wifiPhy.Set ("TxGain", DoubleValue (offset + Prss));
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

   /** Energy Model **/
   /***************************************************************************/
      /* energy source */
   BasicEnergySourceHelper basicSourceHelper;
      // configure energy source
       cout<<"End of energy source1"<<endl;
      basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1.0));
      // install source
       cout<<"End of energy source2"<<endl;
      EnergySourceContainer sources = basicSourceHelper.Install (senzorSiet);
      /* device energy model */
       cout<<"End of energy source3"<<endl;
      WifiRadioEnergyModelHelper radioEnergyHelper;
      // configure radio energy model
      radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
      radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0197));
       cout<<"End of energy source4"<<endl;
      // install device model
      DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (devices, sources);
      cout<<"End of energy source"<<endl;
      /* energy harvester */
      BasicEnergyHarvesterHelper basicHarvesterHelper;
      // configure energy harvester
      //EnergySourceContainer harveHosp = basicSourceHelper.Install (hosp);
      basicHarvesterHelper.Set ("PeriodicHarvestedPowerUpdateInterval", TimeValue (Seconds (harvestingUpdateInterval)));
      basicHarvesterHelper.Set ("HarvestablePower", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=0.1]"));
      // instalovanie harvester na polnohospodara 
      EnergyHarvesterContainer harvester = basicHarvesterHelper.Install (sources);
      /***************************************************************************/
      cout<<"End of hardvester"<<endl;

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (5.0),
                                 "MinY", DoubleValue (5.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);
    
    
  
    
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"),"Y", StringValue ("ns3::UniformRandomVariable[Min=20.0|Max=30.0]"));
    //mobility.SetMobilityModel ("ns3::RandomRectanglePositionAllocator");
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (10, 20, 10, 25)));
    
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

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (sinkNode), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (hosp.Get(sinkNode), tid);
  InetSocketAddress remote = InetSocketAddress (i.GetAddress (sinkNode, 0), 80);
  source->Connect (remote);



  // Give OLSR time to converge-- 30 seconds perhaps
  Simulator::Schedule (Seconds (15.0), &GenerateTraffic, 
                       source, packetSize, numPackets, interPacketInterval);

  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  Simulator::Stop (Seconds (16.0));
  
  AnimationInterface anim ("grid-olsr-animation-ps2.xml"); // Mandatory
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
  
   FlowMonitorHelper flowmon;
   Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  
  
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

