/*
 Further information about the European Union Public Licence - EUPL v.1.1 can
 also be found on the world wide web at http://ec.europa.eu/idabc/eupl
 */

/*
 ------ Copyright (C) 2010 STA Steering Board (space.trajectory.analysis AT gmail.com) ----
 ------------------ Author: Ricardo Noriega  ----------------------------------------------
 ------------------ email: ricardonor@gmail.com  ------------------------------------------
File extensively patch by Guillermo on May 2011
 */


#include "coverageanalysis.h"
#include "Main/findDataFolder.h"

#include <QDebug>


#ifdef Q_WS_MAC
#include <CoreFoundation/CFBundle.h>
#endif


CoverageAnalysis::CoverageAnalysis()
{
}
CoverageAnalysis::CoverageAnalysis(PropagatedScenario * propagatedScenario,
                                   int indexSC,
                                   int indexGS,
                                   int indexMA):m_propagatedScenario(propagatedScenario), m_indexSC(indexSC), m_indexGS(indexGS), m_indexMA(indexMA)
{

    //Constructor with the propagated scenario
}


void CoverageAnalysis::reportAER()
{

    QString staResourcesPath = findDataFolder();

    QFile reportCov1(staResourcesPath + "/" + "reportCov1.txt");
    reportCov1.open(QIODevice::WriteOnly|QIODevice::ReadWrite);
    QTextStream streamReportCov1(&reportCov1);
    streamReportCov1.setRealNumberPrecision(16);

    streamReportCov1<<"MJD"<<"\t"<<"Range [Km]"<<"\t"<<"Elevation [deg]"<<"\t"<<"Azimuth [deg]"<<endl;


    for(int i=0; i<m_propagatedScenario->spaceObjects().at(m_indexSC)->mission().at(m_indexMA)->trajectorySampleCount(); i++)
    {
        double mjd=m_propagatedScenario->spaceObjects().at(m_indexSC)->mission().at(m_indexMA)->trajectorySampleTime(i);
        double elevationAngle=m_propagatedScenario->groundObjects().at(m_indexGS)->elevationAngle(m_propagatedScenario->spaceObjects().at(m_indexSC), mjd);
        elevationAngle=elevationAngle*RAD2DEG;
        double range=m_propagatedScenario->groundObjects().at(m_indexGS)->getRange(m_propagatedScenario->spaceObjects().at(m_indexSC), mjd);
        double azimuth=m_propagatedScenario->groundObjects().at(m_indexGS)->azimuthAngle(m_propagatedScenario->spaceObjects().at(m_indexSC), mjd);
        if(elevationAngle>=0.0)
            streamReportCov1<<mjd<<"\t"<<range<<"\t"<<elevationAngle<<"\t"<<azimuth<<endl;
    }
    reportCov1.close();
}
