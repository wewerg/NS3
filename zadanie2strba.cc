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
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/netanim-module.h"
#include "ns3/net-device-container.h"
#include "ns3/csma-helper.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/config-store-module.h"
#include "ns3/energy-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"


#include <iostream>
#include <cstdlib>
#include <string>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("Zadanie2PS2");

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


int main(int argc, char *argv[]){
    
    //Deklaracie premennych
    string phyMode ("DsssRate1Mbps");
    //uint32_t nWifi = 20;
    double Prss = -80;            // dBm
    double offset = 81;
    bool verbose = false;
    uint32_t numNodes = 25; //10x10
    // Energy Harvester variables
    double harvestingUpdateInterval = 1;  // seconds
    //double distance = 500;  // m
    
    //vytvorenie nodov
    NodeContainer senz; // =wifiStaNodes
    senz.Create(numNodes);
    
    //Polnohospodar
    NodeContainer hosp;// = wifiApNode
    hosp.Create(1);
    
    NodeContainer senzorSiet; //= allNodes;
    senzorSiet.Add(senz);
    senzorSiet.Add(hosp);

    WifiHelper wifi;
    if (verbose)
      {
        wifi.EnableLogComponents ();  // Turn on all Wifi logging
      }

    /** Wifi PHY **/
    /***************************************************************************/
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    wifiPhy.Set ("RxGain", DoubleValue (-10));
    //wifiPhy.Set ("TxGain", DoubleValue (offset + Prss));
    //wifiPhy.Set ("CcaMode1Threshold", DoubleValue (0.0));


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

    
    /** Mobility **/
    
    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (10.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (senz);
    
    mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator","X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=5.0]"),"Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=30.0]"));
    //mobility.SetMobilityModel ("ns3::RandomRectanglePositionAllocator");
    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (-50, 50, -20, 50)));
    mobility.Install (hosp);
//    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (-50, 50, -25, 50)));
//    mobility.Install (hosp);
    
    cout<<"End of mobility"<<endl;
    
    /** Internet stack **/
    InternetStackHelper internet;
    internet.Install (senzorSiet);
    cout<<"End of IPv4 stack"<<endl;
    /** Internet addresses **/
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);
    
    cout<<"End of IPv4 address"<<endl;   
    
    
    /** Install Application **/
    UdpEchoServerHelper echoServer (9);
    ApplicationContainer serverApps = echoServer.Install (hosp);
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (15.0));
    cout<<"End of instal App server"<<endl;    
    UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (10));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps = echoClient.Install (senz);
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (15.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    
    cout<<"End of APPlication"<<endl;
    
    Simulator::Stop (Seconds (15.0));
    
    //zobrazenie simulacie
    AnimationInterface anim ("My-animation-zadanie2.xml"); // Mandatory
     for (uint32_t i = 0; i < senz.GetN (); ++i)
      {
         ostringstream stringStream;
         stringStream << "Senz" << i;
         anim.UpdateNodeDescription (senz.Get (i), stringStream.str()); // Optional
         anim.UpdateNodeColor (senz.Get (i), 255, 0, 0); // Optional
       }
    anim.UpdateNodeDescription (hosp.Get (0), "Hosp"); // Optional
    anim.UpdateNodeColor (hosp.Get (0), 0, 255, 0); // Optional
    anim.EnablePacketMetadata (); // Optional
    anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
    anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
    anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional
    
    
    
    
    Simulator::Run ();
    Simulator::Destroy ();
    
    cout<<"End of simulation"<<endl;
   
    return 0;
}
