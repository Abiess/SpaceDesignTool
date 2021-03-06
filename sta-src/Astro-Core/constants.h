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


//
//                             CONSTANTS and SCCALE FACTORS
// The European Space Agency Constants Lists. The list is divided into several categories:
// Mathematical, Phisical, and String constants.
// All constants are given in the International Unit System MKS.
// The file contains also scale factor between units.
//
// The authoritative sources for physical constants are the 2002 CODATA recommended values,
// published in the web sites:
//   http://www.physics.nist.gov/cuu/Constants/index.html
//   http://physics.nist.gov/Pubs/SP811/appenB9.html
//
//
//
//------------------ Author:       Guillermo Ortega               -------------------
//------------------ Affiliation:  European Space Agency (ESA)    -------------------
// Date: May 21st 2009
// Modification history goes here
//-----------------------------------------------------------------------------------


#ifndef CONSTANTS_H
#define CONSTANTS_H


// Mathematical constants

        const double mypi = 3.141592653589793238462643383279502884197169399375;

        const double TWOPI = 3.141592653589793238462643383279502884197169399375 * 2.0;

        const double HALFPI = 3.141592653589793238462643383279502884197169399375 / 2.0;

        const double RAD2DEG = 180.0 / 3.141592653589793238462643383279502884197169399375;

        const double DEG2RAD = 3.141592653589793238462643383279502884197169399375 / 180.0;

        const double MINUTE = 6e1;                  // s

        const double HOUR = 3.6e3;                  // s

        const double DAY = 8.64e4;                  // s

        const double WEEK = 6.048e5;                // s

        const double INCH = 2.54e-2;                // m

        const double FOOT = 3.048e-1;               // m

        const double YARD = 9.144e-1;               // m

        const double MILE = 1.609344e3;             // m

        const double NAUTICAL_MILE = 1.852e3;       // m

        const double MICRON = 1e-6;                 // m

        const double ANGSTROM = 1e-10;              // m

        const double HECTARE = 1e4;                 // m^2

        const double ACRE = 4.04685642241e3;        // m^2

        const double CALORIE = 4.1868e0;            // kg m^2 / s^2

        const double BTU = 1.05505585262e3;         // kg m^2 / s^2

        const double BAR = 1e5;                     // kg / m s^2

        const double STD_ATMOSPHERE = 1.01325e5;    // kg / m s^2

        const double PSI = 6.89475729317e3;         // kg / m s^2


// Scale factors

        const double YOTTA = 1e24;

        const double ZETTA = 1e21;

        const double EXA = 1e18;

        const double PETA = 1e15;

        const double TERA = 1e12;

        const double GIGA = 1e9;

        const double MEGA = 1e6;

        const double KILO = 1e3;

        const double MILLI = 1e-3;

        const double MICRO = 1e-6;

        const double NANO = 1e-9;

        const double PICO = 1e-12;

        const double FEMTO = 1e-15;

        const double ATTO = 1e-18;

        const double ZEPTO = 1e-21;

        const double YOCTO = 1e-24;



// Physical constants

        const double Mol_Weight_SL_Earth = 28.9644e-3;        // (kg/mole)

        const double RADIATION_CONSTANT = 5.67032e-08;        // (W/m^2 K^4)

        const double ZERO_KELVIN = 273.16;                    // (K)

        const double SPEED_OF_LIGHT = 299792458.0;            // (+/- 1 m/s)

        const double PLANCK_CONSTANT = 6.62606896e-34;        // kg m^2 / s   Planck’s constant, h

        const double ASTRONOMICAL_UNIT = 1.49597870691e11;     // m

        const double LIGHT_YEAR = 9.46053620707e15;            // m

        const double NUM_AVOGADRO = 6.02214199e23;             // 1 / mol

        const double PARSEC = 3.08567758135e16;                // m

        const double ELECTRON_VOLT =1.602176487e-19;           // kg m^2 / s^2

        const double MASS_ELECTRON = 9.10938188e-31;           // kg

        const double BOLTZMANN_CONSTANT = 1.3806504e-23;       // kg m^2 / K s^2  The Boltzmann constant, k.

        const double STEFAN_BOLTZMANN_CONSTANT = 5.67040047374e-8; // kg / K^4 s^3

        const double MOLAR_GAS_CONSTANT = 8.314472e0;          // kg m^2 / K mol s^2     The molar gas constant, R0

        const double STANDARD_GAS_VOLUME = 2.2710981e-2;       // m^3 / mol   The standard gas volume, V

        const double ELECTRON_CHARGE = 1.602176487e-19;        // A s

        const double EARTH_GRAVITY_ZERO = 9.80665;             // (m/s**2) earth sea level and 45 deg northern latitude

        const double GRAVITATIONAL_CONSTANT = 6.67428e-11;    // m^3 / kg s^2


// String constants

        const char QUOTE[] = "''";


#endif // CONSTANTS_H
