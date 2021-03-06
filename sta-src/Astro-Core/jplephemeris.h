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
// Load JPL's DE200, DE405, and DE406 ephemerides and compute planet
// positions.

#ifndef _STA_ASTROCORE_JPLEPH_H_
#define _STA_ASTROCORE_JPLEPH_H_

#include <QtCore>
#include <vector>
#include "ephemeris.h"

class QIODevice;

namespace sta
{

enum JPLEphemItem
{
    JPLEph_Mercury       =  0,
    JPLEph_Venus         =  1,
    JPLEph_EarthMoonBary =  2,
    JPLEph_Mars          =  3,
    JPLEph_Jupiter       =  4,
    JPLEph_Saturn        =  5,
    JPLEph_Uranus        =  6,
    JPLEph_Neptune       =  7,
    JPLEph_Pluto         =  8,
    JPLEph_Moon          =  9,
    JPLEph_Sun           = 10,
    JPLEph_Earth         = 11,
    JPLEph_SSB           = 12,
    JPLEph_Invalid       = -1,
};


#define JPLEph_NItems 12

struct JPLEphCoeffInfo
{
    quint32 offset;
    quint32 nCoeffs;
    quint32 nGranules;
};


class JPLEphRecord
{
public:
    JPLEphRecord() : coeffs(NULL) {}
    ~JPLEphRecord() { delete[] coeffs; }

    double t0;
    double t1;
    double* coeffs;
};


class JPLEphemeris : public Ephemeris
{
private:
    JPLEphemeris();

public:
    virtual ~JPLEphemeris();

    virtual const QList<StaBodyId>& bodyList() const;
    virtual const StaBody* parentBody(const StaBody* body) const;
    virtual const sta::CoordinateSystemType coordinateSystem(const StaBody* body) const;
    virtual StateVector stateVector(const StaBody* body,
                                    double mjd,
                                    const StaBody* center,
                                    sta::CoordinateSystemType coordSys) const;

    static JPLEphemeris* load(QIODevice* device);

    unsigned int getDENumber() const;
    double getStartDate() const;
    double getEndDate() const;


    /** Get the range of time for which ephemeris data is available for
      * the specified body.
      */
    virtual TimeInterval validTimeInterval(const StaBody* body) const;

private:
    JPLEphemItem mapStaId(StaBodyId id) const;
    StateVector getPlanetStateVector(JPLEphemItem planet, double t) const;

private:
    JPLEphCoeffInfo coeffInfo[JPLEph_NItems];
    JPLEphCoeffInfo librationCoeffInfo;

    double startDate;
    double endDate;
    double daysPerInterval;

    double au;
    double earthMoonMassRatio;

    quint32 DENum;       // ephemeris version
    quint32 recordSize;  // number of doubles per record

    std::vector<JPLEphRecord> records;

    QList<StaBodyId> m_bodies;
};

} // namespace sta

#endif // _STA_ASTROCORE_JPLEPH_H_

