/*
 * $Revision: 223 $ $Date: 2010-03-30 05:44:44 -0700 (Tue, 30 Mar 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#include "Entity.h"
#include "Chronology.h"
#include "Arc.h"
#include "Frame.h"
#include "Trajectory.h"
#include "RotationModel.h"
#include "Visualizer.h"

using namespace vesta;
using namespace Eigen;
using namespace std;


Entity::Entity() :
    m_visible(true),
    m_visualizers(NULL)
{
    m_chronology = new Chronology();
}


Entity::~Entity()
{
    delete m_visualizers;
}


/** Get the position of the entity in universal coordinates.
  * @param t the time in seconds since J2000
  */
Vector3d
Entity::position(double t) const
{
    Arc* arc = m_chronology->activeArc(t);
    if (arc)
    {
        Vector3d centerPosition = Vector3d::Zero();
        if (arc->center())
            centerPosition = arc->center()->position(t);
        return centerPosition + arc->trajectoryFrame()->orientation(t) * arc->trajectory()->position(t);
    }
    else
    {
        return Vector3d::Zero();
    }
}


/** Get the state vector of the entity in the fundamental coordinate
  * system (J200).
  * @param t the time in seconds since J2000
  */
StateVector
Entity::state(double t) const
{
    Arc* arc = m_chronology->activeArc(t);
    if (arc)
    {
        StateVector centerState(Vector3d::Zero(), Vector3d::Zero());
        if (arc->center())
        {
            centerState = arc->center()->state(t);
        }

        StateVector state = arc->trajectory()->state(t);

        Matrix3d m = arc->trajectoryFrame()->orientation(t).toRotationMatrix();
        Vector3d w = arc->trajectoryFrame()->angularVelocity(t);
        Vector3d position = m * state.position();
        Vector3d velocity = m * state.velocity() + w.cross(state.position());

        return centerState + StateVector(position, velocity);
    }
    else
    {
        return StateVector(Vector3d::Zero(), Vector3d::Zero());
    }
}


/** Get the orientation of the entity in universal coordinates.
  * @param t the time in seconds since J2000
  */
Quaterniond
Entity::orientation(double t) const
{
    Arc* arc = m_chronology->activeArc(t);
    if (arc)
    {
        return arc->bodyFrame()->orientation(t) * arc->rotationModel()->orientation(t);
    }
    else
    {
        return Quaterniond::Identity();
    }
}


/** Get the angular velocity of the entity in universal coordinates.
  * @param t the time in seconds since J2000
  */
Vector3d
Entity::angularVelocity(double /* t */) const
{
    return Vector3d::Zero();
}


void
Entity::setVisible(bool visible)
{
    m_visible = visible;
}


void
Entity::setName(const std::string& name)
{
    m_name = name;
}


/** Add a new visualizer with a specified tag. If a visualizer with the
  * same tag already exists, it will be replaced.
  */
void
Entity::setVisualizer(const std::string& tag, Visualizer* visualizer)
{
    if (!m_visualizers)
    {
        m_visualizers = new VisualizerTable;
    }
    (*m_visualizers)[tag] = counted_ptr<Visualizer>(visualizer);
}


/** Remove the visualizer with the specified tag. The method has no
  * effect if the tag is not found.
  */
void
Entity::removeVisualizer(const std::string& tag)
{
    if (m_visualizers)
    {
        m_visualizers->erase(tag);
    }
}


/** Get the visualizer with the specified tag. If no visualizer with
  * the requested tag exists, the method returns null.
  */
Visualizer*
Entity::visualizer(const std::string& tag) const
{
    if (m_visualizers)
    {
        VisualizerTable::iterator iter = m_visualizers->find(tag);
        if (iter == m_visualizers->end())
        {
            return NULL;
        }
        else
        {
            return iter->second.ptr();
        }
    }
    else
    {
        return NULL;
    }
}


bool
Entity::hasVisualizers() const
{
    return m_visualizers && !m_visualizers->empty();
}
