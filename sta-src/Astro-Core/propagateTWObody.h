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
//------------------ Author:       Guillermo Ortega               -------------------
//------------------ Affiliation:  European Space Agency (ESA)    -------------------
//-----------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////////////
// Description: 
//      This function propagates spacecraft Keplerian 
//      orbital elements thought an interval of time
//      deltat and obtains the corresponding cartesian 
//      orbital elements at that time 
//
// Input:
//		mu		standard gravitational parameter of the planet/moon 
//		a       semi-major axis (km)
//      ec      eccentricity (-)
//      i       inclination (rad)
//      w0      argument of the perigee (rad)
//      o0      right ascention of the ascending node (rad)
//      m0      mean anomaly (rad)
//      deltat  propagation accuracy interval (s)
//
// Output:
//		pos    xyz coordinates in ECI system (km)
//      vel    vxvyvz coordinates in ECI system (km/s)
//      ran1   RAAN at time=deltat (deg)
//      ap1    argument of perigee at time=deltat (deg)
//      ma1    Mean anomaly at time=deltat (deg)
//	
// Example of call:
//      propagateTWObody(398601.3,500,0,56,0,0,0,500,x,y,z,xd,yd,zd,ran1,ap1,ma1);
//
// Date: October 2006
// Version: 1.0
// Change history:
//
//////////////////////////////////////////////////////////////////////////////////////

#ifndef _ASTROCORE_PROPAGATE_TWO_BODY_H_
#define _ASTROCORE_PROPAGATE_TWO_BODY_H_

#include "statevector.h"

sta::StateVector propagateTWObody(double mu,
                                  double a, double ec, double i, double w0, double o0, double m0,
                                  double deltat,
                                  double& ran1, double& ap1, double& ma1);

// The constant of proportionality is called G, the gravitational constant, the universal gravitational constant, 
// Newton's constant, and colloquially big-G. The gravitational constant is a physical constant which appears 
// in Newton's law of universal gravitation and in Einstein's theory of general relativity.
// G = 6.6742e-11 N m^2 kg^-2
// The  G*M (being M the pass of the Earth) product is the standard gravitational parameter, according to the 
// case also called the geocentric or heliocentric gravitational constant, among others. This gives a convenient 
// simplification of various gravity-related formulas. Also, for the Earth and the Sun, the value of the product 
// is known more accurately than each factor. 
// More information can be foun in http://en.wikipedia.org/wiki/Gravitational_constant
// G*Mass_earth = 398600.4418 km^3 s^-2
// The following solar system bodies are considered in this software
// Body		mu (km3s-2)
// Sun		132712,440018
// Mercury	22,032
// Venus	324859
// Earth	398601.3  //exact
// Mars		42828
// Ceres	63
// Jupiter	126686534
// Saturn	37931187
// Uranus	5793947
// Neptune	6836529
// Pluto	1001
// 
// 
// 

/**
 * Function: Computes Universal coefficients needed for the Kepler propagator
 * Source: Vallado, Fundamentals of Astrodynamics and Applications
 */
void computeCoefficients(double psi, double& c2, double& c3);

/**
 * Function: Propagates the trajectory, by solving Kepler's problem. (Two body propagator)
 * Source: Vallado, Fundamentals of Astrodynamics and Applications
 * @param mu The central body's gravity parameter.
 * @param initialState The state vector before propagation
 * @param dt The timestep [s]
 * @return The propagated state vector
 */
sta::StateVector propagateKEPLER(double mu, sta::StateVector initialState, double dt);

#endif // _ASTROCORE_PROPAGATE_TWO_BODY_H_













