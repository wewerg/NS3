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

#include<iostream>
#include <cstdlib>

using namespace ns3;
using namespace std;




int main(int argc, char *argv[]){
    
    //slideshare.net/rahulhada
    //Deklaracie premennych
    uint32_t numNodes = 25; //10x10
    //double distance = 500;  // m
    //double distanceToRx = 100.0; //m
    
    
    //vytvorenie nodov
    NodeContainer rastliny;
    rastliny.Create(numNodes);
    
    //Polnohospodar
    NodeContainer hospodar;
    hospodar.Create(1);
    
    NodeContainer sietNodov;
    sietNodov.Add(rastliny);
    sietNodov.Add(hospodar);
    
    
    //vytvorenie technologie
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
    
    //nainstalovanie technologie na device
    NetDeviceContainer devices;
    devices = csma.Install(sietNodov);
    
    //na komunikaciu bude potrebny ip adress helper
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0","255.255.255.0");
    //Ipv4InterfaceContainer interfaces = address.Assign(rastliny);
    
    //nakoniec nainstalovanie nejakej aplikacie
    
    
    
    NodeContainer networkNodes;
    for (uint32_t i = 0; i< numNodes;i++)
        networkNodes.Add (rastliny.Get (i));
  
    
    
    //Mobility helper
    
    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (5.0),
                                 "MinY", DoubleValue (5.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (rastliny);
    
    
  
    
    mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"),"Y", StringValue ("ns3::UniformRandomVariable[Min=-20.0|Max=20.0]"));
    //mobility.SetMobilityModel ("ns3::RandomRectanglePositionAllocator");
    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (-100, 200, -100, 200)));
    
    mobility.Install (hospodar);
 
 
    
    
    Simulator::Stop (Seconds (33.0));
    
    //zobrazenie simulacie
    AnimationInterface anim ("my-animation-harvesting.xml"); // Mandatory
     for (uint32_t i = 0; i < rastliny.GetN (); ++i)
      {
         anim.UpdateNodeDescription (rastliny.Get (i), "Zelenina"); // Optional
         anim.UpdateNodeColor (rastliny.Get (i), 255, 0, 0); // Optional
       }
    
      anim.UpdateNodeDescription (hospodar.Get (0), "Hospodar"); // Optional
      anim.UpdateNodeColor (hospodar.Get (0), 0, 255, 0); // Optional
    
     
     
     anim.EnablePacketMetadata (); // Optional
     //anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
     //anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
     //anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional
     
    
    
    
    
    Simulator::Run ();
    Simulator::Destroy ();
    
    //int res=system("/home/student/Documents/ns3/netanim-3.106/NetAnim");
    std::cout<<"End of simulation"<<std::endl;
    
    
    return 0;
}
