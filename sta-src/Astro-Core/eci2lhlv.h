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
 ------------------ Author: Annalisa Riccardi   -------------------------------------------------
 ------------------ E-mail: nina1983@gmail.com  ----------------------------
 Modified in July 2011 by J.Alonso to add lhlv methods needed in RendezVous
 */
#include "Astro-Core/statevector.h"
#include "cartesianTOorbital.h"

/**
 * This function converts inertial planet-centered coordinates of position into spacecraft local
 * orbit frame
 * Input:
 *               xInertial				coordinate X in ECI (km)
 *               yInertial				coordinate Y in ECI (km)
 *               zInertial				coordinate Z in ECI (km)
 *               i					inclination (degree)
 *               Omega				        RAAN (degree)
 *               t					true anomaly (degree)
 *               w					argument of the perigee (degree)
 *
 * Output:
 *               xlocal					coordinate X in LHLV (km)
 *               ylocal					coordinate Y in LHLV (km)
 *               zlocal					coordinate Z in LHLV (km)
 */
void inertialTOlocal(double xInertial, double yInertial, double zInertial,
                double i, double Omega, double t, double w, double& xlocal, double& ylocal, double& zlocal);


//ALONSO.- inverse transformation
void localTOinertial(double xInertial, double yInertial, double zInertial,
                double i, double Omega, double t, double w, double& xlocal, double& ylocal, double& zlocal);


//ALONSO.- Needed for lhlv. Renaming axis, no more.
void lhlvTOlocal(double xLHLV,double yLHLV,double zLHLV, double& xLocal,double& yLocal,double& zLocal);

//ALONSO.- Needed for lhlv. Renaming axis, no more.
void localTOlhlv(double xLocal,double yLocal,double zLocal, double& xLHLV,double& yLHLV,double& zLHLV);

//ALONSO.- This lhlv changes are relative to target position and all of them in
//         inertial coordinate system.

void inertialTOlhlv(double mu,sta::StateVector relativeReference,sta::StateVector transform,sta::StateVector* result);

void lhlvTOinertial(double mu,sta::StateVector relativeReference,sta::StateVector transform,sta::StateVector* result);

