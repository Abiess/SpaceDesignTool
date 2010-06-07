/*
 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA 02111-1307, USA.
 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.
 */

/*
 ------ Copyright (C) 2009 European Space Agency (space.trajectory.analysis AT gmail.com) ----
 ------------------ Author: Claas Grohnfeldt, Steffen Peter  -------------------------------------------------
 ------------------ E-mail: (claasgr@math.uni-bremen.de, spezisteffen@gmx.de) ----------------------------
 */

#include "Constellations/constellationwizard.h"
#include "Constellations/constellationmodule.h"
#include "Scenario/scenario.h"
#include "Main/mainwindow.h"
#include "Loitering/loitering.h"
#include "Astro-Core/stamath.h"
#include <QMessageBox>
#include <QDebug>



ConstellationWizardDialog::ConstellationWizardDialog(MainWindow* parent) :
        mainwindow(parent)
{
    setupUi(this);

    // Set up the input validators
    QDoubleValidator* doublePosValidator = new QDoubleValidator(this);
    doublePosValidator->setBottom(0.0);
    QDoubleValidator* angleValidator = new QDoubleValidator(0.0,359.9999,4,this);
    angleValidator->setRange(0.0, 365.9999,4);
    QDoubleValidator* angleRangeValidator = new QDoubleValidator(0.0,360.0,4,this);

    numberOfOrbitPlanesLineEdit->setValidator(new QIntValidator(1,10,this));
    numberOfSatellitesLineEdit->setValidator(new QIntValidator(1,20,this));
    semiMajoraxisLineEdit->setValidator(doublePosValidator);
    inclinationLineEdit->setValidator(new QDoubleValidator(0.0, 179.9999, 4, this));
    relSpacingLineEdit->setValidator(new QIntValidator(0,9,this));

    numberOfPetalsLineEdit->setValidator(new QIntValidator(0,12,this));
    numberOfDaysToRepeatGroundTrackLineEdit->setValidator(new QIntValidator(0,50,this));
    numberOfSatellitesLineEdit_2->setValidator(new QIntValidator(0,36,this));
    perigeeLineEdit->setValidator(angleValidator);
    heightOfPerigeeLineEdit->setValidator(doublePosValidator);
    raanOfFirstSatelliteLineEdit->setValidator(angleValidator);
    initialTrueAnomalyOfFirstSatelliteLineEdit->setValidator(angleValidator);
    limitedOrbitNodeRangeLineEdit->setValidator(angleRangeValidator);

    numberOfSatellitesLineEdit_3->setValidator(new QIntValidator(1,36,this));

    // signals
    QObject::connect(schemeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(disableflower()));


}

ConstellationWizardDialog::~ConstellationWizardDialog()
{
}

void ConstellationWizardDialog::disableflower()
{
    if (schemeComboBox->currentIndex()==0) limitedOrbitNodeRangeLineEdit->setEnabled(false);
    else limitedOrbitNodeRangeLineEdit->setEnabled(true);
}


void ConstellationWizardDialog::accept()
{

    // close Constellation Wizard
    close();

    /* Guillermo disables this class

    // open Loitering Dialog and get additional Properties
    LoiteringDialog* loiteringdialog = new LoiteringDialog();
    loiteringdialog->setFocus();
    loiteringdialog->disableInitialStateParameters();
    loiteringdialog->exec();
    loiteringdialog->clearFocus();

    // generate keplerian elements of the satellites
    SatelliteKeplerian* satellitekeplerian;
    int n; // number of Satellites
    if (constTypeComboBox->currentIndex() == 0) // Walker
    {
        bool ok;
        int npl = numberOfOrbitPlanesLineEdit->text().toInt(&ok,10);
        int nspl = numberOfSatellitesLineEdit->text().toInt(&ok,10);
        double a = semiMajoraxisLineEdit->text().toDouble(&ok);
        double inc = inclinationLineEdit->text().toDouble(&ok);
        int f = relSpacingLineEdit->text().toInt(&ok,10);
        n = npl * nspl;
        satellitekeplerian = new SatelliteKeplerian[n];
        if (!walkerConstellation(npl, nspl, a, inc, f, satellitekeplerian))
        {
            QMessageBox::warning(this, "Error", "Not able to generate Walker Constellation!");
            return;
        }
    }
    else if (constTypeComboBox->currentIndex() == 1) // Flower
    {
        bool ok;
        double fakdg = sta::Pi()/180.0; // transformation factor from deg -> rad
        // get parameters from Wizard
        int np = numberOfPetalsLineEdit->text().toInt(&ok,10);
        int nd = numberOfDaysToRepeatGroundTrackLineEdit->text().toInt(&ok,10);
        n = numberOfSatellitesLineEdit_2->text().toInt(&ok,10);
        double om = fakdg * perigeeLineEdit->text().toDouble(&ok);
        double inc;
        if (inclinationComboBox->currentIndex()==0) inc = 1.1065; // 63.4 degree in rad
        else inc = 2.0351; // 116.6 degree in rad
        double hp = heightOfPerigeeLineEdit->text().toDouble(&ok);
        bool scheme;
        if (schemeComboBox->currentIndex()==0) scheme = true; // symmetric
        else scheme = false; // asymmetric
        double omega1 = fakdg * raanOfFirstSatelliteLineEdit->text().toDouble(&ok);
        double psi1 = fakdg * initialTrueAnomalyOfFirstSatelliteLineEdit->text().toDouble(&ok);
        double domega;
        if (!scheme) domega = fakdg * limitedOrbitNodeRangeLineEdit->text().toDouble(&ok);
        // get central body data
        ScenarioEnvironment* env = new ScenarioEnvironment();
        loiteringdialog->saveValues(env);
        const StaBody* body =  env->centralBody()->body();
        // rotation angular velocity
        double rotV = 1/(body->rotationState()->rotationPeriod()*86400);
        if (body->name()=="Earth") {
            rotV = 2*sta::Pi()/(body->rotationState()->rotationPeriod()*86400);
        }
        // generate Flower Constellation
        satellitekeplerian = new SatelliteKeplerian[n];
        if (!flowerConstellation(np, nd, n, om, inc, hp, scheme, omega1, psi1, domega,
                                 body->meanRadius(), rotV, body->mu(),body->gravityModel().J2(), satellitekeplerian)){
            QMessageBox::warning(this, "Error", "Not able to generate Flower Constellation!");
            return;
        }
    }
    else if (constTypeComboBox->currentIndex() == 2) // Self-Defined
    {
        bool ok;
        n = numberOfSatellitesLineEdit_3->text().toInt(&ok,10);
        satellitekeplerian = new SatelliteKeplerian[n];
        if (!selfDefinedConstellation(n, satellitekeplerian))
        {
            QMessageBox::warning(this, "Error", "Not able to generate Flower Constellation!");
            return;
        }
    }
    else if (constTypeComboBox->currentIndex() == 3)
    {
        QMessageBox::warning(this, "", "Feature is not implemented yet!");
        return;
    }
    else if (constTypeComboBox->currentIndex() == 4)
    {
        QMessageBox::warning(this, "", "Feature is not implemented yet!");
        return;
    }

    SpaceScenario* scenario = new SpaceScenario();
    scenario->setName(constTypeComboBox->currentText());

    // create new Participants, Properties and trajectories
    for (int i = 0; i < n; i++)
    {

        ScenarioSpaceVehicle* satellite;
        satellite = new ScenarioSpaceVehicle();
        satellite->setType("Satellite");
        satellite->setName(satellitekeplerian[i].name);
        scenario->addParticipant(satellite);

        ScenarioTrajectoryPlan* trajectoryplan = new ScenarioTrajectoryPlan();

        satellite->setTrajectoryPlan(trajectoryplan);

        ScenarioLoiteringTrajectory* loiteringtrajectory = new ScenarioLoiteringTrajectory();
        loiteringdialog->saveValues(loiteringtrajectory->environment());
        loiteringdialog->saveValues(loiteringtrajectory->trajectoryPropagation());
        trajectoryplan->addTrajectory(loiteringtrajectory);

        ScenarioSimulationParameters* simulationparameters = new ScenarioSimulationParameters(0);
        StaBody* body = (StaBody*) loiteringtrajectory->environment()->centralBody()->body();
        ScenarioBody* centralsystemBody = new ScenarioBody(body);
        ScenarioExtendedTimeline* timeline = new ScenarioExtendedTimeline();
        simulationparameters->setTimeline(timeline);
        ScenarioInitialStateAttitude* initialstateattitude = new ScenarioInitialStateAttitude();
        simulationparameters->setInitialStateAttitude(initialstateattitude);
        ScenarioInitialStatePosition* initialstateposition = new ScenarioInitialStatePosition();
        initialstateposition->setCentralsystemBody(centralsystemBody);
        simulationparameters->setInitialStatePosition(initialstateposition);

        ScenarioKeplerianElements* keplerian = new ScenarioKeplerianElements();
        keplerian->m_semimajorAxis = satellitekeplerian[i].param[0];
        keplerian->m_eccentricity = satellitekeplerian[i].param[1];
        keplerian->m_inclination = satellitekeplerian[i].param[2];
        keplerian->m_raan = satellitekeplerian[i].param[3];
        keplerian->m_argumentOfPeriapsis = satellitekeplerian[i].param[4];
        keplerian->m_trueAnomaly = satellitekeplerian[i].param[5];

        initialstateposition->setInitialState(keplerian);

        loiteringtrajectory->setSimulationParameters(simulationparameters);
        loiteringdialog->saveValuesConstellation(simulationparameters);
    }

    mainwindow->setScenario(scenario);
    return;

*/


}

