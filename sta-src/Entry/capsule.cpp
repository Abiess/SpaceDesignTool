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
------------------ Author:       Marnix Volckaert               -------------------
------------------ Affiliation:  TUDelft                        -------------------
-----------------------------------------------------------------------------------
 Modified by Tiziana Sabatini on May-July 2009
*/


#include "capsule.h"
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QProcess>
#include "Scenario/propagationfeedback.h"


using namespace std;

//-------------------------------------------------------------------------------------------------
void capsule_class::selectCdCprofile(QString name) {

    QString path = QString("data/aerodynamics/");
    path.append(name);

    //QTextStream out (stdout); out << "===> aerodynamics path: " << path << endl;

    QFile cprofile(path);

    if (!cprofile.exists() || name == "")
        flag = 1;

    cprofile.open(QIODevice::ReadOnly);
    QTextStream cprofilestream(&cprofile);
    //Code modified by Dominic to read files in ASTOS format
    double temp;
    NdatapointsC = 0;                //initialise counter to keep track of number of lines
    //------
    while (!cprofilestream.atEnd())
    {
        cprofilestream >> temp;      //read in a line just to count it
        NdatapointsC++;                      //increase the counter
    }
    NdatapointsC/=2;
    //------ Allocate the memory for the data arrays ------
    MachC_array = new double[NdatapointsC];
    Cdc_array = new double[NdatapointsC];
    //------ Close and open the file again, workaround to start reading at the beginning of the file (moving the pointer did not work) ------
    cprofile.close();

    cprofile.open(QIODevice::ReadOnly);

    //------ Fill up the data arrays ------
    for (int i=0; i<NdatapointsC; i++) {
        cprofilestream >> MachC_array[i];
    }
    for (int i=0; i<NdatapointsC; i++) {
        cprofilestream >> Cdc_array[i];
    }
    //------ Close the data file ------
    cprofile.close();
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void capsule_class::selectCdPprofile(QString name) {

    //QString path = "C:/Users/Tizy/Desktop/Code/trunk/sta-src/Entry/data/aerodynamics/";

    QString path = QString("data/aerodynamics/");
    path.append(name);

    //QTextStream out (stdout); out << "===> aerodynamics 2 path: " << path << endl;

    QFile pprofile(path);

    if (!pprofile.exists() || name == "")
        flag = 2;

    pprofile.open(QIODevice::ReadOnly);
    QTextStream pprofilestream(&pprofile);
    NdatapointsP = 0;                //initialise counter to keep track of number of lines

    //------
    while (!pprofilestream.atEnd())
    {
        pprofilestream.readLine();      //read in a line just to count it
        NdatapointsP++;                      //increase the counter
    }

    //NdatapointsP--;                  //The loop ends after 1 extra count, we need to subtract by 1
    //------ Allocate the memory for the data arrays ------
    MachP_array = new double[NdatapointsP];
    Cdp_array = new double[NdatapointsP];
    //------ Close and open the file again, workaround to start reading at the beginning of the file (moving the pointer did not work) ------
    pprofile.close();
    pprofile.open(QIODevice::ReadOnly);
    //------ Fill up the data arrays ------
    for (int i=0; i<NdatapointsP; i++) {
        pprofilestream >> MachP_array[i];
        pprofilestream >> Cdp_array[i];
    }
    //------ Close the data file ------
    pprofile.close();
}
//-------------------------------------------------------------------------------------------------


void capsule_class::selectClCprofile(QString name) {//Added by Dominic to include liftforce, should be generalized, so that only a single function is needed for all coefs

    QString path = QString("data/aerodynamics/");
    path.append(name);

    //QTextStream out (stdout); out << "===> aerodynamics path: " << path << endl;

    QFile cprofile(path);

    if (!cprofile.exists() || name == "")
        flag = 1;

    cprofile.open(QIODevice::ReadOnly);
    QTextStream cprofilestream(&cprofile);
    //Code modified by Dominic to read files in ASTOS format
    double temp;
    NdatapointsC = 0;                //initialise counter to keep track of number of lines
    //------
    while (!cprofilestream.atEnd())
    {
        cprofilestream >> temp;      //read in a line just to count it
        NdatapointsC++;                      //increase the counter
    }
    NdatapointsC/=2;
    //------ Allocate the memory for the data arrays ------
    MachC_array = new double[NdatapointsC];
    Clc_array = new double[NdatapointsC];
    //------ Close and open the file again, workaround to start reading at the beginning of the file (moving the pointer did not work) ------
    cprofile.close();

    cprofile.open(QIODevice::ReadOnly);

    //------ Fill up the data arrays ------
    for (int i=0; i<NdatapointsC; i++) {
        cprofilestream >> MachC_array[i];
    }
    for (int i=0; i<NdatapointsC; i++) {
        cprofilestream >> Clc_array[i];
    }
    //------ Close the data file ------
    cprofile.close();
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void capsule_class::selectCsCprofile(QString name) {//Added by Dominic to include sideforce, should be generalized, so that only a single function is needed for all coefs

    QString path = QString("data/aerodynamics/");
    path.append(name);

    //QTextStream out (stdout); out << "===> aerodynamics path: " << path << endl;

    QFile cprofile(path);

    if (!cprofile.exists() || name == "")
        flag = 1;

    cprofile.open(QIODevice::ReadOnly);
    QTextStream cprofilestream(&cprofile);
    //Code modified by Dominic to read files in ASTOS format
    double temp;
    NdatapointsC = 0;                //initialise counter to keep track of number of lines
    //------
    while (!cprofilestream.atEnd())
    {
        cprofilestream >> temp;      //read in a line just to count it
        NdatapointsC++;                      //increase the counter
    }
    NdatapointsC/=2;
    //------ Allocate the memory for the data arrays ------
    MachC_array = new double[NdatapointsC];
    Csc_array = new double[NdatapointsC];
    //------ Close and open the file again, workaround to start reading at the beginning of the file (moving the pointer did not work) ------
    cprofile.close();

    cprofile.open(QIODevice::ReadOnly);

    //------ Fill up the data arrays ------
    for (int i=0; i<NdatapointsC; i++) {
        cprofilestream >> MachC_array[i];
    }
    for (int i=0; i<NdatapointsC; i++) {
        cprofilestream >> Csc_array[i];
    }
    //------ Close the data file ------
    cprofile.close();
}

//-------------------------------------------------------------------------------------------------
double capsule_class::cdc (double M)
{
    double Cdc_int = 0.0;      //interpolated cd value
    //------ if Mach is below lowest data point -----------------------------
    if (MachC_array[0] > M) {
        return(Cdc_array[0]);
    }
    //------ if Mach is above highest data point ------
    else if (M > MachC_array[NdatapointsC-1]) {
        return(Cdc_array[NdatapointsC-1]);
    }
    else {
        //------ Find the data point higher and lower than the Mach value ------
        for (int i=1; i<NdatapointsC; i++) {
            if (MachC_array[i] > M) {
                highbound = i;
                lowbound = i-1;
                break;
            }
        }
    }
    //------ make the interpolation ------
    Cdc_int = Cdc_array[lowbound] + ( (Cdc_array[highbound] - Cdc_array[lowbound])/(MachC_array[highbound] - MachC_array[lowbound]) ) * (M - MachC_array[lowbound]);
    return (Cdc_int);
}
//-------------------------------------------------------------------------------------------------
double capsule_class::clc (double M)
{
    double Clc_int = 0.0;      //interpolated cd value
    //------ if Mach is below lowest data point -----------------------------
    if (MachC_array[0] > M) {
        return(Clc_array[0]);
    }
    //------ if Mach is above highest data point ------
    else if (M > MachC_array[NdatapointsC-1]) {
        return(Clc_array[NdatapointsC-1]);
    }
    else {
        //------ Find the data point higher and lower than the Mach value ------
        for (int i=1; i<NdatapointsC; i++) {
            if (MachC_array[i] > M) {
                highbound = i;
                lowbound = i-1;
                break;
            }
        }
    }
    //------ make the interpolation ------
    Clc_int = Clc_array[lowbound] + ( (Clc_array[highbound] - Clc_array[lowbound])/(MachC_array[highbound] - MachC_array[lowbound]) ) * (M - MachC_array[lowbound]);
    return (Clc_int);
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
double capsule_class::csc (double M)
{
    double Csc_int = 0.0;      //interpolated cd value
    //------ if Mach is below lowest data point -----------------------------
    if (MachC_array[0] > M) {
        return(Csc_array[0]);
    }
    //------ if Mach is above highest data point ------
    else if (M > MachC_array[NdatapointsC-1]) {
        return(Csc_array[NdatapointsC-1]);
    }
    else {
        //------ Find the data point higher and lower than the Mach value ------
        for (int i=1; i<NdatapointsC; i++) {
            if (MachC_array[i] > M) {
                highbound = i;
                lowbound = i-1;
                break;
            }
        }
    }
    //------ make the interpolation ------
    Csc_int = Csc_array[lowbound] + ( (Csc_array[highbound] - Csc_array[lowbound])/(MachC_array[highbound] - MachC_array[lowbound]) ) * (M - MachC_array[lowbound]);
    return (Csc_int);
}
//-------------------------------------------------------------------------------------------------
double capsule_class::cdp (double M) {

    double Cdp_int = 0.0;      //interpolated cd value
    //------ if Mach is below lowest data point -----------------------------
    if (MachP_array[0] > M) {
        return(Cdp_array[0]);
    }
    //------ if Mach is above highest data point ------
    else if (M > MachP_array[NdatapointsP-1]) {
        return(Cdp_array[NdatapointsP-1]);
    }
    else {
        //------ Find the data point higher and lower than the Mach value ------
        for (int i=1; i<NdatapointsP; i++) {
            if (MachP_array[i] > M) {
                highbound = i;
                lowbound = i-1;
                break;
            }
        }
    }
    //------ make the interpolation ------
    Cdp_int = Cdp_array[lowbound] + ( (Cdp_array[highbound] - Cdp_array[lowbound])/(MachP_array[highbound] - MachP_array[lowbound]) ) * (M - MachP_array[lowbound]);
    return (Cdp_int);
}
//-------------------------------------------------------------------------------------------------

