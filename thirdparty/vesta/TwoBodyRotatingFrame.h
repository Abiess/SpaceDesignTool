/*
 * $Revision: 223 $ $Date: 2010-03-30 05:44:44 -0700 (Tue, 30 Mar 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#ifndef _VESTA_TWO_BODY_ROTATING_FRAME_H_
#define _VESTA_TWO_BODY_ROTATING_FRAME_H_

#include "Frame.h"
#include "Entity.h"


namespace vesta
{

/** A two-body rotating frame has the following axes:
  *   +x points from the primary to the secondary body
  *   +z is normal to both the velocity and x-axis
  *   +y points in the direction of the velocity of the
  *    secondary relative to the primary, and is
  *    orthogonal to both the x and z axes. Note that y
  *    will not in general point precisely in the direction
  *    of the velocity.
  */
class TwoBodyRotatingFrame : public Frame
{
public:
    TwoBodyRotatingFrame(Entity* primary, Entity* secondary);
    virtual ~TwoBodyRotatingFrame();

    virtual Eigen::Quaterniond orientation(double t) const;
    virtual Eigen::Vector3d angularVelocity(double t) const;

    /** Get the central object of the two-body frame. */
    Entity* primary() const
    {
        return m_primary.ptr();
    }

    /** Get the secondary object of the two-body frame. */
    Entity* secondary() const
    {
        return m_secondary.ptr();
    }

private:
    counted_ptr<Entity> m_primary;
    counted_ptr<Entity> m_secondary;
};

}

#endif // _VESTA_TWO_BODY_ROTATING_FRAME_H_
