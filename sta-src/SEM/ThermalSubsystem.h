/*
 This program is free software; you can redistribute it and/or modify it under
 the terms of the European Union Public Licence - EUPL v.1.1 as published by
 the European Commission.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the European Union Public Licence - EUPL v.1.1
 for more details.

 You should have received a copy of the European Union Public Licence - EUPL v.1.1
 along with this program.

 Further information about the European Union Public Licence - EUPL v.1.1 can
 also be found on the world wide web at http://ec.europa.eu/idabc/eupl
 */

/*
 ------ Copyright (C) 2010 STA Steering Board (space.trajectory.analysis AT gmail.com) ----
 ------------------ Author: Ozgun YILMAZ      ---------------------------------------------
 ------------------ email: ozgunus@yahoo.com  ---------------------------------------------

 */

#ifndef THERMALSUBSYSTEM_H
#define THERMALSUBSYSTEM_H

#include <QString>
using namespace std;


enum ThermalControlType{NoControl,
                        Passive,
                        Active,
                        PassiveAndActive};

//------------------------- Structures ---------------------------------//
struct PlanetThermalInfo
{
    QString Planet;
    double SolarConstantOfPlanet;
    double AlbedoConstantOfPlanet;
    double CollimatedSolarEnergyOfPlanet;
    double PlanetIREnergy;
    double PlanetAngularRadius;
};

struct CoatingProperties
{
    QString Type;
    double Emmissivity;
    double Absorptivity;
};

struct PayloadThermalInfo
{
    QString Name;
    double MinimumTemperature;
    double MaximumTemperature;
    bool   Alert;
};

struct SCStructure
{
    QString Name;
    double Emmissivity;
    double Absorptivity;
};

class ThermalSubsystem
{
public:
    //----------------- Constructor -------------------------------------//
    ThermalSubsystem();
    //----------------- Functions ---------------------------------------//
    /**
      * It will set the planet properties that are needed for thermal
      * calculations.
      * @return SolarConstantOfPlanet          kW/m2
      * @return AlbedoConstantOfPlanet         no unit
      *--- @return CollimatedSolarEnergyOfPlanet  no unit (function of angular
      *                                       radius)
      * @return PlanetIREnergy                 W/m2
      *--- @return PlanetAngularRadius            arcsin(R_planet/(altitude + R_planet))
     */
    //It has to be fixed for collimated energy and angular radius
    void setPlanetThermalProperties(QString PName,
                                    double AngularRadius,
                                    double SolarConstant,
                                    double AlbedoConstant,
                                    double IREnergy,
                                    double collimatedSolarEnergy);

    PlanetThermalInfo getPlanetThermalProperties();

    //Fills the ColdFaceCoatingPorperties according to selected name
    //"ColdFaceCoating" from the database, if it is new defined second
    //call has to be made with additional Emmissivity and Absorptivity
    //values
    void setColdFaceCoatingProperties(QString Coating);
    void setColdFaceCoatingProperties(QString Coating,
                                      double Emmissivity,
                                      double Absorptivity);

    CoatingProperties getColdFaceCoatingProperties();

    //Fills the HotFaceCoatingPorperties according to selected name
    //"HotFaceCoating" from the database, if it is new defined second
    //call has to be made with additional Emmissivity and Absorptivity
    //values
    void setHotFaceCoatingProperties(QString Coating);
    void setHotFaceCoatingProperties(QString Coating,
                                     double Emmissivity,
                                     double Absorptivity);

    CoatingProperties getHotFaceCoatingProperties();

    void CalculateAndSetSolarFluxHeat();
    double getSolarFluxHeat();

    void CalculateAndSetAlbedoHeat();
    double getAlbedoHeat();

    void CalculateAndSetPlanetIRHeat();
    double getPlanetIRHeat();

    void setMinimumSCElectronicsHeatInEclipse(double SCElectronicsHeat);
    double getMinimumSCElectronicsHeatInEclipse();

    void setMaximumSCElectronicsHeatInDaylight(double SCElectronicsHeat);
    double getMaximumSCElectronicsHeatInDaylight();

    void setMinimumSCElectronicsHeatInDaylight(double SCElectronicsHeat);
    double getMinimumSCElectronicsHeatInDaylight();

    void setMaximumSCElectronicsHeatInEclipse(double SCElectronicsHeat);
    double getMaximumSCElectronicsHeatInEclipse();

    void CalculateAndSetSCTemperatureRange();
    void setMinimumSCTemperature(double temperature);
//    void CalculateAndSetMinimumSCTemperature();
    double getMinimumSCTemperature();

//    void CalculateAndSetMaximumSCTemperature();
    void setMaximumSCTemperature(double temperature);
    double getMaximumSCTemperature();

    void setMinimumSCTempWithRadiatorOrHeater(double temp);
    void setMaximumSCTempWithRadiatorOrHeater(double temp) ;

    double getMinimumSCTempWithRadiatorOrHeater() ;
    double getMaximumSCTempWithRadiatorOrHeater() ;

    void setMinimumPayloadTemperature(int Index,
                                      QString Name,
                                      double MinTemperature);
    void setMaximumPayloadTemperature(int Index,
                                      QString Name,
                                      double MaxTemperature);
    PayloadThermalInfo * getPayloadTemperature();

    /**
      * Calculates the needed heater to reach the desired temperature
      * limit. SC temperature has to be calculated before
      * @param DesiredMinimumTemperature Minimum temperature limit that
      *                                  user defined for SC
     */
    void CalculateAndSetNeededHeater();
    void setNeededHeaterToZero();
    void setNeededHeater(double heater);
    double getNeededHeater();

    void CreateTemperatureTimeFunction();

    /**
      * Calculates the needed heater to reach the desired temperature
      * limit. SC temperature has to be calculated before
      * @param DesiredMaximumTemperature Maximum temperature limit that
      *                                  user defined for SC
     */
    void CalculateAndSetNeededRadiator();
    void setNeededRadiatorToZero();
    void setNeededRadiator(double radiator);
    double getNeededRadiator();

    bool IsPayloadInAlert(int Index);

    void setAreaOfHotFace(double Area);
    double getAreaOfHotFace();

    void setAreaOfColdFace(double Area);
    double getAreaOfColdFace();

    void setTotalAreaOfHotFace(double Area);
    double getTotalAreaOfHotFace();

    void setTotalAreaOfColdFace(double Area);
    double getTotalAreaOfColdFace();

    void setSCStructuralProperties(QString Name,
                                   double Emmissivity,
                                   double Absorptivity);
    SCStructure * getSCStructuralProperties();

    //set the payload mass the mass of thermal is a function of Payload
    void setThermalSubsystemMass(double PayloadMass);
    double getThermalSubsystemMass();

    void setThermalSubsystemVolume(double Volume);
    double getThermalSubsystemVolume();

    void setThermalSubsystemPower(double Power);
    double getThermalSubsystemPower();

private:
    //----------------- Member objects ----------------------------------//
    double AreaOfHotFace;
    double AreaOfColdFace;
    double TotalAreaOfHotFace;
    double TotalAreaOfColdFace;

    double SolarFluxHeat;
    double AlbedoHeat;
    double ReceivedPlanetIRHeat;
    double MinimumSCElectronicsHeatInEclipse;
    double MaximumSCElectronicsHeatInDaylight;
    double MinimumSCElectronicsHeatInDaylight;
    double MaximumSCElectronicsHeatInEclipse;

    double MinimumSCTemperature;
    double MaximumSCTemperature;

    double MinimumSCTempWithRadiatorOrHeater ;
    double MaximumSCTempWithRadiatorOrHeater ;

    double DesiredMinimumTemperature;
    double DesiredMaximumTemperature;

    double NeededHeater;
    double NeededRadiator;

    double ThermalSubsystemMass;
    double ThermalSubsystemVolume;
    double ThermalSubsystemPower;

    ThermalControlType ThermalControl;

    SCStructure SCStructuralProperties;

    PlanetThermalInfo PlanetThermalProperties;
    CoatingProperties ColdFaceCoatingProperties;
    CoatingProperties HotFaceCoatingProperties;

    PayloadThermalInfo PayloadThermalDetails[4];

};
//-------------- End of ThermalSubsystem class definition -----------------//

#endif // THERMALSUBSYSTEM_H
