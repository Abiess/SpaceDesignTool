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
*/

/*
 ------------------ Author:       Tiziana Sabatini               -------------------
 ------------------ Affiliation:  Politecnico di Milano          -------------------
 -----------------------------------------------------------------------------------
  Created July 2009
*/


#ifndef REENTRYSTRUCTURES_H_INCLUDED
#define REENTRYSTRUCTURES_H_INCLUDED

#include <QString>
#include <Eigen/Core>
#include "Scenario/scenario.h"
class ScenarioReEntryTrajectory;

//USING_PART_OF_NAMESPACE_EIGEN

/**
 * EntrySettings structure.
 */
struct EntrySettings
{
    QString bodyname;
    QString modelname;
    QString CdCprofilename;
    QString ClCprofilename;
    QString CsCprofilename;

    QString CdPprofilename;
    QString integrator;
    QString propagator;
    double stepsize;
    double maxloadfactor;
    double maxheatrate;
    double maxaltitude;
    double parachuteArea;
    double parachuteDeployMach;
};

/**
 * EntryParameters structure.
 */
struct EntryParameters
{
    int coordselector;  //1 for spherical, 2 for cartesian coordinates
    //Uncertainties_struct uncertainties;
    double Sref;
    double cref;
    double m;
    double Rn;          //nose radius
    double inputstate[6];
};

/**
 * enum _status, to represent the capsule status during the trajectory.
 */
enum _status
{
    landed, OK, crushed, burnt, gone
};

/**
 * Peak structure.
 */
struct peak
{
        double value;
        double time;
        double altitude;
};

/**
 * Endstate structure.
 */
struct Endstate
{
    double longitude, latitude;
    peak maxconvheatrate;
    peak maxradheatrate;
    peak maxtotalheatrate;
    peak maxloadfactor;
    double Machone_transition_time;
    double Machone_transition_altitude;
    double parachutedeploy_time;
    double parachutedeploy_altitude;
    double totalheatload;
    double tpsmass;
    double timeofflight;
    double impactvelocity;
    _status status;      //0 = landed, 1 = OK, 2 = crushed, 3 = burnt, 4 = gone
};

/**
 * angle structure.
 */
struct angle
{
    int degrees;
    int minutes;
    double seconds;
};


///**
// * Uncertainties_struct structure.
// */
//struct Uncertainties_struct {
//    double mass;
//    double altitude;
//    double tau;
//    double delta;
//    double velocity;
//    double gamma;
//    double chi;
//    double density;
//    double temperature;
//    double pressure;
//    double speedofsound;
//    double cdcapsule;
//    double cdparachute;
//};

//struct interval
//{
//    double low;
//    double high;
//};

//struct EntryParameterIntervals
//{
//    interval K;
//    interval Rn;
//    interval velocity;
//    interval gamma;
//    interval chi;
//};


/**
 * Return an EntrySettings structure given the re-entry scenario and the vehicle features.
 */
EntrySettings createEntrySettings(ScenarioEntryArcType* entry, ScenarioREV* vehicle);

/**
 * Return an EntryParameters structure for the simulation mode given the re-entry scenario and the vehicle features.
 */
EntryParameters createEntryParametersSimulation(ScenarioEntryArcType* entry, ScenarioREV* vehicle);


//The following functions are disabled because they refer to re-entry modes other than simulation

///**
// * Return an EntryParameterIntervals structure given the re-entry scenario features.
// */
//EntryParameterIntervals createEntryParameterIntervals(ScenarioReEntryTrajectory* scenarioReentry);

///**
// * Return an EntryParameters structure for the window analysis mode given the re-entry scenario and the parameters intervals.
// */
//EntryParameters createEntryParametersWindow(ScenarioReEntryTrajectory* scenarioReentry, EntryParameterIntervals parameterIntervals);

///**
// * Return an Uncertainties_struct given the re-entry scenario features.
// */
//Uncertainties_struct createUncertainties_struct(ScenarioReEntryTrajectory* scenarioReentry);

//    angle createTargetAngleLatitude(ScenarioReEntryTrajectory* scenarioReentry);
//    angle createTargetAngleLongitude(ScenarioReEntryTrajectory* scenarioReentry);
//
//    angle createTargetAngleLatitudeWindow(ScenarioReEntryTrajectory* scenarioReentry);
//    angle createTargetAngleLongitudeWindow(ScenarioReEntryTrajectory* scenarioReentry);
//




#endif // REENTRYSTRUCTURES_H_INCLUDED

