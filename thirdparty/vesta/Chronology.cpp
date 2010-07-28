/*
 * $Revision: 223 $ $Date: 2010-03-30 05:44:44 -0700 (Tue, 30 Mar 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#include "Chronology.h"
#include "Arc.h"

using namespace vesta;
using namespace std;


Chronology::Chronology() :
    m_beginning(0.0),
    m_duration(0.0)
{
}


Chronology::~Chronology()
{
}


/** Set the starting time of this chronology.
  * @param t time in seconds since J2000
  */
void Chronology::setBeginning(double t)
{
    m_beginning = t;
}


/** Return a pointer to the arc active at the specified time. Returns
  * NULL if t is outside the time span covered by the chronology. An arc
  * is considered active when startTime <= t < endTime. The exception is
  * the last arc, which is also active when t is exactly equal to the end
  * time.
  */
Arc*
Chronology::activeArc(double t) const
{
    if (t < beginning() || t > ending() || m_arcSequence.empty())
    {
        return NULL;
    }
    else
    {
        double m_arcBeginning = m_beginning;
        for (list<counted_ptr<Arc> >::const_iterator iter = m_arcSequence.begin(); iter != m_arcSequence.end(); iter++)
        {
            counted_ptr<Arc> arc = *iter;
            if (t - m_arcBeginning < arc->duration())
            {
                return &*arc;
            }

            m_arcBeginning += arc->duration();
        }

        // Only reached when t == ending
        return &*m_arcSequence.back();
    }
}


/** Return the first arc of the chronology (or NULL if the chronology is empty.)
  */
Arc*
Chronology::firstArc() const
{
    if (m_arcSequence.empty())
        return NULL;
    else
        return &*m_arcSequence.front();
}


/** Return the last arc of the chronology (or NULL if the chronology is empty.)
  */
Arc*
Chronology::lastArc() const
{
    if (m_arcSequence.empty())
        return NULL;
    else
        return &*m_arcSequence.back();
}


/** Add a new arc to the chronology.
  */
void
Chronology::addArc(Arc* arc)
{
    m_arcSequence.push_back(counted_ptr<Arc>(arc));
    m_duration += arc->duration();
}
