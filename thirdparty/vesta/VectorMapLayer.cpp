/*
 * $Revision: 477 $ $Date: 2010-08-31 11:49:37 -0700 (Tue, 31 Aug 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#include "VectorMapLayer.h"
#include "RenderContext.h"
#include "QuadtreeTile.h"
#include "Units.h"
#include <Eigen/Geometry>
#include <algorithm>
#include <cmath>

using namespace vesta;
using namespace Eigen;
using namespace std;


enum
{
    OutWest  = 0x1,
    OutEast  = 0x2,
    OutSouth = 0x4,
    OutNorth = 0x8
};


struct SpherePatch
{
    float west;
    float east;
    float south;
    float north;
};

struct GreatCircleArc
{
    float lon0;
    float lat0;
    float lon1;
    float lat1;
};


// Compute outcodes for Cohen-Sutherland line clipping
static unsigned int
computeOutcode(const SpherePatch& box, const Vector2f& p)
{
    unsigned int outcode = 0;

    outcode |= (p.x() < box.west)  ? OutWest   : 0;
    outcode |= (p.x() > box.east)  ? OutEast   : 0;
    outcode |= (p.y() < box.south) ? OutSouth  : 0;
    outcode |= (p.y() > box.north) ? OutNorth  : 0;

    return outcode;
}

static Vector3f sphToCart(float lon, float lat)
{
    return Vector3f(cos(lon) * cos(lat), sin(lon) * cos(lat), sin(lat));
}

static Vector3f spherePoint(float latDeg, float lonDeg)
{
    float lat = float(toRadians(latDeg));
    float lon = float(toRadians(lonDeg));

    return Vector3f(lon, lat, 0.0f);
}


// Compute the extents of a patch that completely contains an arc
// on great circle.
//
// The arc is assumed not to cross the 180 degree meridian. Such arcs
// should be split into two separate segments, one on either side of
// 180 degrees. Given this assumption, the minimum and maximum longitude
// may be calculated trivially as the minimum and maximum of the endpoint
// longitudes.
//
// The minimum and maximum latitudes require some more care. The minimum
// latitude is the minimum of the endpoints latitudes *unless* the arc
// contains the overall minimum latitude of great circle.
static SpherePatch
computeExtrema(const GreatCircleArc& arc)
{
    // Compute the normal to the plane containing the great circle
    // TODO: Be more careful about cases where the arc spans an angle
    // very close to 0 or 180 degrees.
    Vector3f v0 = sphToCart(arc.lon0, arc.lat0);
    Vector3f v1 = sphToCart(arc.lon1, arc.lat1);
    Vector3f N = v0.cross(v1).normalized();

    SpherePatch bounds;
    bounds.west  = min(arc.lon0, arc.lon1);
    bounds.east  = max(arc.lon0, arc.lon1);
    bounds.south = min(arc.lat0, arc.lat1);
    bounds.north = max(arc.lat0, arc.lat1);

    // If the normal is aligned with the z-axis, the great circle is the equator and we're done.
    // Otherwise, proceed with the calculation of extremal latitudes
    if (abs(N.z()) + 1.0e-6f < 1.0f)
    {
        float angleLatMin = atan2(N.y(), N.x());
        float angleLatMax = angleLatMin + float(angleLatMin < 0.0f ? PI : -PI);
        if (N.z() < 0.0f)
        {
            swap(angleLatMin, angleLatMax);
        }

        if (angleLatMin > bounds.west && angleLatMin < bounds.east)
        {
            bounds.south = -acos(N.z());
        }
        else if (angleLatMax > bounds.west && angleLatMax < bounds.east)
        {
            bounds.north = acos(N.z());
        }
    }

    return bounds;
}


VectorMapLayer::VectorMapLayer()
{
    MapLineString* lines = new MapLineString();
    lines->addPoint(spherePoint(47.5f, -122.5f));
    lines->addPoint(spherePoint(64.133f, -21.933f));
    lines->addPoint(spherePoint(-16.5f, -68.5f));
    lines->addPoint(spherePoint(47.5f, -122.5f));
    lines->setColor(Spectrum(0.0f, 1.0f, 0.0f));

    m_lineStrings.push_back(lines);
}


VectorMapLayer::~VectorMapLayer()
{
}


static void
drawParallel(float lat, float lon0, float lon1)
{
    float cosLat = cos(lat);
    float sinLat = sin(lat);

    glBegin(GL_LINE_STRIP);
    for (unsigned int i = 0; i <= 32; ++i)
    {
        float t = float(i) / 32.0f;
        float lon = (1.0f - t) * lon0 + t * lon1;
        Vector3f v(cos(lon) * cosLat, sin(lon) * cosLat, sinLat);
        glVertex3fv(v.data());
    }
    glEnd();
}


static void
drawMeridian(float lon, float lat0, float lat1)
{
    float cosLon = cos(lon);
    float sinLon = sin(lon);

    glBegin(GL_LINE_STRIP);
    for (unsigned int i = 0; i <= 32; ++i)
    {
        float t = float(i) / 32.0f;
        float lat = (1.0f - t) * lat0 + t * lat1;
        float cosLat = cos(lat);
        Vector3f v(cosLon * cosLat, sinLon * cosLat, sin(lat));
        glVertex3fv(v.data());
    }
    glEnd();
}


// Draw an arc of a great circle between two points on the surface
// of a sphere.
static void
drawGreatCircleArc(const Vector3f& v0, const Vector3f& v1)
{
    glBegin(GL_LINE_STRIP);

    float cosArc = v0.dot(v1);
    float sinArc = sqrt(1.0f - cosArc * cosArc);
    float invSinArc = 1.0f / sinArc;
    float arc = acos(v0.dot(v1));
    float scale = 1.0f + 1.0e-5f;

    Vector3f v = v0 * scale;
    glVertex3fv(v.data());
    for (unsigned int i = 1; i < 32; ++i)
    {
        float t = float(i) / 32.0f;
        v = ((1.0f - t) * v0 + t * v1).normalized() * scale;
        glVertex3fv(v.data());
    }
    v = v1 * scale;
    glVertex3fv(v.data());
    glEnd();
}


static void
drawConstantBearingArc(float lon0, float lat0, float lon1, float lat1, unsigned int subdivision)
{
    float d = 1.0f / subdivision;
    float dlat = lat1 - lat0;
    float dlon = lon1 - lon0;

    glBegin(GL_LINE_STRIP);
    for (unsigned int i = 0; i < subdivision; ++i)
    {
        float t = i * d;
        float lat = lat0 + dlat * t;
        float lon = lon0 + dlon * t;
        float cosLat = cos(lat);
        glVertex3f(cos(lon) * cosLat, sin(lon) * cosLat, sin(lat));
    }
    glVertex3f(cos(lon1) * cos(lat1), sin(lon1) * cos(lat1), sin(lat1));
    glEnd();
}


// Draw an arc of a great circle between two points on the surface
// of a sphere. Subdivide the arc into segments of uniform length.
static void
drawGreatCircleArcUniform(const Vector3f& v0, const Vector3f& v1)
{
    glBegin(GL_LINE_STRIP);

    float cosArc = v0.dot(v1);
    float sinArc = sqrt(1.0f - cosArc * cosArc);
    float invSinArc = 1.0f / sinArc;
    float arc = acos(v0.dot(v1));

    glVertex3fv(v0.data());
    for (unsigned int i = 1; i < 32; ++i)
    {
        float t = float(i) / 32.0f;
        //Vector3f v = ((1.0f - t) * v0 + t * v1).normalized();
        Vector3f v = invSinArc * (sin((1.0f - t) * arc) * v0 + sin(t * arc) * v1);
        glVertex3fv(v.data());
    }
    glVertex3fv(v1.data());
    glEnd();
}


static void clippedLine(const SpherePatch& box, const Vector2f& p0, const Vector2f& p1)
{
    bool done = false;
    unsigned int out0 = computeOutcode(box, p0);
    unsigned int out1 = computeOutcode(box, p1);

    // When done, r0 and r1 will contain the clipped line endpoints
    Vector2f r0 = p0;
    Vector2f r1 = p1;

    // Cohen-Sutherland line clipping
    bool rejected = false;
    while (!done)
    {
        bool accept = (out0 | out1) == 0;
        bool reject = (out0 & out1) != 0;
        bool needClip = !accept && !reject;

        if (accept)
        {
            done = true;
        }
        else if (reject)
        {
            rejected = true;
            done = true;
        }
        else if (needClip)
        {
            Vector2f diff = r1 - r0;
            float clipX;
            float clipY;

            unsigned int out_;
            if (out0)
            {
                out_ = out0;
            }
            else
            {
                out_ = out1;
            }

            // Find the intersection point.
            if (out_ & OutNorth)
            {
                clipX = r0.x() + diff.x() * (box.north - r0.y()) / diff.y();
                clipY = box.north;
            }
            else if (out_ & OutSouth)
            {
                clipX = r0.x() + diff.x() * (box.south - r0.y()) / diff.y();
                clipY = box.south;
             }
             else if (out_ & OutEast)
             {
                 clipX = box.east;
                 clipY = r0.y() + diff.y() * (box.east - r0.x()) / diff.x();
             }
             else if (out_ & OutWest)
             {
                 clipX = box.west;
                 clipY = r0.y() + diff.y() * (box.west - r0.x()) / diff.x();
             }

             if (out_ == out0)
             {
                 r0 = Vector2f(clipX, clipY);
                 out0 = computeOutcode(box, r0);
             }
             else
             {
                 r1 = Vector2f(clipX, clipY);
                 out1 = computeOutcode(box, r1);
             }
         }
     }

    if (!rejected)
    {
#if GREAT_CIRCLE
        Vector3f v0 = sphToCart(r0.x(), r0.y());
        Vector3f v1 = sphToCart(r1.x(), r1.y());
        drawGreatCircleArc(v0, v1);
#else
        drawConstantBearingArc(r0.x(), r0.y(), r1.x(), r1.y(), 32);
#endif
    }
}


void
VectorMapLayer::renderTile(RenderContext& rc, const WorldGeometry* /* world */, const QuadtreeTile* tile) const
{
    rc.setVertexInfo(VertexSpec::PositionColor);

    Material simpleMaterial;
    simpleMaterial.setDiffuse(Spectrum(1.0f, 1.0f, 1.0f));
    simpleMaterial.setOpacity(1.0f);
    rc.bindMaterial(&simpleMaterial);

    float tileArc = float(PI) * tile->extent();
    Vector2f southwest = tile->southwest();

    SpherePatch box;
    box.west = float(PI) * southwest.x();
    box.east = box.west + tileArc;
    box.south = float(PI) * southwest.y();
    box.north = box.south + tileArc;

    for (vector<MapLineString*>::const_iterator iter = m_lineStrings.begin(); iter != m_lineStrings.end(); ++iter)
    {
        const MapLineString* lineString = *iter;
        Spectrum color = lineString->color();
        glColor4f(color.red(), color.green(), color.blue(), lineString->opacity());

        if (lineString->points().size() >= 2)
        {
            for (unsigned int i = 1; i < lineString->points().size(); ++i)
            {
                Vector3f p0 = lineString->points().at(i - 1);
                Vector3f p1 = lineString->points().at(i);

                clippedLine(box, p0.start<2>(), p1.start<2>());
            }
        }
    }

}


MapLineString::MapLineString() :
    m_color(1.0f, 1.0f, 1.0f),
    m_opacity(1.0f)
{

}


void
MapLineString::addPoint(const Eigen::Vector3f& p)
{
    // Automatically split arcs that cross the 180 degree meridian

    m_points.push_back(p);
}


const std::vector<Eigen::Vector3f>&
MapLineString::points() const
{
    return m_points;
}
