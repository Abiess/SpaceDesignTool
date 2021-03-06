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


///////////////////////////////////////////////////////////////////////////////////////////
// Description: 
//  This subroutine computes the Greenwich hour angle in degrees for the
//  input time.  It uses the model referenced in The Astronomical Almanac
//  for 1984, Section S (Supplement) and documented in "Exact 
//  closed-form geolocation algorithm for Earth survey sensors", by 
//  F.S. Patt and W.W. Gregg, Int. Journal of Remote Sensing, 1993.
//  It includes the correction to mean sideral time for nutation
//  as well as precession.
//
// Calling Arguments
//
//  Name					Type 	I/O	Description
//  iyr						I*4	 I	Year (four digits)
//  day						R*8	 I	Day (time of day as fraction)
//
// Return argument
//  Greenwich Hour Angle	R*8	 O	Greenwich hour angle (degrees)
//
//
//	Subprograms referenced:
//
//	JD		Computes Julian day from calendar date
//	EPHPARMS	Computes mean solar longitude and anomaly and
//			 mean lunar lontitude and ascending node
//	NUTATE		Compute nutation corrections to lontitude and 
//		        obliquity
// 
//	Program written by:	Frederick S. Patt in  FORTRAN, October 92
//	Program adapted by G. Ortega in MATLAB, December 2001
//  Program adapted by G. Ortega in C++, October 2006
///////		Program adapted by G. Ortega for STA January 2007
//
//	
//
// Date: October 2006
// Version: 1.0
// Change history:
//
//
//////////////////////////////////////////////////////////////////////////////////////////




//double getGreenwichHourAngle(int,double);
//double getGreenwichHourAngle (int iyr,  int month, int day, int hour, int minute, int second);
double getGreenwichHourAngle (double jdate);

