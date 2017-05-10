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


#include <iostream>
#include <cstdlib>
#include <string>

using namespace ns3;
using namespace std;




int main(int argc, char *argv[]){
    
    //Deklaracie premennych
    string phyMode ("DsssRate1Mbps");
    double Prss = -80;            // dBm
    double offset = 81;
    bool verbose = false;
    uint32_t numNodes = 25; //10x10
    // Energy Harvester variables
    double harvestingUpdateInterval = 1;  // seconds
    //double distance = 500;  // m
    
     // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
                      StringValue ("2200"));
     // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                      StringValue ("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

    
    //vytvorenie nodov
    NodeContainer senz;
    senz.Create(numNodes);
    
    //Polnohospodar
    NodeContainer hosp;
    hosp.Create(1);
    
    NodeContainer senzorSiet;
    senzorSiet.Add(senz);
    senzorSiet.Add(hosp);
    
    
    //Linkova vrstva
    //CsmaHelper csma;
    //csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    //csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
    
    //nainstalovanie tlinkovej vrstvy
    //NetDeviceContainer devices;
    //devices = csma.Install(senzorSiet);
    cout<<"End of nodeeee init"<<endl;
    WifiHelper wifi;
    if (verbose)
      {
        wifi.EnableLogComponents ();  // Turn on all Wifi logging
      }
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

    /** Wifi PHY **/
    /***************************************************************************/
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    wifiPhy.Set ("RxGain", DoubleValue (-10));
    wifiPhy.Set ("TxGain", DoubleValue (offset + Prss));
    wifiPhy.Set ("CcaMode1Threshold", DoubleValue (0.0));

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
     /** Internet stack **/
    InternetStackHelper internet;
    internet.Install (senzorSiet);
    //na komunikaciu bude potrebny ip adress helper
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);
    cout<<"End of IPv4"<<endl;
    
    /** Mobility **/
    
    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (senz);
    
    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (-50, 50, -25, 50)));
    mobility.Install (hosp);
    
    cout<<"End of mobility"<<endl;
    /** Energy Model **/
    /***************************************************************************/
    /* energy source */
    BasicEnergySourceHelper basicSourceHelper;
    // configure energy source
     cout<<"End of energy source1"<<endl;
    basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1.0));
    // install source
     cout<<"End of energy source2"<<endl;
    EnergySourceContainer sources = basicSourceHelper.Install (senz);
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
    
    Simulator::Stop (Seconds (33.0));
    
    //zobrazenie simulacie
    AnimationInterface anim ("My-animation-zadanie2.xml"); // Mandatory
     for (uint32_t i = 0; i < senz.GetN (); ++i)
      {
         anim.UpdateNodeDescription (senz.Get (i), "Senz"); // Optional
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
