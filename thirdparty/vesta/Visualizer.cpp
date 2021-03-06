/*
 * $Revision: 530 $ $Date: 2010-10-12 11:26:43 -0700 (Tue, 12 Oct 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#include "Visualizer.h"
#include "Entity.h"

using namespace vesta;
using namespace Eigen;


/** Create a new visualizer with the specified geometry. The newly constructed
  * visualizer is visible by default.
  */
Visualizer::Visualizer(Geometry* geometry) :
    m_visible(true),
    m_geometry(geometry),
    m_depthAdjustment(NoAdjustment)
{
}


Visualizer::~Visualizer()
{
}


/** Get the orientation of this visualizer within the global coordinate
  * system. Subclasses should override this method. The default implementation
  * returns the identity orientation (i.e. no rotation with respect to the
  * fundamental coordinate system.)
  */
Eigen::Quaterniond
Visualizer::orientation(const Entity* /* parent */, double /* t */) const
{
    return Quaterniond::Identity();
}


/** Return true if the given ray intersects the visualizer. The ray origin
  * and direction are in the local coordinate system of the body that the
  * visualizer is attached to. The pixel angle parameter is required for
  * testing intersections with visualizers such as labels that have a fixed
  * size on screen.
  *
  * \param pickOrigin origin of the ray
  * \param pickDirection normalized ray direction
  * \param pixelAngle angle in radians subtended by a pixel
  */
bool
Visualizer::rayPick(const Eigen::Vector3d& pickOrigin,
                    const Eigen::Vector3d& pickDirection,
                    double pixelAngle) const
{
    return handleRayPick(pickOrigin, pickDirection, pixelAngle);
}


/** handleRayPick is called to test whether a visualizer is intersected
  * by a pick ray. It should be overridden by any pickable visualizer.
  *
  * \param pickOrigin origin of the pick ray in local coordinates
  * \param pickDirection the normalized pick ray direction, in local coordinates
  * \param pixelAngle angle in radians subtended by one pixel of the viewport
  */
bool
Visualizer::handleRayPick(const Eigen::Vector3d& pickOrigin,
                          const Eigen::Vector3d& pickDirection,
                          double pixelAngle) const
{
    if (!m_geometry.isNull())
    {
        // For geometry with a fixed apparent size, test to see if the pick ray is
        // within apparent size / 2 pixels of the center.
        if (m_geometry->hasFixedApparentSize())
        {
            double cosAngle = pickDirection.dot(-pickOrigin.normalized());
            if (cosAngle > 0.0)
            {

                if (cosAngle >= 1.0 || acos(cosAngle) < m_geometry->apparentSize() / 2.0 * pixelAngle)
                {
                    return true;
                }
            }
        }
    }

    return false;
}
