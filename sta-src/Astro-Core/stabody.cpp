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
 ------------------ Author: Chris Laurel  -------------------------------------------------
 ------------------ E-mail: (claurel@gmail.com) ----------------------------
 */

#include <cassert>
#include "stabody.h"
#include "stamath.h"
#include "ephemeris.h"
#include "EarthRotationState.h"
#include "UniformRotationState.h"

using namespace sta;
using namespace Eigen;

typedef Eigen::Matrix< double, 3, 3 > 	MyMatrix3d;
typedef Eigen::Matrix< double, 3, 1 > 	MyVector3d;

SolarSystemBodyDictionary* SolarSystemBodyDictionary::s_instance = NULL;

struct RotationalElements
{
    StaBodyId body;
    double poleRa;
    double poleDec;
    double W0;
    double rotationRate;
};

// Rotational elements from the IAU working group:
// Planets: http://astrogeology.usgs.gov/Projects/WGCCRE/constants/iau2000_table1.html
// Moons:   http://astrogeology.usgs.gov/Projects/WGCCRE/constants/iau2000_table2.html
static struct RotationalElements SSRotationalElements[] =
{
    { STA_SUN,     286.13,    63.87,     84.10,   14.1844    },

    { STA_MERCURY, 281.01,    61.45,    329.548,   6.1385025 },
    { STA_VENUS,   272.76,    67.16,    160.20,   -1.4813688 },
    // Earth omitted deliberately--we use a custom class for its rotation state
    { STA_MARS,    317.68143, 52.88650, 176.630, 350.8919822 },
    { STA_JUPITER, 268.05,    64.49,    284.95,  870.5366420 },
    { STA_SATURN,   40.589,   83.537,    38.90,  810.7939024 },
    { STA_URANUS,  257.311,  -15.175,   203.81, -501.1600928 },
    { STA_NEPTUNE, 299.36,    43.36,    253.18,  536.3128492 },
    { STA_PLUTO,   313.02,     9.09,    236.77,  -56.3623195 },

    { STA_MOON,       269.9949,  66.5392,   38.3213,   13.17635815 },

    { STA_PHOBOS,     317.68,    52.90,     35.06,   1128.8445850  },
    { STA_DEIMOS,     316.65,    53.52,     79.41,    285.1618970  },

    { STA_IO,         268.05,    64.50,     200.39,   203.4889538  },
    { STA_EUROPA,     268.08,    64.51,     36.022,   101.3747236  },
    { STA_GANYMEDE,   268.20,    64.57,     44.064,    50.3176081  },
    { STA_CALLISTO,   268.72,    64.83,     259.51,    21.5710715  },

    { STA_ENCELADUS,   40.66,    83.52,      2.82,    262.7318996  },
    { STA_TITAN,       36.41,    83.94,    189.64,     22.5769768  },

    { STA_TRITON,     299.36,    41.17,    296.53,    -61.2572637  },
};


struct VisualAttributes
{
    StaBodyId body;
    const char* baseMapFilename;
};

struct VisualAttributes SSVisualAttributes[] =
{
    { STA_SUN,     "textures/medres/sun.jpg"         },

    { STA_MERCURY, "textures/medres/mercury.jpg"     },
    { STA_VENUS,   "textures/medres/venussurface.jpg"       },
    { STA_EARTH,   "textures/medres/earth.jpg"       },
    { STA_MARS,    "textures/medres/mars.jpg"        },
    { STA_JUPITER, "textures/medres/jupiter.jpg"     },
    { STA_SATURN,  "textures/medres/saturn.jpg"      },
    { STA_URANUS,  "textures/medres/uranus.jpg"      },
    { STA_NEPTUNE, "textures/medres/neptune.jpg"     },
    { STA_PLUTO,   "textures/medres/pluto-lok.jpg"   },

    { STA_MOON,      "textures/medres/moon.jpg"      },

    { STA_PHOBOS,    "textures/medres/phobos.jpg"    },
    { STA_DEIMOS,    "textures/medres/deimos.jpg"    },

    { STA_IO,        "textures/medres/io.jpg"        },
    { STA_EUROPA,    "textures/medres/europa.jpg"    },
    { STA_GANYMEDE,  "textures/medres/ganymede.jpg"  },
    { STA_CALLISTO,  "textures/medres/callisto.jpg"  },

    { STA_MIMAS,     "textures/medres/mimas.jpg"     },
    { STA_ENCELADUS, "textures/medres/enceladus.jpg" },
    { STA_DIONE,     "textures/medres/dione.jpg"     },
    { STA_TETHYS,    "textures/medres/tethys.jpg"    },
    { STA_RHEA,      "textures/medres/rhea.jpg"      },
    { STA_TITAN,     "textures/medres/titan.jpg"     },
    { STA_HYPERION,  "textures/medres/hyperion.jpg"  },
    { STA_IAPETUS,   "textures/medres/iapetus.jpg"   },
    { STA_PHOEBE,    "textures/medres/phoebe.jpg"    },

    { STA_MIRANDA,   "textures/medres/miranda.jpg"   },
    { STA_ARIEL,     "textures/medres/ariel.jpg"     },
    { STA_UMBRIEL,   "textures/medres/umbriel.jpg"   },
    { STA_TITANIA,   "textures/medres/titania.jpg"   },
    { STA_OBERON,    "textures/medres/oberon.jpg"    },

    { STA_TRITON,    "textures/medres/triton.jpg"    },
};


struct GravitationalAttributes
{
    StaBodyId body;
    VectorXd zonals;
};

// Some gravitational parameters from SPICE kernel of planetary constants
// Gravity.tpc:
//   ftp://naif.jpl.nasa.gov/pub/naif/generic_kernels
struct GravitationalAttributes SSGravitationalAttributes[] =
{
    { STA_EARTH,     MyVector3d(1.082616e-3, -2.53881-6, -1.65597e-6) },
    { STA_MERCURY,   MyVector3d(6.0e-5,       0.0,       0.0   ) },
    { STA_VENUS,     MyVector3d(4.458e-006,   0.0,       0.0   ) },
    { STA_MARS,      MyVector3d(1.96045e-3,   3.6e-5,    0.0   ) },
    { STA_JUPITER,   MyVector3d(1.4736e-2,    0.0,      -5.8e-4) },
    { STA_SATURN,    MyVector3d(1.6298e-2,    0.0,      -1.0e-3) },
    { STA_URANUS,    MyVector3d(3.34343e-3,   0.0,       0.0   ) },
    { STA_NEPTUNE,   MyVector3d(3.411e-3,     0.0,       0.0   ) },
    { STA_PLUTO,     MyVector3d(0.0,          0.0,       0.0   ) },
    { STA_SUN,       MyVector3d(0.0,          0.0,       0.0   ) },
    { STA_MOON,      MyVector3d(203.986e-006, 0.0,       0.0   ) },
};


// Add a solar system body with a triaxial shape
static void
AddSolarSystemBody(QHash<StaBodyId, StaBody*>& dict,
                   QHash<QString, StaBody*>& nameIndex,
                   StaBodyId id,
                   const QString& name,
                   double mu,
                   const MyVector3d& radii,
                   double linearV,
                   double distance,
                   double orbitalPeriod,
                   double inclination)
{
    StaBody* body = new StaBody(id, name, mu, radii, linearV, distance, orbitalPeriod, inclination);
    dict.insert(id, body);
    nameIndex.insert(name.toLower(), body);
}


// Add a solar system body with a spheroidal shape;
// eqPolarRadii.x = equatorial radius
// eqPolarRadii.y = polar radius
static void
AddSolarSystemBody(QHash<StaBodyId, StaBody*>& dict,
                   QHash<QString, StaBody*>& nameIndex,
                   StaBodyId id,
                   const QString& name,
                   double mu,
                   const Vector2d& eqPolarRadii,
                   double linearV,
                   double distance,
                   double orbitalPeriod,
                   double inclination)
{
    AddSolarSystemBody(dict, nameIndex, id, name, mu, MyVector3d(eqPolarRadii.x(), eqPolarRadii.x(), eqPolarRadii.y()),linearV, distance, orbitalPeriod, inclination);
}


// Add a spherical solar system body.
static void
AddSolarSystemBody(QHash<StaBodyId, StaBody*>& dict,
                   QHash<QString, StaBody*>& nameIndex,
                   StaBodyId id,
                   const QString& name,
                   double mu,
                   double radius,
                   double linearV,
                   double distance,
                   double orbitalPeriod,
                   double inclination)
{
    AddSolarSystemBody(dict, nameIndex, id, name, mu, MyVector3d(radius, radius, radius), linearV, distance, orbitalPeriod, inclination);
}


static void
InitSolarSystemBodyDictionary(QHash<StaBodyId, StaBody*>& dict, QHash<QString, StaBody*>& nameIndex)
{
    // Data sources:
    // [1] SPICE Planetary Constants Kernel (PCK) Gravity.tpc
    //      ftp://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/Gravity.tpc
    //
    // [2] SPICE PCK for Cassini mission, July 2010:
    //      ftp://naif.jpl.nasa.gov/pub/naif/CASSINI/kernels/pck/cpck06Jul2010.tpc
    //
    // [3] Report of the IAU/IAG Working Group on cartographic coordinates and rotational elements: 2006.:
    //      http://www2.keck.hawaii.edu/inst/people/conrad/research/pub/WGCCRE2006Feb9.pdf

    // GM values for satellites of Mars, Jupiter, Uranus, and Neptune: [1]
    // GM values for satellites of Saturn: [2]
    // Radii of outer planet satellites: [3]
    //
    // For GM values, the number of significant digits used in the source is preserved, even
    // though it generally exceeds the precision of the measurements.
    //

    // Dynamical points
    AddSolarSystemBody(dict, nameIndex, STA_SOLAR_SYSTEM_BARYCENTER, "Solar System Barycenter", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    AddSolarSystemBody(dict, nameIndex, STA_EARTH_BARYCENTER,        "Earth Barycenter",        0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    //                                   Body name              mu Km^3 s^-2        Radius Km                       linear vel. mean distance  Period (d) inclination
    AddSolarSystemBody(dict, nameIndex, STA_EARTH,     "Earth",  398600.415,     Vector2d(6378.14, 6356.75),        29.784678,  149597870.7,     365.256, 7.155);
    AddSolarSystemBody(dict, nameIndex, STA_MERCURY,   "Mercury",     22032,     Vector2d(2439.7, 2439.7),          47.87,      57909100.0,       87.968, 3.38);
    AddSolarSystemBody(dict, nameIndex, STA_VENUS,     "Venus",      324859,     Vector2d(6051.8, 6051.8),          35.02,      108208930.0,     224.701, 3.86);
    AddSolarSystemBody(dict, nameIndex, STA_MARS,      "Mars",        42828,     Vector2d(3396.19, 3376.20),        24.13,      227939100.0,     686.980, 5.65);
    AddSolarSystemBody(dict, nameIndex, STA_JUPITER,   "Jupiter", 126686534,     Vector2d(71492, 66854),            13.07,      778547200.0,    4332.589, 6.09);
    AddSolarSystemBody(dict, nameIndex, STA_SATURN,    "Saturn",   37931187,     Vector2d(60268, 54364),            9.69,       1433449370.0,  10759.22,  5.51);
    AddSolarSystemBody(dict, nameIndex, STA_URANUS,    "Uranus",    5793947,     Vector2d(25559, 24973),            6.81,       2876679082.0,  30685.4,   6.48);
    AddSolarSystemBody(dict, nameIndex, STA_NEPTUNE,   "Neptune",   6836529,     Vector2d(24764, 24341),            5.43,     4503443.661e+03, 60189,     6.43);
    AddSolarSystemBody(dict, nameIndex, STA_PLUTO,     "Pluto",        1001,     1195,                              4.72,       5906380e+03,   90465,    17.2);

    AddSolarSystemBody(dict, nameIndex, STA_SUN,       "Sun",  1.32712440018e11, 695500,                            0,          0,          0,               0);

    AddSolarSystemBody(dict, nameIndex, STA_MOON,      "Moon",  4902.801,        1737.4,                            1.023,      384399,     27.3217,      5.145);

    AddSolarSystemBody(dict, nameIndex, STA_PHOBOS,    "Phobos",  85.22834344572990e-5,     MyVector3d(13.4, 11.2, 9.2),           0,     9380,          0.31910,  1.075);
    AddSolarSystemBody(dict, nameIndex, STA_DEIMOS,    "Deimos", 119.9192772100721e-6,      MyVector3d( 7.5,  6.1, 5.2),           0,    23459,          1.26244,  1.79);

    AddSolarSystemBody(dict, nameIndex, STA_IO,        "Io",       5960.981613781674,       MyVector3d(1829.4,  1819.3,  1815.7),  0,    421800,         1.7692,   0.47);
    AddSolarSystemBody(dict, nameIndex, STA_EUROPA,    "Europa",   3200.988724107317,       MyVector3d(1564.13, 1561.23, 1560.93), 0,    670900,         3.551,    0.47);
    AddSolarSystemBody(dict, nameIndex, STA_GANYMEDE,  "Ganymede", 9886.997387719448,       MyVector3d(2632.4,  2632.29, 2623.35), 0,    1070400,        7.155,    0.47);
    AddSolarSystemBody(dict, nameIndex, STA_CALLISTO,  "Callisto", 7180.972962701350,       MyVector3d(2409.4,  2409.2,  2409.3),  0,    1882700,       16.69,     0.47);

    AddSolarSystemBody(dict, nameIndex, STA_MIMAS,     "Mimas",       2.5026538541303731,   MyVector3d(207.4,196.8,190.6 ),      0,      185540,         0.9417,    0);
    AddSolarSystemBody(dict, nameIndex, STA_ENCELADUS, "Enceladus",   7.210753192578406,    MyVector3d(256.3,247.3,244.6 ),      0,      238020,         1.370218,  0);
    AddSolarSystemBody(dict, nameIndex, STA_TETHYS,    "Tethys",     41.22399120522562,     MyVector3d(540.5,531.1,527.5 ),      0,      294670,         1.8879,    0);
    AddSolarSystemBody(dict, nameIndex, STA_DIONE,     "Dione",      73.11575812534798,     MyVector3d(563.8,561.0,560.3 ),      0,      377420,         2.7370,    0);
    AddSolarSystemBody(dict, nameIndex, STA_RHEA,      "Rhea",      153.9434658935521,      MyVector3d(767.2,762.5,763.1 ),      0,      527070,         4.5167,    0);
    AddSolarSystemBody(dict, nameIndex, STA_TITAN,     "Titan",    8978.138877124944,       2575,                           5.58,      1221870,        15.945421, 0.33);
    AddSolarSystemBody(dict, nameIndex, STA_HYPERION,  "Hyperion",    0.372092758433870,    MyVector3d(164, 130, 107 ),          0,      1508800,        21.28,     0);
    AddSolarSystemBody(dict, nameIndex, STA_IAPETUS,   "Iapetus",   120.5175109354886,      MyVector3d(747.1, 749.0, 712.6 ),    0,      3560840,        79.33,     0);
    AddSolarSystemBody(dict, nameIndex, STA_PHOEBE,    "Phoebe",      0.5531033330031904,   MyVector3d(108.6, 107.7, 101.5 ),    0,      12947780,       550.31,    0);

    AddSolarSystemBody(dict, nameIndex, STA_TRITON,    "Triton", 1427.9,           1352.6,                         -4.39,       354759,     5.876854,   157.345);
}

static QList<StaBody*>
InitMajorBodiesList()
{
    StaBodyId majorBodyIds[] = {
        STA_EARTH,
        STA_SUN,
        STA_MOON,
        STA_MERCURY,
        STA_VENUS,
        STA_MARS,
        STA_JUPITER,
        STA_SATURN,
        STA_URANUS,
        STA_NEPTUNE,
        STA_PLUTO,
    };
    
    QList<StaBody*> bodies;
    for (unsigned int i = 0; i < sizeof(majorBodyIds) / sizeof(majorBodyIds[0]); i++)
    {
        StaBody* body = STA_SOLAR_SYSTEM->lookup(majorBodyIds[i]);
        assert(body != NULL);
        bodies.append(body);
    }
    
    return bodies;
}


/*! Private constructor called only be the static Create() method.
 */
SolarSystemBodyDictionary::SolarSystemBodyDictionary() :
    m_sun(NULL),
    m_earth(NULL),
    m_ssb(NULL)
{
}


/*! Create a new SolarSystemBodyDictionary and fill it with the standard
 *  list of bodies recognized used in STA.
 */
void
SolarSystemBodyDictionary::Create()
{
    Q_ASSERT(s_instance == NULL);  // multiple creations not allowed
    s_instance = new SolarSystemBodyDictionary();
    InitSolarSystemBodyDictionary(s_instance->m_bodies, s_instance->m_indexByName);
    s_instance->m_majorBodies = InitMajorBodiesList();

    // Set up gravity models for Solar System bodies with known harmonic coefficients
    for (unsigned int i = 0; i < sizeof(SSGravitationalAttributes) / sizeof(SSGravitationalAttributes[0]); ++i)
    {
        const GravitationalAttributes& att = SSGravitationalAttributes[i];
        StaBody* body = s_instance->lookup(att.body);
        if (body)
        {
            body->gravityModel().setZonals(att.zonals);
        }
    }
    //s_instance->lookup(STA_EARTH)->gravityModel().setZonals(MyVector3d(1.08263e-03, 2.54e-06, 1.61e-06));
    //s_instance->lookup(STA_MARS)->gravityModel().setZonals(MyVector3d(1.96045e-3, 0.0, 0.0));

    // Set rotation states for Solar System bodies
    s_instance->lookup(STA_EARTH)->setRotationState(new EarthRotationState());
    for (unsigned int i = 0; i < sizeof(SSRotationalElements) / sizeof(SSRotationalElements[0]); ++i)
    {
        const RotationalElements& re = SSRotationalElements[i];
        StaBody* body = s_instance->lookup(re.body);
        if (body)
        {
            body->setRotationState(new UniformRotationState(degToRad(re.poleRa),
                                                            degToRad(re.poleDec),
                                                            degToRad(re.W0),
                                                            degToRad(re.rotationRate)));
        }
    }

    // Set the visual attributes for all Solar System bodies
    for (unsigned int i = 0; i < sizeof(SSVisualAttributes) / sizeof(SSVisualAttributes[0]); ++i)
    {
        const VisualAttributes& att = SSVisualAttributes[i];
        StaBody* body = s_instance->lookup(att.body);
        if (body)
        {
            body->setBaseTexture(att.baseMapFilename);
        }
    }

    // Keep 'shortcuts' for very frequently used objects
    s_instance->m_ssb   = s_instance->lookup(STA_SOLAR_SYSTEM_BARYCENTER);
    s_instance->m_sun   = s_instance->lookup(STA_SUN);
    s_instance->m_earth = s_instance->lookup(STA_EARTH);
}


/*! Use the specified ephemeris for all bodies contained in the ephemeris.
 *  If a body already has an ephemeris assigned, the new one overrides.
 */
void
SolarSystemBodyDictionary::UseEphemeris(const sta::Ephemeris* ephemeris)
{
    Q_ASSERT(s_instance);
    foreach (StaBodyId id, ephemeris->bodyList())
    {
        if (s_instance->m_bodies.contains(id))
        {
            StaBody* body = s_instance->m_bodies.value(id);
            body->setEphemeris(ephemeris);
        }
    }
}


/*! Look up an STA body by its id.
 */
StaBody* 
SolarSystemBodyDictionary::lookup(StaBodyId id) const
{
    if (m_bodies.contains(id))
        return m_bodies.value(id);
    else
        return NULL;
}


/*! Look up an STA body by its name. The lookup is case insensitive.
 */
StaBody*
SolarSystemBodyDictionary::lookup(const QString& name) const
{
    QString lowercaseName = name.toLower();
    if (m_indexByName.contains(lowercaseName))
        return m_indexByName.value(lowercaseName);
    else
        return NULL;
}


/*! Return a list containing all solar system bodies.
 */
const QList<StaBody*>
SolarSystemBodyDictionary::list() const
{
    return m_bodies.values();
}


/*! Return an StaBody list containing the planets, Sun, and Moon.
 */
const QList<StaBody*>&
SolarSystemBodyDictionary::majorBodies() const
{
    return m_majorBodies;
}


/*! Get the state vector of the body at the given time and with the specified
 *  center and coordinate system. This method will return a zero state vector
 *  if the body does not have an ephemeris assigned.
 */
sta::StateVector
StaBody::stateVector(double mjd, const StaBody* center, sta::CoordinateSystemType coordSys) const
{
    if (m_ephemeris)
    {
        StateVector v = m_ephemeris->stateVector(this, mjd, center, coordSys);
        if (coordSys != COORDSYS_EME_J2000)
            v = CoordinateSystem(coordSys).fromEmeJ2000(v, center, mjd);
        return v;
    }
    else
    {
        return sta::StateVector::zero();
    }
}


/*! Get the orientation of the body at the given time and in the specified
 *  coordinate system. This method will return a rotation of zero degrees
 *  if the body has no rotation state assigned.
 */
Quaterniond
StaBody::orientation(double mjd, sta::CoordinateSystemType /* coordSys */) const
{
    if (m_rotationState)
    {
        // TODO: Need to implement coordinate system conversion
        return m_rotationState->orientation(mjd);
    }
    else
    {
        return Quaterniond::Identity();
    }
}


/*! Convert planetographic coordinates on a body to cartesian coordinates in the fixed
 *  frame of the body. Latitude and longitude are given in degrees, altitude is in
 *  kilometers.
 */
MyVector3d
StaBody::planetographicToCartesian(double latitude, double longitude, double altitude) const
{
    // TODO: This approximation assumes the body is spherical. Need a point-to-ellipse
    // distance function to do this right.
    double longitudeRad = sta::degToRad(longitude);
    double latitudeRad = sta::degToRad(latitude);
    MyVector3d normPosition(cos(latitudeRad) * cos(longitudeRad),
                          cos(latitudeRad) * sin(longitudeRad),
                          sin(latitudeRad));

    // TODO: Another approximation--this would be appropriate for planetocentric coordinates,
    // not planetographic.
    return normPosition.cwise() * radii() + normPosition * altitude;
}



