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
--------------------------------- Author: Guillermo Ortega  ----------------------------------
----------------------------------------------------------------------------------------------
 */


#include "Scenario/staschema.h"
#include "Scenario/scenario.h"
#include "Scenario/propagationfeedback.h"
#include "Scenario/scenario.h"
#include "Scenario/missionsDefaults.h"
#include "Scenario/scenarioPropagator.h"

#include "Main/scenarioelementbox.h"
#include "Main/mainwindow.h"
#include "Main/scenarioview.h"
#include "Main/scenariotree.h"
#include "Main/propagatedscenario.h"
#include "Main/timelinewidget.h"
#include "Main/timelineview.h"

#include "Astro-Core/orbitalTOcartesian.h"
#include "Astro-Core/cartesianTOorbital.h"
#include "Astro-Core/trueAnomalyTOmeanAnomaly.h"
#include "Astro-Core/stamath.h"
#include "Astro-Core/date.h"
#include "Astro-Core/EclipseDuration.h"

#include"Astro-Core/eci2lhlv.h"

//#include "Plotting/plottingtool.h"

#include "Loitering/loitering.h"
#include "Loitering/loiteringTLE.h"

#include "RendezVous/rendezVousManoeuvres.h"


#include "Entry/reentry.h"

#include "External/external.h"

#include <QWidget>
#include <QColor>
#include <QDebug>
#include <QMessageBox>

#include <cmath>

USING_PART_OF_NAMESPACE_EIGEN;



void scenarioPropagatorSatellite(ScenarioSC* vehicle, PropagationFeedback& feedback, PropagatedScenario* propScenario, QWidget* parent)
{
	const QList<QSharedPointer<ScenarioAbstractTrajectoryType> >& trajectoryList = vehicle->SCMission()->TrajectoryPlan()->AbstractTrajectory();
    int numberOfArcs = trajectoryList.size();
    sta::StateVector theLastStateVector = sta::StateVector::zero();

    Quaterniond zeroQuaternion(0.0, 0.0, 0.0, 0.0);
    Vector3d zeroRates(0.0, 0.0, 0.0);
    staAttitude::AttitudeVector theLastAttitudeVector(zeroQuaternion, zeroRates);

    double theLastSampleTime = 0.0;

	// Initial state is stored in the first trajectory (for now); so,
	// the empty trajectory plan case has to be treated specially.
	if (!trajectoryList.isEmpty())
	{
        SpaceObject* spaceObject = new SpaceObject(vehicle);
		spaceObject->setName(vehicle->Name());

		// Propagate all segments of the trajectory plan.
		foreach (QSharedPointer<ScenarioAbstractTrajectoryType> trajectory, trajectoryList)
		{
            QList<double> sampleTimesTrajectory; QList<double> sampleTimesAttitude;
            QList<sta::StateVector> samplesTrajectory; QList<staAttitude::AttitudeVector> samplesAttitude;
            int numberOFsamples;

			if (dynamic_cast<ScenarioLoiteringType*>(trajectory.data()))    // Loitering
			{
                ScenarioLoiteringType* loitering = dynamic_cast<ScenarioLoiteringType*>(trajectory.data());
                QString propagator = loitering->PropagationPosition()->propagator();

                if (theLastSampleTime > 0.0)  // This arc is NOT the first one
                {
                    if (propagator == "TWO BODY")
                    {
                        ScenarioKeplerianElementsType* elements = new ScenarioKeplerianElementsType;
                        QString centralBodyName = loitering->Environment()->CentralBody()->Name();
                        StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
                        sta::KeplerianElements initialStateKeplerian = cartesianTOorbital(centralBody->mu(), theLastStateVector);
                        //qDebug() << initialStateKeplerian.Inclination << endl;
                        elements->setSemiMajorAxis(initialStateKeplerian.SemimajorAxis);
                        elements->setEccentricity(initialStateKeplerian.Eccentricity);
                        elements->setInclination(sta::radToDeg(initialStateKeplerian.Inclination));
                        elements->setRAAN(sta::radToDeg(initialStateKeplerian.AscendingNode));
                        elements->setArgumentOfPeriapsis(sta::radToDeg(initialStateKeplerian.ArgumentOfPeriapsis));
                        elements->setTrueAnomaly(sta::radToDeg(initialStateKeplerian.TrueAnomaly));
                        loitering->InitialPosition()->setAbstract6DOFPosition(QSharedPointer<ScenarioAbstract6DOFPositionType>(elements));
                        //QDateTime myStarDateTime = sta::JdToCalendar(sta::MjdToJd(theLastSampleTime));
                        //loitering->TimeLine()->setStartTime(myStarDateTime);
                    }
                    else
                    {
                        ScenarioStateVectorType* stateVector = new ScenarioStateVectorType();
                        stateVector->setX(theLastStateVector.position(0));
                        stateVector->setY(theLastStateVector.position(1));
                        stateVector->setZ(theLastStateVector.position(2));
                        stateVector->setVx(theLastStateVector.velocity(0));
                        stateVector->setVy(theLastStateVector.velocity(1));
                        stateVector->setVz(theLastStateVector.velocity(2));
                        //qDebug() << stateVector->x() << stateVector->y() << stateVector->z() << endl;
                        loitering->InitialPosition()->setAbstract6DOFPosition(QSharedPointer<ScenarioAbstract6DOFPositionType>(stateVector));
                        //QDateTime myStarDateTime = sta::JdToCalendar(sta::MjdToJd(theLastSampleTime));
                        //loitering->TimeLine()->setStartTime(myStarDateTime);
                    }
                }

                PropagateLoiteringTrajectory(loitering, sampleTimesTrajectory, samplesTrajectory, feedback);

                PropagationFeedback feedbackAttitude;
                PropagateLoiteringAttitude(loitering, sampleTimesAttitude, samplesAttitude, feedbackAttitude);

                // Recovering the last state vector
                numberOFsamples = sampleTimesTrajectory.size();
                theLastStateVector = samplesTrajectory.at(numberOFsamples - 1);
                //theLastAttitudeVector = samplesAttitude.at(numberOFsamples - 1);
                theLastSampleTime = sampleTimesTrajectory.at(numberOFsamples - 1);

				//******************************************************************** /OZGUN
				// Eclipse function is called and the "data/EclipseStarLight.stad" is generated
				EclipseDuration* Eclipse = new EclipseDuration();

                Eclipse->StarLightTimeFunction(sampleTimesTrajectory,
                                               samplesTrajectory,
											   STA_SOLAR_SYSTEM->lookup("Earth"),
											   STA_SOLAR_SYSTEM->lookup("Sun"));

				//******************************************************************** OZGUN/

				if (feedback.status() != PropagationFeedback::PropagationOk)
				{
					return;
				}

				QString centralBodyName = loitering->Environment()->CentralBody()->Name();
				StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
				if (!centralBody)
				{
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized central body");
					continue;
				}

				QString coordSysName = loitering->InitialPosition()->CoordinateSystem();
				sta::CoordinateSystem coordSys(coordSysName);
				if (coordSys.type() == sta::COORDSYS_INVALID)
				{
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized coordinate system");
					continue;
				}

                if (numberOFsamples > 1)
				{
                    MissionArc* arc = new MissionArc(centralBody,
                                                     coordSys,
                                                     sampleTimesTrajectory,
                                                     samplesTrajectory,
                                                     sampleTimesAttitude,
                                                     samplesAttitude);

                    // Loading arc color, name, and model
                    arc->setArcName(loitering->ElementIdentifier()->Name());
                    QString arcColorName = loitering->ElementIdentifier()->colorName();
                    MissionsDefaults myMissionDefaults;
                    QColor trajectoryColor = myMissionDefaults.missionArcColorFromQt(arcColorName);
                    arc->setArcTrajectoryColor(trajectoryColor);
                    arc->setModelName(loitering->ElementIdentifier()->modelName());

                    spaceObject->addMissionArc(arc);
				}
			} ///////// End of loitering IF
			else if (dynamic_cast<ScenarioLoiteringTLEType*>(trajectory.data()))    //// TLEs
			{
				ScenarioLoiteringTLEType* loiteringTLE = dynamic_cast<ScenarioLoiteringTLEType*>(trajectory.data());
                PropagateLoiteringTLETrajectory(loiteringTLE, sampleTimesTrajectory, samplesTrajectory, feedback);

				if (feedback.status() != PropagationFeedback::PropagationOk)
				{
                    return;
				}

				QString centralBodyName = "Earth";
				StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);

				if (!centralBody)
				{
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized central body");
					continue;
				}

				QString coordSysName = "INERTIAL J2000";
				sta::CoordinateSystem coordSys(coordSysName);
				if (coordSys.type() == sta::COORDSYS_INVALID)
				{
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized coordinate system");
					continue;
				}

                if (sampleTimesTrajectory.size() > 1)
				{
					MissionArc* arc = new MissionArc(centralBody,
													 coordSys,
                                                     sampleTimesTrajectory,
                                                     samplesTrajectory);

                    // Loading arc color, name, and model
                    QString arcName = loiteringTLE->ElementIdentifier()->Name();
                    QString arcColorName = loiteringTLE->ElementIdentifier()->colorName();
                    MissionsDefaults myMissionDefaults;
                    QColor trajectoryColor = myMissionDefaults.missionArcColorFromQt(arcColorName);
                    QString arcModelName = loiteringTLE->ElementIdentifier()->modelName();
                    arc->setArcTrajectoryColor(trajectoryColor);
                    arc->setModelName(arcModelName);

					spaceObject->addMissionArc(arc);
				}
            }
            else if (dynamic_cast<ScenarioDeltaVType*>(trajectory.data()))    // DeltaVs
            {
                ScenarioDeltaVType* deltaV = dynamic_cast<ScenarioDeltaVType*>(trajectory.data());
                // Calculating direction and magnitude. The magnitude is
                // stored in m/s, and we need to convert to km/s
                double theDeltaVMagnitude = deltaV->Magnitude() * 0.001;

                // Calculating the duration dt (in seconds)
                theLastSampleTime = sta::JdToMjd(sta::CalendarToJd(deltaV->TimeLine()->StartTime()));
                double endTime = sta::JdToMjd(sta::CalendarToJd(deltaV->TimeLine()->EndTime()));
                double dt = sta::daysToSecs(endTime - theLastSampleTime);

                sampleTimesTrajectory << theLastSampleTime;
                samplesTrajectory << theLastStateVector;

                // Calculating direction and magnitude. The magnitude is
                // stored in m/s, and we need to convert to km/s
                Vector3d dv = Vector3d(deltaV->DeltaVx(), deltaV->DeltaVy(), deltaV->DeltaVz()) * (deltaV->Magnitude() * 0.001);

                // Compute the transformation from the local satellite coordinate system to the
                // coordinate system of the trajectory.
                Vector3d T = theLastStateVector.velocity.normalized();
                Vector3d R = -theLastStateVector.position.normalized();

                // Compute the rows of the 3x3 transformation matrix
                Vector3d X = T;
                Vector3d Y = (X.cross(R)).normalized();
                Vector3d Z = Y.cross(X);
                Matrix3d M;
                M << X, Y, Z;

                // Transform delta-V
                dv = M * dv;

                // TODO: We should be performing at least two body propagation over the duration
                // of the maneuver.
                theLastStateVector.position += theLastStateVector.velocity * dt;
                theLastStateVector.velocity += dv;

                theLastSampleTime = endTime;

                // Emit the time+state sample
                sampleTimesTrajectory << theLastSampleTime;
                samplesTrajectory << theLastStateVector;

                QString centralBodyName = deltaV->Environment()->CentralBody()->Name();
                StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
                QString coordSysName = deltaV->InitialPosition()->CoordinateSystem();
                sta::CoordinateSystem coordSys(coordSysName);

                MissionArc* arc = new MissionArc(centralBody,
                                                 coordSys,
                                                 sampleTimesTrajectory,
                                                 samplesTrajectory);

                // Loading arc color, name, and model
                arc->setArcName(deltaV->ElementIdentifier()->Name());
                QString arcColorName = deltaV->ElementIdentifier()->colorName();
                MissionsDefaults myMissionDefaults;
                QColor trajectoryColor = myMissionDefaults.missionArcColorFromQt(arcColorName);
                arc->setArcTrajectoryColor(trajectoryColor);
                arc->setModelName(deltaV->ElementIdentifier()->modelName());
                spaceObject->addMissionArc(arc);
            }
            else if (dynamic_cast<ScenarioExternalType*>(trajectory.data()))    // External trajectory
            {
                ScenarioExternalType* extTrajectory = dynamic_cast<ScenarioExternalType*>(trajectory.data());

                QString centralBodyName = extTrajectory->CentralBody()->Name();
                StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
                if (!centralBody)
                {
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized central body");
                    continue;
                }

                QString coordSysName = extTrajectory->CoordinateSystem();
                sta::CoordinateSystem coordSys(coordSysName);
                if (coordSys.type() == sta::COORDSYS_INVALID)
                {
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized coordinate system");
                    continue;
                }

                PropagateExternalTrajectory(extTrajectory, "", sampleTimesTrajectory, samplesTrajectory, feedback);

                if (feedback.status() != PropagationFeedback::PropagationOk)
                {
                    return;
                }

                if (sampleTimesTrajectory.size() > 1)
                {
                    MissionArc* arc = new MissionArc(centralBody,
                                                     coordSys,
                                                     sampleTimesTrajectory,
                                                     samplesTrajectory);

                    // Loading arc color, name, and model
                    QString arcName = extTrajectory->ElementIdentifier()->Name();
                    QString arcColorName = extTrajectory->ElementIdentifier()->colorName();

                    MissionsDefaults myMissionDefaults;
                    QColor trajectoryColor = myMissionDefaults.missionArcColorFromQt(arcColorName);
                    QString arcModelName = extTrajectory->ElementIdentifier()->modelName();

                    arc->setArcName(arcName);
                    arc->setArcTrajectoryColor(trajectoryColor);
                    arc->setModelName(arcModelName);

                    spaceObject->addMissionArc(arc);
                }


            }
            else if(dynamic_cast<ScenarioRendezVousManoeuvreType*>(trajectory.data())) //All kind of rendezvous manoeuvres
            {
                ScenarioRendezVousManoeuvreType* RVmanoeuvre = dynamic_cast<ScenarioRendezVousManoeuvreType*>(trajectory.data());
                QString propagator = RVmanoeuvre->PropagationPosition()->propagator();

                QString centralBodyName = RVmanoeuvre->Environment()->CentralBody()->Name();
                StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
                double mu=centralBody->mu();



                if (theLastSampleTime > 0.0)  // This arc is NOT the first one
                {
                    if (propagator == "TWO BODY")
                    {
                        ScenarioKeplerianElementsType* elements = new ScenarioKeplerianElementsType;
                        QString centralBodyName = RVmanoeuvre->Environment()->CentralBody()->Name();
                        StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
                        sta::KeplerianElements initialStateKeplerian = cartesianTOorbital(centralBody->mu(), theLastStateVector);
                        elements->setSemiMajorAxis(initialStateKeplerian.SemimajorAxis);
                        elements->setEccentricity(initialStateKeplerian.Eccentricity);
                        elements->setInclination(sta::radToDeg(initialStateKeplerian.Inclination));
                        elements->setRAAN(sta::radToDeg(initialStateKeplerian.AscendingNode));
                        elements->setArgumentOfPeriapsis(sta::radToDeg(initialStateKeplerian.ArgumentOfPeriapsis));
                        elements->setTrueAnomaly(sta::radToDeg(initialStateKeplerian.TrueAnomaly));
                        RVmanoeuvre->InitialPosition()->setAbstract6DOFPosition(QSharedPointer<ScenarioAbstract6DOFPositionType>(elements));
                    }
                    else
                    {
                        ScenarioStateVectorType* stateVector = new ScenarioStateVectorType();
                        stateVector->setX(theLastStateVector.position(0));
                        stateVector->setY(theLastStateVector.position(1));
                        stateVector->setZ(theLastStateVector.position(2));
                        stateVector->setVx(theLastStateVector.velocity(0));
                        stateVector->setVy(theLastStateVector.velocity(1));
                        stateVector->setVz(theLastStateVector.velocity(2));
                        RVmanoeuvre->InitialPosition()->setAbstract6DOFPosition(QSharedPointer<ScenarioAbstract6DOFPositionType>(stateVector));
                    }
                }
                //Propagation method, as loitering.
                //No OOP, no multilayer. This should be changed in my opinion.

                PropagateRendezVousTrajectory(RVmanoeuvre, sampleTimesTrajectory, samplesTrajectory, feedback, propScenario);

                //
                sta::StateVector* targetStateAux=new sta::StateVector();

                QString coordSysName = RVmanoeuvre->InitialPosition()->CoordinateSystem();
                sta::CoordinateSystem coordSys(coordSysName);
                SpaceObject* targetPropagated;

                targetPropagated=(propScenario->spaceObjects().first());

                sta::StateVector *result=new sta::StateVector();

                for(int i=0; i<samplesTrajectory.size(); i++)
                {

                    targetPropagated->mission().at(0)->getStateVector(sampleTimesTrajectory[i],targetStateAux);

                    // Final samples transformation

                    lhlvTOinertial(mu,*targetStateAux,samplesTrajectory[i],result);

                    //Data in inertial system are saved in samples.
                    samplesTrajectory[i].position(0)=result->position(0)+targetStateAux->position(0);
                    samplesTrajectory[i].position(1)=result->position(1)+targetStateAux->position(1);
                    samplesTrajectory[i].position(2)=result->position(2)+targetStateAux->position(2);
                    samplesTrajectory[i].velocity(0)=targetStateAux->velocity(0);
                    samplesTrajectory[i].velocity(1)=targetStateAux->velocity(1);
                    samplesTrajectory[i].velocity(2)=targetStateAux->velocity(2);

                }

                // Recovering the last state vector
                numberOFsamples = sampleTimesTrajectory.size();
                theLastStateVector = samplesTrajectory.at(numberOFsamples - 1);
                theLastSampleTime = sampleTimesTrajectory.at(numberOFsamples - 1);


                //******************************************************************** /OZGUN
                // Eclipse function is called and the "data/EclipseStarLight.stad" is generated
                EclipseDuration* Eclipse = new EclipseDuration();

                Eclipse->StarLightTimeFunction(sampleTimesTrajectory,
                                               samplesTrajectory,
                                               STA_SOLAR_SYSTEM->lookup("Earth"),
                                               STA_SOLAR_SYSTEM->lookup("Sun"));

                //******************************************************************** OZGUN/

                if (feedback.status() != PropagationFeedback::PropagationOk)
                {
                    return;
                }

                //QString centralBodyName = RVmanoeuvre->Environment()->CentralBody()->Name();
                //StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
                if (!centralBody)
                {
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized central body");
                    continue;
                }

                //QString coordSysName = RVmanoeuvre->InitialPosition()->CoordinateSystem();
                //sta::CoordinateSystem coordSys(coordSysName);
                if (coordSys.type() == sta::COORDSYS_INVALID)
                {
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized coordinate system");
                    continue;
                }


                if (numberOFsamples > 1)
                {
                    MissionArc* arc = new MissionArc(centralBody,
                                                     coordSys,
                                                     sampleTimesTrajectory,
                                                     samplesTrajectory);

                    // Loading arc color, name, and model
                    arc->setArcName(RVmanoeuvre->ElementIdentifier()->Name());
                    QString arcColorName = RVmanoeuvre->ElementIdentifier()->colorName();
                    MissionsDefaults myMissionDefaults;
                    QColor trajectoryColor = myMissionDefaults.missionArcColorFromQt(arcColorName);
                    arc->setArcTrajectoryColor(trajectoryColor);
                    arc->setModelName(RVmanoeuvre->ElementIdentifier()->modelName());

                    spaceObject->addMissionArc(arc);
                }
            }

            /////////////////////////// end of the big IF for all arcs
        }

        // Claas and Steffen : Add Transmitter and Receiver to the structure to study constellations
        const QList<QSharedPointer<ScenarioAbstractPayloadType> >& payloadList = vehicle->SCMission()->PayloadSet()->AbstractPayload();

        foreach (QSharedPointer<ScenarioAbstractPayloadType> payload, payloadList)
        {
            if (dynamic_cast<ScenarioReceiverPayloadType*>(payload.data()))    // Receiver
            {
                ScenarioReceiver* rec = dynamic_cast<ScenarioReceiverPayloadType*>(payload.data())->Receiver().data();
                PSAntennaObject* psAntennaObject = new PSAntennaObject();
                psAntennaObject->setAzimuth(rec->PointingDirection()->azimuth());
                psAntennaObject->setElevation(rec->PointingDirection()->elevation());
                psAntennaObject->setConeAngle(rec->PointingDirection()->coneAngle());
                psAntennaObject->setConeShape(rec->PointingDirection()->coneShape());
                spaceObject->addReceiver(psAntennaObject);
            }
            if (dynamic_cast<ScenarioTransmitterPayloadType*>(payload.data())) // Transmitter
            {
                ScenarioTransmitter* tra = dynamic_cast<ScenarioTransmitterPayloadType*>(payload.data())->Transmitter().data();
                PSAntennaObject* psAntennaObject = new PSAntennaObject();
                psAntennaObject->setAzimuth(tra->PointingDirection()->azimuth());
                psAntennaObject->setElevation(tra->PointingDirection()->elevation());
                psAntennaObject->setConeAngle(sta::degToRad(tra->Coverage()->FrustumAngle2()));
                if (tra->Coverage()->FrustumShape()=="ellipse")
                {
                    psAntennaObject->setConeShape(1);
                }
                else
                {
                    psAntennaObject->setConeShape(0);
                }
                psAntennaObject->setObservationChecked(tra->ObservationChecked());
                spaceObject->addTransmitter(psAntennaObject);
            }
        }

        propScenario->addSpaceObject(spaceObject);
	}
}


void scenarioPropagatorReEntryVehicle(ScenarioREV* entryVehicle, PropagationFeedback& feedback, PropagatedScenario* propScenario, QWidget* parent)
{
    const QList<QSharedPointer<ScenarioAbstractTrajectoryType> >& trajectoryList = entryVehicle->REVMission()->REVTrajectoryPlan()->AbstractTrajectory();

    if(!trajectoryList.isEmpty())
    {
        SpaceObject* spaceObject = new SpaceObject(entryVehicle);
        spaceObject->setName(entryVehicle->Name());

        // Propagate all segments of the trajectory plan.
        foreach (QSharedPointer<ScenarioAbstractTrajectoryType> trajectory, trajectoryList)
        {
            QList<double> sampleTimes;
            QList<sta::StateVector> samples;

            if (dynamic_cast<ScenarioEntryArcType*>(trajectory.data()))
            {
                ScenarioEntryArcType* entry = dynamic_cast<ScenarioEntryArcType*>(trajectory.data());
                PropagateEntryTrajectory(entryVehicle, entry, sampleTimes, samples, feedback);

                if (feedback.status() != PropagationFeedback::PropagationOk)
                {
                    // An error occurred during propagate. Clean up everything and return immediately.
                    // The current propagation results will not be replaced.
                    if (feedback.status() == PropagationFeedback::PropagationCanceled)
                    {
                        int ret = QMessageBox::warning(parent, "Canceled", "Propagation was canceled.");
                    }
                    else
                    {
                        int ret = QMessageBox::critical(parent, "Propagation Error", "Error during propagation");
                    }

                    delete propScenario;
                    return;
                }

                QString centralBodyName = entry->Environment()->CentralBody()->Name();
                StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
                if (!centralBody)
                {
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized central body");
                    continue;
                }

                QString coordSysName = entry->InitialPosition()->CoordinateSystem();
                sta::CoordinateSystem coordSys(coordSysName);
                if (coordSys.type() == sta::COORDSYS_INVALID)
                {
                    int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized coordinate system");
                    continue;
                }
                MissionArc* arc = new MissionArc(centralBody,
                                                 coordSys,
                                                 sampleTimes,
                                                 samples);

                // Loading arc color, name, and model
                arc->setArcName(entry->ElementIdentifier()->Name());
                QString arcColorName = entry->ElementIdentifier()->colorName();
                MissionsDefaults myMissionDefaults;
                QColor trajectoryColor = myMissionDefaults.missionArcColorFromQt(arcColorName);
                // Yellow for the time being
                arc->setArcTrajectoryColor("Yellow");
                //arc->setArcTrajectoryColor(trajectoryColor);
                arc->setModelName(entry->ElementIdentifier()->modelName());
                spaceObject->addMissionArc(arc);
            }
        }



        propScenario->addSpaceObject(spaceObject);
    }
}




void scenarioPropagatorReEntryVehicle(ScenarioREV* entryVehicle,  QColor trajectoryColor, PropagationFeedback& feedback, PropagatedScenario* propScenario)
{
    const QList<QSharedPointer<ScenarioAbstractTrajectoryType> >& trajectoryList = entryVehicle->REVMission()->REVTrajectoryPlan()->AbstractTrajectory();

    if(!trajectoryList.isEmpty())
    {
        SpaceObject* spaceObject = new SpaceObject(entryVehicle);
        spaceObject->setName(entryVehicle->Name());
        spaceObject->setTrajectoryColor(trajectoryColor);
        foreach (QSharedPointer<ScenarioAbstractTrajectoryType> trajectory, trajectoryList)
        {
            QList<double> sampleTimes;
            QList<sta::StateVector> samples;

            if (dynamic_cast<ScenarioEntryArcType*>(trajectory.data()))
            {
                ScenarioEntryArcType* entry = dynamic_cast<ScenarioEntryArcType*>(trajectory.data());
                PropagateEntryTrajectory(entryVehicle, entry, sampleTimes, samples, feedback);
                if (feedback.status() != PropagationFeedback::PropagationOk)
                {
                    // An error occurred during propagate. Clean up everything and return immediately.
                    // The current propagation results will not be replaced.
                    if (feedback.status() == PropagationFeedback::PropagationCanceled)
                    {
                        //QMessageBox::information(this, tr("Canceled"), tr("Propagation was canceled."));
                    }
                    else
                    {
                        //QMessageBox::critical(this, tr("Propagation Error"), tr("Error during propagation: %1").arg(feedback.errorString()));
                    }

                    delete propScenario;
                    return;
                }

                QString centralBodyName = entry->Environment()->CentralBody()->Name();
                StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
                if (!centralBody)
                {
                    //QMessageBox::warning(this, tr("Propagation Error"), tr("Unrecognized body '%1'").arg(centralBodyName));
                    continue;
                }

                QString coordSysName = entry->InitialPosition()->CoordinateSystem();
                sta::CoordinateSystem coordSys(coordSysName);
                if (coordSys.type() == sta::COORDSYS_INVALID)
                {
                    //QMessageBox::warning(this, tr("Propagation Error"), tr("Unrecognized coordinate system '%1'").arg(coordSysName));
                    continue;
                }
                MissionArc* arc = new MissionArc(centralBody,
                                                 coordSys,
                                                 sampleTimes,
                                                 samples);
                // Loading arc color, name, and model
                arc->setArcName(entry->ElementIdentifier()->Name());
                QString arcColorName = entry->ElementIdentifier()->colorName();
                MissionsDefaults myMissionDefaults;
                QColor trajectoryColor = myMissionDefaults.missionArcColorFromQt(arcColorName);
                arc->setArcTrajectoryColor(trajectoryColor);
                arc->setModelName(entry->ElementIdentifier()->modelName());
                spaceObject->addMissionArc(arc);
            }
        }
        propScenario->addSpaceObject(spaceObject);
    }
}




void scenarioPropagatorLaunchVehicle(ScenarioLV* vehicle,  QColor trajectoryColor, PropagationFeedback& feedback, PropagatedScenario* propScenario)
{
    // Nothing for STA
}




void scenarioPropagatorGroundElement(ScenarioGroundStation* groundElement, PropagationFeedback& feedback, PropagatedScenario* propScenario, QWidget* parent)
{
    QSharedPointer<ScenarioAbstract3DOFPositionType> position = groundElement->Location()->Abstract3DOFPosition();
    QSharedPointer<ScenarioGroundPositionType> groundPosition = qSharedPointerDynamicCast<ScenarioGroundPositionType>(position);

    if (!groundPosition.isNull())
    {
        StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(groundElement->Location()->CentralBody());
        if (centralBody)
        {
            GroundObject* groundObject = new GroundObject();

            groundObject->centralBody = centralBody;
            groundObject->longitude = groundPosition->longitude();
            groundObject->latitude = groundPosition->latitude();
            groundObject->altitude = groundPosition->altitude();

            // Loading station color, name, and model
            QString stationName = groundElement->ElementIdentifier()->Name();
            //groundObject->name = groundElement->Name();
            groundObject->name = stationName;
            QString stationColorName = groundElement->ElementIdentifier()->colorName();
            MissionsDefaults myMissionDefaults;
            QColor trajectoryColor = myMissionDefaults.missionArcColorFromQt(stationColorName);
            QString arcModelName = groundElement->ElementIdentifier()->modelName();

            // Claas and Steffen : Add Transmitter and Receiver to the structure to study constellations
            const QList<QSharedPointer<ScenarioAbstractPayloadType> >& payloadList = groundElement->PayloadSet()->AbstractPayload();
            foreach (QSharedPointer<ScenarioAbstractPayloadType> payload, payloadList)
            {
                if (dynamic_cast<ScenarioReceiverPayloadType*>(payload.data()))    // Receiver
                {
                    ScenarioReceiver* rec = dynamic_cast<ScenarioReceiverPayloadType*>(payload.data())->Receiver().data();
                    PSAntennaObject* psAntennaObject = new PSAntennaObject();
                    psAntennaObject->setAzimuth(rec->PointingDirection()->azimuth());
                    psAntennaObject->setElevation(rec->PointingDirection()->elevation());
                    psAntennaObject->setConeAngle(rec->PointingDirection()->coneAngle());
                    psAntennaObject->setConeShape(rec->PointingDirection()->coneShape());
                    groundObject->addReceiver(psAntennaObject);
                }
                if (dynamic_cast<ScenarioTransmitterPayloadType*>(payload.data())) // Transmitter
                {
                    ScenarioTransmitter* tra = dynamic_cast<ScenarioTransmitterPayloadType*>(payload.data())->Transmitter().data();
                    PSAntennaObject* psAntennaObject = new PSAntennaObject();
                    psAntennaObject->setAzimuth(tra->PointingDirection()->azimuth());
                    psAntennaObject->setElevation(tra->PointingDirection()->elevation());
                    psAntennaObject->setConeAngle(sta::degToRad(tra->Coverage()->FrustumAngle2()));
                    if (tra->Coverage()->FrustumShape()=="ellipse")
                    {
                        psAntennaObject->setConeShape(1);
                    }
                    else
                    {
                        psAntennaObject->setConeShape(0);
                    }
                    psAntennaObject->setObservationChecked(tra->ObservationChecked());
                    groundObject->addTransmitter(psAntennaObject);
                }
            }

            propScenario->addGroundObject(groundObject);
        }
        else
        {
            int ret = QMessageBox::warning(parent, "Propagation Error", "Unrecognized central body");
        }
    }
}




void scenarioPropagatorGroundElement(ScenarioGroundStation* groundElement,  QColor trajectoryColor, PropagationFeedback& feedback, PropagatedScenario* propScenario)
{
    QSharedPointer<ScenarioAbstract3DOFPositionType> position = groundElement->Location()->Abstract3DOFPosition();
    QSharedPointer<ScenarioGroundPositionType> groundPosition = qSharedPointerDynamicCast<ScenarioGroundPositionType>(position);

    if (!groundPosition.isNull())
    {
        StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(groundElement->Location()->CentralBody());
        if (centralBody)
        {
            GroundObject* groundObject = new GroundObject();

            groundObject->name = groundElement->Name();
            groundObject->centralBody = centralBody;
            groundObject->longitude = groundPosition->longitude();
            groundObject->latitude = groundPosition->latitude();
            groundObject->altitude = groundPosition->altitude();

            // Claas and Steffen : Add Transmitter and Receiver to the structure to study constellations
            const QList<QSharedPointer<ScenarioAbstractPayloadType> >& payloadList = groundElement->PayloadSet()->AbstractPayload();

            foreach (QSharedPointer<ScenarioAbstractPayloadType> payload, payloadList)
            {
                if (dynamic_cast<ScenarioReceiverPayloadType*>(payload.data()))    // Receiver
                {
                    ScenarioReceiver* rec = dynamic_cast<ScenarioReceiverPayloadType*>(payload.data())->Receiver().data();
                    PSAntennaObject* psAntennaObject = new PSAntennaObject();
                    psAntennaObject->setAzimuth(rec->PointingDirection()->azimuth());
                    psAntennaObject->setElevation(rec->PointingDirection()->elevation());
                    psAntennaObject->setConeAngle(rec->PointingDirection()->coneAngle());
                    psAntennaObject->setConeShape(rec->PointingDirection()->coneShape());
                    groundObject->addReceiver(psAntennaObject);
                }
                if (dynamic_cast<ScenarioTransmitterPayloadType*>(payload.data())) // Transmitter
                {
                    ScenarioTransmitter* tra = dynamic_cast<ScenarioTransmitterPayloadType*>(payload.data())->Transmitter().data();
                    PSAntennaObject* psAntennaObject = new PSAntennaObject();
                    psAntennaObject->setAzimuth(tra->PointingDirection()->azimuth());
                    psAntennaObject->setElevation(tra->PointingDirection()->elevation());
                    psAntennaObject->setConeAngle(sta::degToRad(tra->Coverage()->FrustumAngle2()));
                    if (tra->Coverage()->FrustumShape()=="ellipse")
                    {
                        psAntennaObject->setConeShape(1);
                    }
                    else
                    {
                        psAntennaObject->setConeShape(0);
                    }
                    psAntennaObject->setObservationChecked(tra->ObservationChecked());
                    groundObject->addTransmitter(psAntennaObject);
                }
            }

            propScenario->addGroundObject(groundObject);
        }
    }
}



void scenarioPropagatorPoint(ScenarioPoint* aPoint,  QColor trajectoryColor, PropagationFeedback& feedback, PropagatedScenario* propScenario)
{
    QSharedPointer<ScenarioAbstract3DOFPositionType> position = aPoint->Location()->Abstract3DOFPosition();
    QSharedPointer<ScenarioGroundPositionType> groundPosition = qSharedPointerDynamicCast<ScenarioGroundPositionType>(position);
    if (!groundPosition.isNull())
    {
        StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(aPoint->Location()->CentralBody());

        if (centralBody)
        {
            GroundObject* groundObject = new GroundObject();

            groundObject->name = aPoint->Name();
            groundObject->centralBody = centralBody;
            groundObject->longitude = groundPosition->longitude();
            groundObject->latitude = groundPosition->latitude();
            groundObject->altitude = groundPosition->altitude();

            propScenario->addGroundObject(groundObject);
        }
    }

}


void scenarioPropagatorPoint(ScenarioPoint* aPoint,  PropagationFeedback& feedback, PropagatedScenario* propScenario)
{
    QSharedPointer<ScenarioAbstract3DOFPositionType> position = aPoint->Location()->Abstract3DOFPosition();
    QSharedPointer<ScenarioGroundPositionType> groundPosition = qSharedPointerDynamicCast<ScenarioGroundPositionType>(position);
    if (!groundPosition.isNull())
    {
        StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(aPoint->Location()->CentralBody());

        if (centralBody)
        {
            GroundObject* groundObject = new GroundObject();

            groundObject->name = aPoint->Name();
            groundObject->centralBody = centralBody;
            groundObject->longitude = groundPosition->longitude();
            groundObject->latitude = groundPosition->latitude();
            groundObject->altitude = groundPosition->altitude();

            propScenario->addGroundObject(groundObject);
        }
    }

}


void
        scenarioPropagatorRegion(ScenarioRegion* region, PropagationFeedback& feedback, PropagatedScenario* propScenario)
{
    StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(region->CentralBody());

    if (centralBody)
    {
        QList<Vector2d> boundary;

        QList<double> boundaryData = region->Boundary();

        // Each pair of values in the ScenarioRegion's boundary represents a point on the surface of
        // the central body. Verify that we have an even number of values.
        if (boundaryData.size() % 2 != 0)
        {
            return;
        }

        unsigned int pointCount = boundaryData.size() / 2;

        // We need at least three points to define the region
        if (pointCount < 3)
        {
            return;
        }

        for (unsigned int i = 0; i < pointCount; ++i)
        {
            // Note that while the surface coordinates of a boundary point appear in the
            // order longitude first, latitude second, they are stored with longitude in
            // the x component of the vector and latitude in y.
            boundary << Vector2d(boundaryData[i * 2 + 1], boundaryData[i * 2]);
        }

        RegionObject* regionObject = new RegionObject(region->Name(), centralBody);
        regionObject->setBoundary(boundary);
        MissionsDefaults myMissionDefaults;
        regionObject->setColor(myMissionDefaults.missionArcColorFromQt(region->ElementIdentifier()->colorName()));
        propScenario->addRegionObject(regionObject);
    }
}
