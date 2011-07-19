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
 ------------------ Author:       Guillermo Ortega               -------------------
 ------------------ Affiliation:  European Space Agency (ESA)    -------------------
 -----------------------------------------------------------------------------------
 Modified by Tiziana Sabatini on July 2009 to support the new perturbations layer
 Extensively modified by Guillermo on August 2010 to comply with the new schema v2.0
 Patched by Catarina to add attitude, July 2011
*/

#include "Loitering/loitering.h"

#include "Main/scenariotree.h"

#include "Scenario/scenario.h"
#include "Scenario/propagationfeedback.h"
#include "Scenario/missionAspectDialog.h"
#include "Scenario/missionAspectDialog.h"

#include "Astro-Core/date.h"
#include "Astro-Core/stacoordsys.h"
#include "Astro-Core/stabody.h"
#include "Astro-Core/stamath.h"
#include "Astro-Core/trueAnomalyTOmeanAnomaly.h"
#include "Astro-Core/cartesianTOorbital.h"
#include "Astro-Core/orbitalTOcartesian.h"
#include "Astro-Core/propagateTWObody.h"
#include "Astro-Core/propagateCOWELL.h"
#include "Astro-Core/propagateENCKE.h"
#include "Astro-Core/propagateGAUSS.h"
#include "Astro-Core/attitudevector.h"
#include "Astro-Core/attitudeintegration.h"
#include "Astro-Core/attitudetransformations.h"

#include "ui_missionAspectDialog.h"

#include "Eigen/Geometry"

#include <QWidget>
#include <QTextStream>
#include <QDebug>
#include <QFile>

using namespace Eigen;

static const unsigned int MAX_OUTPUT_STEPS = 1000000;


LoiteringDialog::LoiteringDialog(ScenarioTree* parent) :
        QDialog(parent),
        m_tesserals(0)
{
    setupUi(this);
    // Set up the combo boxes
    CoordSystemComboBox->addItem(tr("Planet Fixed"), (int) sta::COORDSYS_BODYFIXED);
    CoordSystemComboBox->addItem(tr("Inertial (J2000)"), (int) sta::COORDSYS_EME_J2000);
    CoordSystemComboBox->addItem(tr("Inertial (B1950)"), (int) sta::COORDSYS_EME_B1950);
    CoordSystemComboBox->addItem(tr("Ecliptic (J2000)"), (int) sta::COORDSYS_ECLIPTIC_J2000);

    PropagatorComboBox->addItem(tr("Two Body"), "TWO BODY");
    PropagatorComboBox->addItem(tr("Gauss"), "GAUSS");
    PropagatorComboBox->addItem(tr("Cowell"), "COWELL");
    PropagatorComboBox->addItem(tr("Encke"), "ENCKE");

    IntegratorComboBox->addItem(tr("Runge-Kutta 3-4"), "RK4");
    IntegratorComboBox->addItem(tr("Runge-Kutta-Fehlberg"), "RKF");

    IntegratorComboBox_3->addItem(tr("Runge-Kutta 3-4"),"RK4"); //Catarina: Integrator for Attitude

    // Set up the input validators
    QDoubleValidator* doubleValidator = new QDoubleValidator(this);
    QDoubleValidator* angleValidator = new QDoubleValidator(this);
    angleValidator->setBottom(0.0);
    angleValidator->setTop(360.0);
    QDoubleValidator* positiveDoubleValidator = new QDoubleValidator(this);
    positiveDoubleValidator->setBottom(0.0);
    QDoubleValidator* zeroToOneValidator = new QDoubleValidator(this);
    zeroToOneValidator->setBottom(0.0);
    zeroToOneValidator->setTop(0.9999);
    QDoubleValidator* minusOneToOneValidator = new QDoubleValidator(this);
    minusOneToOneValidator->setBottom(-1.0);
    minusOneToOneValidator->setTop(1.0);

    TimeStepLineEdit->setValidator(positiveDoubleValidator);

    positionXEdit->setValidator(doubleValidator);
    positionYEdit->setValidator(doubleValidator);
    positionZEdit->setValidator(doubleValidator);
    velocityXEdit->setValidator(doubleValidator);
    velocityYEdit->setValidator(doubleValidator);
    velocityZEdit->setValidator(doubleValidator);

    semimajorAxisEdit->setValidator(positiveDoubleValidator);
    eccentricityEdit->setValidator(zeroToOneValidator);
    inclinationEdit->setValidator(angleValidator);
    raanEdit->setValidator(angleValidator);
    argOfPeriapsisEdit->setValidator(angleValidator);
    trueAnomalyEdit->setValidator(angleValidator);

    //positionXEdit_2->setValidator(doubleValidator);
    //positionYEdit_2->setValidator(doubleValidator);
    //positionZEdit_2->setValidator(doubleValidator);

    //Check if the inputs are valid
    Euler123phi->setValidator(angleValidator);
    Euler123theta->setValidator(angleValidator);
    Euler123psi->setValidator(angleValidator);

    //    AngularVelocityXEdit->setValidator(doubleValidator);
    //    AngularVelocityYEdit->setValidator(doubleValidator);
    //    AngularVelocityZEdit->setValidator(doubleValidator);

    //    quaternion1Edit->setValidator(minusOneToOneValidator);
    //    quaternion2Edit->setValidator(minusOneToOneValidator);
    //    quaternion3Edit->setValidator(minusOneToOneValidator);
    //    quaternion4Edit->setValidator(minusOneToOneValidator);

    //    AngularVelocityXEdit_321->setValidator(doubleValidator);
    //    AngularVelocityYEdit_321->setValidator(doubleValidator);
    //    AngularVelocityZEdit_321->setValidator(doubleValidator);

    //Added combo box to select atmosphere models
    // Lines patched by Guillermo to take away the icons
    AtmosphereComboBox->addItem(tr("GRAM 99"), "gram99");
    AtmosphereComboBox->addItem(tr("Exponential"), "exponential");
    AtmosphereComboBox->addItem(tr("US 76"), "ussa1976");
    AtmosphereComboBox->addItem(tr("Mars EMCD"), "mars_emcd");

    // Lines patched by Guillermo to take away the icons
    GravityComboBox->addItem(tr("EGM 2008"), "EGM2008");
    GravityComboBox->addItem(tr("GTM 090"), "GTM090");
    GravityComboBox->addItem(tr("SHGJ 180"), "SHGJ180");
    GravityComboBox->addItem(tr("JGL 165"), "JGL165");

    AtmosphereComboBox->setEnabled(false);
    GravityComboBox->setEnabled(false);

    IntStepEdit->setValidator(positiveDoubleValidator);
    IntStepEdit_2->setValidator(positiveDoubleValidator);

    ZonalsSpinBox->setMaximum(20);
    //Add the tesserals SpinBox
    TesseralSpinBox = new TesseralBox(this);
    TesseralSpinBox->setObjectName(QString::fromUtf8("TesseralSpinBox"));
    TesseralSpinBox->setEnabled(false);
    TesseralSpinBox->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    TesseralSpinBox->setMaximum(ZonalsSpinBox->value());
    TesseralSpinBox->setValue(m_tesserals);

    gridLayout_4->addWidget(TesseralSpinBox, 2, 1, 1, 1);

    SolarPressureRadioButton->setCheckable(true);
    SolarPressureRadioButton->setChecked(false);
    AlbedoCheckBox->setCheckable(true);
    AlbedoCheckBox->setChecked(false);
    IRCheckBox->setCheckable(true);
    IRCheckBox->setChecked(false);

    connect(GravityFieldRadioButton, SIGNAL(toggled(bool)), GravityComboBox, SLOT(setEnabled(bool)));
    connect(GravityFieldRadioButton, SIGNAL(toggled(bool)), ZonalsSpinBox, SLOT(setEnabled(bool)));
    connect(GravityFieldRadioButton, SIGNAL(toggled(bool)), TesseralSpinBox, SLOT(setEnabled(bool)));
    connect(AtmDragRadioButton, SIGNAL(toggled(bool)), AtmosphereComboBox, SLOT(setEnabled(bool)));
    connect(SolarPressureRadioButton, SIGNAL(toggled(bool)), CrSpinBox, SLOT(setEnabled(bool)));
    connect(SolarPressureRadioButton, SIGNAL(toggled(bool)), AlbedoCheckBox, SLOT(setEnabled(bool)));
    connect(SolarPressureRadioButton, SIGNAL(toggled(bool)), IRCheckBox, SLOT(setEnabled(bool)));
    connect(SolarPressureRadioButton, SIGNAL(clicked(bool)), AlbedoCheckBox, SLOT(setChecked(bool)));
    connect(SolarPressureRadioButton, SIGNAL(clicked(bool)), IRCheckBox, SLOT(setChecked(bool)));
    connect(PertBodyRadioButton, SIGNAL(toggled(bool)), BodyListWidget, SLOT(setEnabled(bool)));
    connect(PertBodyRadioButton, SIGNAL(toggled(bool)), PertBodyListWidget, SLOT(setEnabled(bool)));
    connect(PertBodyRadioButton, SIGNAL(toggled(bool)), AddPushButton, SLOT(setEnabled(bool)));
    connect(PertBodyRadioButton, SIGNAL(toggled(bool)), RemovePushButton, SLOT(setEnabled(bool)));
    connect(BodyListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(addPerturbingPlanet(QListWidgetItem*)));
    connect(AddPushButton, SIGNAL(pressed()), this, SLOT(addPerturbingPlanet()));
    connect(RemovePushButton, SIGNAL(pressed()), this, SLOT(removePerturbingPlanet()));
    connect(TesseralSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setTesserals(int)));
    connect(ZonalsSpinBox, SIGNAL(valueChanged(int)), TesseralSpinBox, SLOT(setVariableMaximum(int)));

    connect(PropagatorComboBox, SIGNAL(activated(int)), this, SLOT(disableIntegratorComboBox(int)));

}

LoiteringDialog::~LoiteringDialog()
{
    delete TesseralSpinBox;
}

void LoiteringDialog::addPerturbingPlanet()
{
    QList<QListWidgetItem *> planets = BodyListWidget->selectedItems();
    for (int i = 0; i < planets.size(); i++)
    {
        addPerturbingPlanet(planets.at(i));
    }
}
void LoiteringDialog::addPerturbingPlanet(QListWidgetItem* item)
{
    QString text = item->text();
    if(PertBodyListWidget->findItems(text,Qt::MatchExactly).isEmpty())
        PertBodyListWidget->addItem(text);
}
void LoiteringDialog::removePerturbingPlanet()
{
    QList<QListWidgetItem *> perturbplanets = PertBodyListWidget->selectedItems();
    for (int i = 0; i < perturbplanets.size(); i++)
    {
        delete perturbplanets.at(i);
    }
}
void LoiteringDialog::setTesserals(int i)
{
    m_tesserals = i;
}
void LoiteringDialog::disableIntegratorComboBox(int i)
{
    if (i == 0)
        IntegratorComboBox->setEnabled(false);
    else IntegratorComboBox->setEnabled(true);
}
bool LoiteringDialog::loadValues(ScenarioLoiteringType* loitering)
{
    ScenarioElementIdentifierType* arcIdentifier        = loitering->ElementIdentifier().data();
    ScenarioEnvironmentType* environment                = loitering->Environment().data();
    ScenarioTimeLine* parameters                        = loitering->TimeLine().data();
    ScenarioPropagationPositionType* propagation        = loitering->PropagationPosition().data();
    ScenarioInitialPositionType* initPosition           = loitering->InitialPosition().data();  //Modified by Dominic to reflect chages in XML schema (initial position now in sharedelements)

    ScenarioPropagationAttitudeType* attitudePropagation= loitering->PropagationAttitude().data();  //Added by Catarina
    ScenarioInitialAttitudeType* initAttitude           = loitering->InitialAttitude().data();  //Added by Catarina

    if (loadValues(arcIdentifier) && loadValues(environment) && loadValues(parameters) && loadValues(propagation) && loadValues(initPosition) && loadValues(attitudePropagation) && loadValues(initAttitude))
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool LoiteringDialog::loadValues(ScenarioElementIdentifierType* arcIdentifier)
{
    QString theArcName = arcIdentifier->Name();
    loiteringAspect.loadValueArcName(theArcName);

    QString theArcColor = arcIdentifier->colorName();
    loiteringAspect.loadValueArcColor(theArcColor);

    QString theArcModel = arcIdentifier->modelName();
    loiteringAspect.loadValueArcModel(theArcModel);

    return true;
}
bool LoiteringDialog::loadValues(ScenarioEnvironmentType* environment)
{
    // The cntral body
    QSharedPointer<ScenarioCentralBodyType> centralBody = environment->CentralBody();
    if (!centralBody.isNull())
    {
        QString centralBodyName = centralBody->Name().trimmed();
        const StaBody* body = STA_SOLAR_SYSTEM->lookup(centralBodyName);
        if (!body)
        {
            qDebug() << "Bad central body '" << centralBodyName << "' in loitering trajectory.";
            return false;
        }
        loiteringAspect.loadValueCentralBody(centralBodyName);
    }
    else
    {
        return false;
    }

    // The perturbations
    QSharedPointer<ScenarioPerturbationsType> perturbationsToCentralBody = environment->PerturbationsToCentralBody();
    DebrisRadioButton->setChecked(false);
    GravityFieldRadioButton->setChecked(false);
    SolarPressureRadioButton->setChecked(false);
    AtmDragRadioButton->setChecked(false);
    PertBodyRadioButton->setChecked(false);

    if (!perturbationsToCentralBody.isNull())
    {
        //qDebug() << "Not null" << endl;
        bool theAlbedo = perturbationsToCentralBody->albedo();
        bool theAtmosphericDrag = perturbationsToCentralBody->atmosphereDrag();
        bool theSolarPressure = perturbationsToCentralBody->solarPressure();
        bool theGravityEffects = perturbationsToCentralBody->gravityEffets();
        bool theDebris = perturbationsToCentralBody->micrometeoroids();
        bool the3rdBody =perturbationsToCentralBody->thirdBody();
        bool theIR =perturbationsToCentralBody->IR();

        if (theDebris)
            DebrisRadioButton->setChecked(true);
        else
            DebrisRadioButton->setChecked(false);

        if (theGravityEffects)
        {
            GravityFieldRadioButton->setChecked(true);
            if(!centralBody->GravityModel()->modelName().isEmpty())
            {
                QString gravitymodel = centralBody->GravityModel()->modelName();
                gravitymodel.remove(".stad");
                for (int i = 0; i < GravityComboBox->count(); i++)
                {
                    if (GravityComboBox->itemData(i) == gravitymodel)
                    {
                        GravityComboBox->setCurrentIndex(i);
                        break;
                    }
                }
            }
            ZonalsSpinBox->setValue(centralBody->GravityModel()->numberOfZonals());
            m_tesserals = centralBody->GravityModel()->numberOfTesserals();
            TesseralSpinBox->setValue(m_tesserals);
        } // ---- Gravity
        else
            GravityFieldRadioButton->setChecked(false);


        if (theSolarPressure)
            SolarPressureRadioButton->setChecked(true);
        else
            SolarPressureRadioButton->setChecked(false);

        if (theAtmosphericDrag)
        {
            AtmDragRadioButton->setChecked(true);
            if(!centralBody->AtmosphereModel().isEmpty())
            {
                QString atmospheremodel = centralBody->AtmosphereModel();
                atmospheremodel.remove(".stad");
                for (int i = 0; i < AtmosphereComboBox->count(); i++)
                {
                    if (AtmosphereComboBox->itemData(i) == atmospheremodel)
                    {
                        AtmosphereComboBox->setCurrentIndex(i);
                        break;
                    }
                }
            }
        } // ---- Atmopsheric drag
        else
            AtmDragRadioButton->setChecked(false);


        if (theAlbedo)
            AlbedoCheckBox->setChecked(true);
        else
            AlbedoCheckBox->setChecked(false);

        if (theIR)
            IRCheckBox->setChecked(true);
        else
            IRCheckBox->setChecked(false);

        if (the3rdBody)
        {
            QList<QString> perturbingBodyList = environment->PerturbationsToCentralBody()->perturbingBody();
            QString nameBody;
            for (int j=0; j<perturbingBodyList.size(); j++)
            {
                nameBody = perturbingBodyList.at(j);
                QList<QListWidgetItem*> items = BodyListWidget->findItems(nameBody, Qt::MatchFixedString);
                //if(!items.isEmpty() && nameBody != environment->CentralBody()->Name())
                if(nameBody != environment->CentralBody()->Name())
                    PertBodyListWidget->addItem(nameBody);
            }
            PertBodyRadioButton->setChecked(true);
        } // --- the 3rd Bodies
        else
            PertBodyRadioButton->setChecked(true);
    }
    // Finally returning
    return true;
}
bool LoiteringDialog::loadValues(ScenarioTimeLine* timeLine)
{
    dateTimeEdit->setDateTime(timeLine->StartTime());
    dateTimeEdit_2->setDateTime(timeLine->EndTime());
    TimeStepLineEdit->setText(QString::number(timeLine->StepTime()));

    return true;
}
bool LoiteringDialog::loadValues(ScenarioPropagationPositionType* propagation)
{
    QString currentPropagator = propagation->propagator().trimmed();
    for (int i = 0; i < PropagatorComboBox->count(); i++)
    {
        if (PropagatorComboBox->itemData(i) == currentPropagator)
        {
            PropagatorComboBox->setCurrentIndex(i);
            break;
        }
    }
    QString currentIntegrator = propagation->integrator().trimmed();
    for (int i = 0; i < IntegratorComboBox->count(); i++)
    {
        if (IntegratorComboBox->itemData(i) == currentIntegrator)
        {
            IntegratorComboBox->setCurrentIndex(i);
            break;
        }
    }

    IntStepEdit->setText(QString::number(propagation->timeStep()));
    return true;
}
bool LoiteringDialog::loadValues(ScenarioInitialPositionType* initPosition)
{
    QString coordSysName = initPosition->CoordinateSystem().trimmed();
    sta::CoordinateSystem coordSys(coordSysName);
    if (!coordSys.valid())
    {
        qDebug() << "Bad coordinate system '" << coordSysName << "' in scenario ";
        coordSys = sta::CoordinateSystem(sta::COORDSYS_EME_J2000);
    }

    // Set the coordinate system combo box value
    for (int i = 0; i < CoordSystemComboBox->count(); i++)
    {
        if (CoordSystemComboBox->itemData(i) == coordSys.type())
        {
            CoordSystemComboBox->setCurrentIndex(i);
            break;
        }
    }

    ScenarioAbstract6DOFPositionType* position = initPosition->Abstract6DOFPosition().data();

    ScenarioKeplerianElementsType* elements = dynamic_cast<ScenarioKeplerianElementsType*>(position);
    ScenarioStateVectorType* stateVector    = dynamic_cast<ScenarioStateVectorType*>(position);

    Q_ASSERT(elements || stateVector);

    if (elements)
    {
        InitialStateComboBox->setCurrentIndex(0);
        InitialStateStackedWidget->setCurrentWidget(keplerianPage);

        semimajorAxisEdit->setText(QString::number(elements->semiMajorAxis()));
        eccentricityEdit->setText(QString::number(elements->eccentricity()));
        inclinationEdit->setText(QString::number(elements->inclination()));
        raanEdit->setText(QString::number(elements->RAAN())); //*180/Pi()));
        argOfPeriapsisEdit->setText(QString::number(elements->argumentOfPeriapsis())); //*180/Pi()));
        trueAnomalyEdit->setText(QString::number(elements->trueAnomaly())); //*180/Pi()));
    }
    else if (stateVector)
    {
        InitialStateComboBox->setCurrentIndex(1);
        InitialStateStackedWidget->setCurrentWidget(stateVectorPage);

        positionXEdit->setText(QString::number(stateVector->x()));
        positionYEdit->setText(QString::number(stateVector->y()));
        positionZEdit->setText(QString::number(stateVector->z()));
        velocityXEdit->setText(QString::number(stateVector->vx()));
        velocityYEdit->setText(QString::number(stateVector->vy()));
        velocityZEdit->setText(QString::number(stateVector->vz()));
    }
    else
    {
        // Unknown initial state type
        return false;
    }

    return true;

}
//---------------------------------- Propagation Attitude ----------------------------------
bool LoiteringDialog::loadValues(ScenarioPropagationAttitudeType *propagation)
{
    QString currentIntegrator = propagation->integrator().trimmed();
    for (int i = 0; i < IntegratorComboBox->count(); i++)
    {
        if (IntegratorComboBox_3->itemData(i) == currentIntegrator)
        {
            IntegratorComboBox_3->setCurrentIndex(i);
            break;
        }
    }

    IntStepEdit->setText(QString::number(propagation->timeStep()));
    return true;
}
//-----------------------------------------------------------------------------------------
bool LoiteringDialog::loadValues(ScenarioInitialAttitudeType* initAttitude)
{
    QString attitudeCoordSysName = initAttitude->CoordinateSystem().trimmed();
    sta::CoordinateSystem coordSys(attitudeCoordSysName);
    //    if (!coordSys.valid())
    //    {
    //        qDebug() << "Bad coordinate system '" << attitudeCoordSysName << "' in scenario ";
    //        attitudeCoordSysName = sta::CoordinateSystem(sta::COORDSYS_LOCAL_VERTICAL);
    //    }

    // Set the coordinate system combo box value
    for (int i = 0; i < CoordSystemComboBox->count(); i++)
    {
        if (CoordSystemComboBox->itemData(i) == coordSys.type())
        {
            CoordSystemComboBox->setCurrentIndex(i);
            break;
        }
    }

    //IH: What is being done here? Assessing the type of 'attitude'?
    ScenarioAbstract6DOFAttitudeType* attitude = initAttitude->Abstract6DOFAttitude().data();

    //IH: Typecast the type of the attitude depending on what the combobox options are?
    ScenarioEulerBIType*    Euler321     = dynamic_cast<ScenarioEulerBIType*>(attitude);
    ScenarioEulerBIType* Euler123  = dynamic_cast<ScenarioEulerBIType*>(attitude);
    ScenarioEulerBIType* Euler313 = dynamic_cast<ScenarioEulerBIType*>(attitude);
    ScenarioqBIType*        qBI         = dynamic_cast<ScenarioqBIType*>(attitude);
    //ScenarioqBLVLHType*     qBLVLH      = dynamic_cast<ScenarioqBLVLHType*>(attitude);

    Q_ASSERT(Euler321 || Euler123 || Euler313 || qBI );

    if (Euler321)
    {
        AttitudeTypeComboBox->setCurrentIndex(0);
        stackedWidget_2->setCurrentWidget(Euler123Page);
        //InitialStateStackedWidget->setCurrentWidget(Euler123Page);

        Euler321phi->setText(QString::number(Euler321->phi()));
        Euler321theta->setText(QString::number(Euler321->theta()));
        Euler321psi->setText(QString::number(Euler321->psi()));
        Euler321omegaX->setText(QString::number(Euler321->phiDot()));      //Later, change to p
        Euler321omegaY->setText(QString::number(Euler321->thetaDot()));    //Later, change to q
        Euler321omegaZ->setText(QString::number(Euler321->psiDot()));      //Later, change to r

    }
    else if (Euler123)
    {
        AttitudeTypeComboBox->setCurrentIndex(1);
        stackedWidget_2->setCurrentWidget(Euler321Page);
        //InitialStateStackedWidget->setCurrentWidget(Euler321Page);

        Euler123phi->setText(QString::number(Euler123->phi()));
        Euler123theta->setText(QString::number(Euler123->theta()));
        Euler123psi->setText(QString::number(Euler123->psi()));
        Euler123omegaX->setText(QString::number(Euler123->phiDot()));      //Later, change to p
        Euler123omegaY->setText(QString::number(Euler123->thetaDot()));    //Later, change to q
        Euler123omegaZ->setText(QString::number(Euler123->psiDot()));      //Later, change to r
    }
    else if (Euler313)
    {
        AttitudeTypeComboBox->setCurrentIndex(2);
        stackedWidget_2->setCurrentWidget(Euler313Page);
        //InitialStateStackedWidget->setCurrentWidget(Euler313Page);

        Euler313phi->setText(QString::number(Euler313->phi()));
        Euler313theta->setText(QString::number(Euler313->theta()));
        Euler313psi->setText(QString::number(Euler313->psi()));
        Euler313omegaX->setText(QString::number(Euler313->phiDot()));      //Later, change to p
        Euler313omegaY->setText(QString::number(Euler313->thetaDot()));    //Later, change to q
        Euler313omegaZ->setText(QString::number(Euler313->psiDot()));      //Later, change to r
    }
    else if (qBI)
    {
        AttitudeTypeComboBox->setCurrentIndex(3);
        stackedWidget_2->setCurrentWidget(QuaternionJPLPage);

        q1->setText(QString::number(qBI->q1()));
        q2->setText(QString::number(qBI->q2()));
        q3->setText(QString::number(qBI->q3()));
        q4->setText(QString::number(qBI->q4()));
        q1dot->setText(QString::number(qBI->q1Dot()));
        q2dot->setText(QString::number(qBI->q2Dot()));
        q3dot->setText(QString::number(qBI->q3Dot()));
        q4dot->setText(QString::number(qBI->q4Dot()));
    }
    else
    {
        // Unknown initial attitude type
        return false;
    }

    return true;

}

bool LoiteringDialog::saveValues(ScenarioLoiteringType* loitering)
{
    ScenarioEnvironmentType* environment	     = loitering->Environment().data();
    ScenarioTimeLine* parameters		         = loitering->TimeLine().data();
    ScenarioPropagationPositionType* propagation = loitering->PropagationPosition().data();
    ScenarioElementIdentifierType* identifier    = loitering->ElementIdentifier().data();
    ScenarioInitialPositionType* initPosition    = loitering->InitialPosition().data(); //Modified by Dominic to reflect chages in XML schema (initial position now in sharedelements)

    ScenarioPropagationAttitudeType* attitudePropagation = loitering->PropagationAttitude().data(); //Added by Catarina
    ScenarioInitialAttitudeType* initAttitude    = loitering->InitialAttitude().data(); //Added by Catarina

    if (saveValues(identifier) && saveValues(environment) && saveValues(parameters) && saveValues(propagation) && saveValues(initPosition) && saveValues(attitudePropagation) && saveValues(initAttitude))
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool LoiteringDialog::saveValues(ScenarioElementIdentifierType* arcIdentifier)
{
    // The arc name
    QString theArcName = loiteringAspect.saveValueArcName();
    arcIdentifier->setName(theArcName);

    // The color
    QString theColorName = loiteringAspect.saveValueArcColor();
    arcIdentifier->setColorName(theColorName);

    // The model
    QString theModelName = loiteringAspect.saveValueArcModel();
    arcIdentifier->setModelName(theModelName);

    return true;
}
bool LoiteringDialog::saveValues(ScenarioEnvironmentType* environment)
{
    // The central body
    QString myCentralBody = loiteringAspect.saveValueCentralBody();
    StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(myCentralBody);
    if (centralBody)
    {
        environment->CentralBody()->setName(myCentralBody);
    }
    else
    {
        qWarning("Unknown central body");
        return false;
    }

    // The perturbations
    QSharedPointer<ScenarioPerturbationsType> perturbationsToCentralBody = environment->PerturbationsToCentralBody();

    if (perturbationsToCentralBody.isNull())
    {
        ScenarioPerturbationsType* perturbationsToCentralBody = new ScenarioPerturbationsType();
        environment->setPerturbationsToCentralBody(QSharedPointer<ScenarioPerturbationsType>(perturbationsToCentralBody));
    }


    if (DebrisRadioButton->isChecked())
        environment->PerturbationsToCentralBody()->setMicrometeoroids(true);
    else
        environment->PerturbationsToCentralBody()->setMicrometeoroids(false);

    if (GravityFieldRadioButton->isChecked())
    {
        environment->PerturbationsToCentralBody()->setGravityEffets(true);
        QString gravitymodel = GravityComboBox->itemData(GravityComboBox->currentIndex()).toString();
        gravitymodel.append(".stad");
        environment->CentralBody()->GravityModel()->setModelName(gravitymodel);
        environment->CentralBody()->GravityModel()->setNumberOfZonals(ZonalsSpinBox->value());
        environment->CentralBody()->GravityModel()->setNumberOfTesserals(m_tesserals);
    }
    else
        environment->PerturbationsToCentralBody()->setGravityEffets(false);

    if (AtmDragRadioButton->isChecked())
    {
        environment->PerturbationsToCentralBody()->setAtmosphereDrag(true);
        QString atmospheremodel = AtmosphereComboBox->itemData(AtmosphereComboBox->currentIndex()).toString();
        atmospheremodel.append(".stad");
        environment->CentralBody()->setAtmosphereModel(atmospheremodel);
    }
    else
        environment->PerturbationsToCentralBody()->setAtmosphereDrag(false);

    if (SolarPressureRadioButton->isChecked())
    {
        environment->PerturbationsToCentralBody()->setSolarPressure(true);
        if (AlbedoCheckBox->isChecked())
            environment->PerturbationsToCentralBody()->setAlbedo(true);
        else
        {
            environment->PerturbationsToCentralBody()->setAlbedo(false);
            AlbedoCheckBox->setChecked(false);
        }
        if (IRCheckBox->isChecked())
            environment->PerturbationsToCentralBody()->setIR(true);
        else
        {
            environment->PerturbationsToCentralBody()->setIR(false);
            IRCheckBox->setChecked(false);
        }
    }
    else
    {
        AlbedoCheckBox->setChecked(false);
        IRCheckBox->setChecked(false);
        environment->PerturbationsToCentralBody()->setSolarPressure(false);
        environment->PerturbationsToCentralBody()->setIR(false);
        environment->PerturbationsToCentralBody()->setAlbedo(false);
    }

    if (PertBodyRadioButton->isChecked())
    {
        environment->PerturbationsToCentralBody()->setThirdBody(true);
        QList<QString> myPerturbingBodyList = environment->PerturbationsToCentralBody()->perturbingBody();
        myPerturbingBodyList.clear(); // Reset the perturbing bodies list
        for (int j = 0; j < PertBodyListWidget->count(); j++)
        {
            StaBody* body = STA_SOLAR_SYSTEM->lookup(PertBodyListWidget->item(j)->text());
            if (body && body != centralBody)
                myPerturbingBodyList.append(body->name());
        }
        qDebug() << myPerturbingBodyList << endl;
        environment->PerturbationsToCentralBody()->setPerturbingBody(myPerturbingBodyList);
        qDebug() << environment->PerturbationsToCentralBody()->perturbingBody() << endl;
    }
    else
        environment->PerturbationsToCentralBody()->setThirdBody(false);

    environment->PerturbationsToCentralBody()->setUserDefined(false); // nothing for the moment

    // Finally returning
    return true;
}
bool LoiteringDialog::saveValues(ScenarioTimeLine* timeline)
{
    timeline->setStartTime(dateTimeEdit->dateTime());
    timeline->setEndTime(dateTimeEdit_2->dateTime());
    timeline->setStepTime(TimeStepLineEdit->text().toDouble());

    return true;
}
bool LoiteringDialog::saveValues(ScenarioInitialPositionType* initPos)
{
    sta::CoordinateSystemType coordSysType = sta::CoordinateSystemType(CoordSystemComboBox->itemData(CoordSystemComboBox->currentIndex()).toInt());
    initPos->setCoordinateSystem(sta::CoordinateSystem(coordSysType).name());

    switch (InitialStateComboBox->currentIndex())
    {
    case 0: // --- KEPLERIAN
        {
            // Guillermo April 23 2010
            ScenarioKeplerianElementsType* elements_deg = new ScenarioKeplerianElementsType;
            elements_deg->setSemiMajorAxis(semimajorAxisEdit->text().toDouble());
            elements_deg->setEccentricity(eccentricityEdit->text().toDouble());
            elements_deg->setInclination(inclinationEdit->text().toDouble());
            elements_deg->setRAAN(raanEdit->text().toDouble());
            elements_deg->setArgumentOfPeriapsis(argOfPeriapsisEdit->text().toDouble());
            elements_deg->setTrueAnomaly(trueAnomalyEdit->text().toDouble());

            initPos->setAbstract6DOFPosition(QSharedPointer<ScenarioAbstract6DOFPositionType>(elements_deg));
        }
        return true;
    case 1:  // --- STATE VECTOR
        {
            ScenarioStateVectorType* stateVector = new ScenarioStateVectorType();
            stateVector->setX(positionXEdit->text().toDouble());
            stateVector->setY(positionYEdit->text().toDouble());
            stateVector->setZ(positionZEdit->text().toDouble());
            stateVector->setVx(velocityXEdit->text().toDouble());
            stateVector->setVy(velocityYEdit->text().toDouble());
            stateVector->setVz(velocityZEdit->text().toDouble());

            initPos->setAbstract6DOFPosition(QSharedPointer<ScenarioAbstract6DOFPositionType>(stateVector));
        }
        return true;

    default:
        return false;
    }
}
bool LoiteringDialog::saveValues(ScenarioInitialAttitudeType* initAtt)
{
    sta::CoordinateSystemType coordSysType = sta::CoordinateSystemType(CoordSystemAttitudeComboBox->itemData(CoordSystemAttitudeComboBox->currentIndex()).toInt());
    initAtt->setCoordinateSystem(sta::CoordinateSystem(coordSysType).name());

    switch (AttitudeTypeComboBox->currentIndex())
    {
    case 0:  // --- Euler 321
        {
            ScenarioEulerBIType* euler = new ScenarioEulerBIType;
            euler->setPhi(Euler321phi->text().toDouble());
            euler->setTheta(Euler321theta->text().toDouble());
            euler->setPsi(Euler321psi->text().toDouble());
            euler->setPhiDot(Euler321omegaX->text().toDouble());
            euler->setThetaDot(Euler321omegaY->text().toDouble());
            euler->setPsiDot(Euler321omegaZ->text().toDouble());

            initAtt->setAbstract6DOFAttitude(QSharedPointer<ScenarioAbstract6DOFAttitudeType>(euler));
        }
    case 1:  // --- Euler 123
        {
            ScenarioEulerBIType* euler = new ScenarioEulerBIType;
            euler->setPhi(Euler123phi->text().toDouble());
            euler->setTheta(Euler123theta->text().toDouble());
            euler->setPsi(Euler123psi->text().toDouble());
            euler->setPhiDot(Euler123omegaX->text().toDouble());
            euler->setThetaDot(Euler123omegaY->text().toDouble());
            euler->setPsiDot(Euler123omegaZ->text().toDouble());

            initAtt->setAbstract6DOFAttitude(QSharedPointer<ScenarioAbstract6DOFAttitudeType>(euler));
        }
    case 2:  // --- Euler 313
        {
            ScenarioEulerBIType* euler = new ScenarioEulerBIType;
            euler->setPhi(Euler313phi->text().toDouble());
            euler->setTheta(Euler313theta->text().toDouble());
            euler->setPsi(Euler313psi->text().toDouble());
            euler->setPhiDot(Euler313omegaX->text().toDouble());
            euler->setThetaDot(Euler313omegaY->text().toDouble());
            euler->setPsiDot(Euler313omegaZ->text().toDouble());

            initAtt->setAbstract6DOFAttitude(QSharedPointer<ScenarioAbstract6DOFAttitudeType>(euler));
        }
    case 3:  // --- Quaternions
        {
            ScenarioqBIType* quat = new ScenarioqBIType;

            quat->setQ1(q1->text().toDouble());
            quat->setQ2(q2->text().toDouble());
            quat->setQ3(q3->text().toDouble());
            quat->setQ4(q4->text().toDouble());
            quat->setQ1Dot(q1dot->text().toDouble());
            quat->setQ2Dot(q2dot->text().toDouble());
            quat->setQ3Dot(q3dot->text().toDouble());
            quat->setQ4Dot(q4dot->text().toDouble());

            initAtt->setAbstract6DOFAttitude(QSharedPointer<ScenarioAbstract6DOFAttitudeType>(quat));
        }
        return true;
    default:
        return false;
    }
}
bool LoiteringDialog::saveValues(ScenarioPropagationPositionType* propagation)
{
    propagation->setIntegrator(IntegratorComboBox->itemData(IntegratorComboBox->currentIndex()).toString());
    //    propagation->setPropagator(PropagatorComboBox->itemData(PropagatorComboBox->currentIndex()).toString());
    propagation->setTimeStep(IntStepEdit->text().toDouble());

    return true;
}
//------------------------------------------- ATTITUDE PROPAGATION ----------------------------------------
bool LoiteringDialog::saveValues(ScenarioPropagationAttitudeType* propagation)
{
    propagation->setIntegrator(IntegratorComboBox_3->itemData(IntegratorComboBox_3->currentIndex()).toString());
    //    propagation->setPropagator(PropagatorComboBox->itemData(PropagatorComboBox->currentIndex()).toString());
    propagation->setTimeStep(IntStepEdit->text().toDouble());

    return true;
}
//---------------------------------------------------------------------------------------------------------

TesseralBox::TesseralBox(QDialog* parent) : QSpinBox(parent)
{
}
void TesseralBox::setVariableMaximum(int i)
{
    this->setMaximum(i);
}
/////////////////////////////////////// PropagateLoiteringTrajectory /////////////////////////////
//bool PropagateLoiteringTrajectory(ScenarioLoiteringType* loitering,
//                                     QList<double>& sampleTimes,
//                                     QList<sta::StateVector>& samples,
//                                     QList<sta::AttitudeVector>& samplesAtt,
//                                     QString attitudeRotationSequence,
//                                     PropagationFeedback& propFeedback)
//{
//    //Check the coordinate system of position
//    QString loiteringLabel = loitering->ElementIdentifier()->Name();

//    QString centralBodyName = loitering->Environment()->CentralBody()->Name();
//    StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
//    if (!centralBody)
//    {
//        propFeedback.raiseError(QObject::tr("Unrecognized central body '%1'").arg(centralBodyName));
//        return false;
//    }

//    QString coordSysName = loitering->InitialPosition()->CoordinateSystem();
//    sta::CoordinateSystem coordSys(coordSysName);
//    if (coordSys.type() == sta::COORDSYS_INVALID)
//    {
//        propFeedback.raiseError(QObject::tr("Unrecognized coordinate system '%1'").arg(coordSysName));
//        return false;
//    }

//    //Check the coordinate system of attitude
//    QString attitudeCoordSysName = loitering->InitialAttitude()->CoordinateSystem();
//    //sta::CoordinateSystem coordSys(attitudeCoordSysName);
//    if (coordSys.type() == sta::COORDSYS_INVALID)
//    {
//        propFeedback.raiseError(QObject::tr("Unrecognized coordinate system '%1'").arg(attitudeCoordSysName));
//        return false;
//    }
//    // Get the initial state in two forms:
//    //   - Keplerian elements for simple two-body propagation
//    //   - A state vector for any other sort of propagation
//    ScenarioAbstract6DOFPositionType* position = loitering->InitialPosition()->Abstract6DOFPosition().data();
//    sta::StateVector initialState = AbstractPositionToStateVector(position, centralBody);
//    sta::KeplerianElements initialStateKeplerian = AbstractPositionToKeplerianElements(position, centralBody);

//    double mu = centralBody->mu();

//    // Get the initial attitude
//    //Stores the information of the attitude, inserted in the GUI
//    ScenarioAbstract6DOFAttitudeType* attitude = loitering->InitialAttitude()->Abstract6DOFAttitude().data();
//    int seq1, seq2, seq3;

//    if (dynamic_cast<const ScenarioEulerBIType*>(attitude))
//    {
//        const ScenarioEulerBIType* euler = dynamic_cast<const ScenarioEulerBIType*>(attitude);

//        if(attitudeRotationSequence == "123")
//        {
//            seq1 = 1;
//            seq2 = 2;
//            seq3 = 3;
//        }
//        else if(attitudeRotationSequence == "321")
//        {
//            seq1 = 3;
//            seq2 = 2;
//            seq3 = 1;
//        }
//        else if(attitudeRotationSequence == "313")
//        {
//            seq1 = 3;
//            seq2 = 1;
//            seq3 = 3;
//        }
//    }
//    //Initial attitude: contains the quaternions and the body rates. Transforms the input to quaternions and quaternion rates
//    sta::AttitudeVector initialAttitude = AbstractAttitudeToQuaternions(attitude, seq1, seq2, seq3);
//    //Stores the initial attitude in quaternions and body rates!!
//    sta::AttitudeVector initialAttitudeQuaternions = AttitudeVector(initialAttitude);

//    // Get the timeline information
//    const ScenarioTimeLine* timeline = loitering->TimeLine().data();
//    double timelineDuration = timeline->StartTime().secsTo(timeline->EndTime());
//    double dt = loitering->PropagationPosition()->timeStep();

//    if (dt == 0.0)
//    {
//        propFeedback.raiseError(QObject::tr("Time step is zero!"));
//        return false;
//    }

//    // We don't output values at every integration step. Instead use the time step
//    // from simulation parameters. The actual output step used will not necessarily
//    // match the requested output step: the code below sets it to be an integer
//    // multiple of the integration step.
//    double requestedOutputTimeStep = timeline->StepTime();
//    double outputTimeStep;
//    unsigned int outputRate;
//    if (requestedOutputTimeStep < dt)
//    {
//        outputRate = 1;
//        outputTimeStep = dt;
//    }
//    else
//    {
//        outputRate = (unsigned int) floor(requestedOutputTimeStep / dt + 0.5);
//        outputTimeStep = outputRate * dt;
//    }

//    if (timelineDuration / outputTimeStep > MAX_OUTPUT_STEPS)
//    {
//        propFeedback.raiseError(QObject::tr("Number of propagation steps exceeds %1. Try increasing the simulation time step.").arg(MAX_OUTPUT_STEPS));
//        return false;
//    }

//        QList<Perturbations*> perturbationsList; // Create the list of perturbations that will influence the propagation

//    sta::StateVector stateVector = initialState;
//    sta::AttitudeVector attitudeVector = initialAttitudeQuaternions;

//    // deviation, reference, and q will be used only in Encke propagation
//    sta::StateVector deviation(Vector3d::Zero(), Vector3d::Zero());
//    sta::StateVector reference = initialState;
//    double q = 0.0;

//    double startTime = sta::JdToMjd(sta::CalendarToJd(timeline->StartTime()));

//    sampleTimes << startTime;
//    samples << stateVector;
//    samplesAtt << attitudeVector;

//    QFile ciccio("data/PerturbationsData.stae");
//    QTextStream cicciostream(&ciccio);
//    ciccio.open(QIODevice::WriteOnly);

//    //unsigned int steps = 0;
//    unsigned int steps = 1; // patched by Ana on 18th June 2010

//    QString propagator = loitering->PropagationPosition()->propagator();
//    QString integrator = loitering->PropagationPosition()->integrator();

//    MyMatrix3d inertiaMatrix;
//    ScenarioSC* mySatellite = new ScenarioSC();
//    inertiaMatrix(0,0) = mySatellite->System()->Structure()->MomentsOfInertia()->xAxis();
//    inertiaMatrix(1,1) = mySatellite->System()->Structure()->MomentsOfInertia()->yAxis();
//    inertiaMatrix(2,2) = mySatellite->System()->Structure()->MomentsOfInertia()->zAxis();
//    inertiaMatrix(0,1) = mySatellite->System()->Structure()->SecondMomentsOfArea()->xAxis();
//    inertiaMatrix(0,2) = mySatellite->System()->Structure()->SecondMomentsOfArea()->yAxis();
//    inertiaMatrix(1,2) = mySatellite->System()->Structure()->SecondMomentsOfArea()->zAxis();

//    if (propagator == "TWO BODY")
//    {

//        double sma            = initialStateKeplerian.SemimajorAxis;
//        double e              = initialStateKeplerian.Eccentricity;
//        double inclination    = initialStateKeplerian.Inclination*Pi()/180.0;
//        double raan           = initialStateKeplerian.AscendingNode*Pi()/180.0;
//        double argOfPeriapsis = initialStateKeplerian.ArgumentOfPeriapsis*Pi()/180.0;
//        double trueAnomaly    = initialStateKeplerian.TrueAnomaly*Pi()/180.0;
//        double meanAnomaly    = trueAnomalyTOmeanAnomaly(trueAnomaly, e);

//        // Next lines patched by Guillermo on April 23 2010 to speed up calculations outside the for loop
//        double argOfPeriapsisUpdated      = 0.0;
//        double meanAnomalyUpdated         = 0.0;
//        double raanUpdated                = 0.0;

//        double perigee = sma * (1 - e);
//        if (perigee < centralBody->meanRadius())
//        {
//            propFeedback.raiseError(QObject::tr("The perigee distance is smaller than the main body radius."));
//            return false;
//        }
//        else
//        {
//            for (double t = dt; t < timelineDuration + dt; t += dt)
//            {
//                JulianDate jd = startTime + sta::secsToDays(t);

//                stateVector = propagateTWObody(mu, sma, e, inclination, argOfPeriapsis, raan, meanAnomaly,
//                                               dt,
//                                               raanUpdated, argOfPeriapsisUpdated, meanAnomalyUpdated);

//                argOfPeriapsis = argOfPeriapsisUpdated;
//                meanAnomaly    = meanAnomalyUpdated;
//                raan           = raanUpdated;

//                // -- Integrating the attitude -- //

//                //Integrating the body rates
//                AttitudeIntegration propagBodyRates;
//                MyVector3d bodyRates = propagBodyRates.propagateEulerEquation(attitudeVector.bodyRates, startTime, dt, inertiaMatrix);

//                //Integrating the quaternions
//                AttitudeIntegration propAttitudeVector;
//                MyVector4d theQuaternions(attitudeVector.quaternion.coeffs().coeffRef(0),
//                                          attitudeVector.quaternion.coeffs().coeffRef(1),
//                                          attitudeVector.quaternion.coeffs().coeffRef(2),
//                                          attitudeVector.quaternion.coeffs().coeffRef(3));

//                MyVector4d integratedQuaternions = propAttitudeVector.propagateQUATERNIONS(theQuaternions, startTime, dt, bodyRates);

//                //Saving integrated values to the new attitude vector
//                attitudeVector.quaternion.coeffs().coeffRef(0) = integratedQuaternions(0);
//                attitudeVector.quaternion.coeffs().coeffRef(1) = integratedQuaternions(1);
//                attitudeVector.quaternion.coeffs().coeffRef(2) = integratedQuaternions(2);
//                attitudeVector.quaternion.coeffs().coeffRef(3) = integratedQuaternions(3);
//                attitudeVector.bodyRates(0) = bodyRates(0);
//                attitudeVector.bodyRates(1) = bodyRates(1);
//                attitudeVector.bodyRates(2) = bodyRates(2);

//                // -- End of: Integrating the attitude -- //

//                // Append a trajectory sample every outputRate integration steps (and always at the last step.)
//                if (steps % outputRate == 0 || t >= timelineDuration)
//                {
//                    sampleTimes << jd;
//                    samples << stateVector;
//                    samplesAtt << attitudeVector;
//                }
//                ++steps;
//            }
//        }
//    }
//    else if (propagator == "COWELL")
//    {
//        for (double t = dt; t < timelineDuration + dt; t += dt)
//        {
//            JulianDate jd = startTime + sta::secsToDays(t);
//            stateVector = propagateCOWELL(mu, stateVector, dt, perturbationsList, jd, integrator, propFeedback);

//            // -- Integrating the attitude -- //

//            //Integrating the body rates
//            AttitudeIntegration propagBodyRates;
//            MyVector3d bodyRates = propagBodyRates.propagateEulerEquation(attitudeVector.bodyRates, startTime, dt, inertiaMatrix);

//            //Integrating the quaternions
//            AttitudeIntegration propAttitudeVector;
//            MyVector4d theQuaternions(attitudeVector.quaternion.coeffs().coeffRef(0),
//                                      attitudeVector.quaternion.coeffs().coeffRef(1),
//                                      attitudeVector.quaternion.coeffs().coeffRef(2),
//                                      attitudeVector.quaternion.coeffs().coeffRef(3));

//            MyVector4d integratedQuaternions = propAttitudeVector.propagateQUATERNIONS(theQuaternions, startTime, dt, bodyRates);

//            //Saving integrated values to the new attitude vector
//            attitudeVector.quaternion.coeffs().coeffRef(0) = integratedQuaternions(0);
//            attitudeVector.quaternion.coeffs().coeffRef(1) = integratedQuaternions(1);
//            attitudeVector.quaternion.coeffs().coeffRef(2) = integratedQuaternions(2);
//            attitudeVector.quaternion.coeffs().coeffRef(3) = integratedQuaternions(3);
//            attitudeVector.bodyRates(0) = bodyRates(0);
//            attitudeVector.bodyRates(1) = bodyRates(1);
//            attitudeVector.bodyRates(2) = bodyRates(2);

//            // -- End of: Integrating the attitude -- //

//            // Append a trajectory sample every outputRate integration steps (and always at the last step.)
//            if (steps % outputRate == 0 || t >= timelineDuration)
//            {
//                sampleTimes << jd;
//                samples << stateVector;
//                samplesAtt << attitudeVector;
//            }
//            ++steps;
//        }
//    }
//    else if (propagator == "ENCKE")
//    {
//        double sma            = initialStateKeplerian.SemimajorAxis;
//        double e              = initialStateKeplerian.Eccentricity;
//        double inclination    = initialStateKeplerian.Inclination;
//        double raan           = initialStateKeplerian.AscendingNode;
//        double argOfPeriapsis = initialStateKeplerian.ArgumentOfPeriapsis;
//        double meanAnomaly    = initialStateKeplerian.MeanAnomaly;

//        for (double t = dt; t < timelineDuration + dt; t += dt)
//        {
//            JulianDate jd = startTime + sta::secsToDays(t);
//            deviation = propagateENCKE(mu, reference, dt, perturbationsList, jd, stateVector, deviation,  q, integrator, propFeedback);

//            // PropagateTWObody is used to propagate the reference trajectory
//            double argOfPeriapsisUpdated      = 0.0;
//            double meanAnomalyUpdated         = 0.0;
//            double raanUpdated                = 0.0;
//            reference = propagateTWObody(mu, sma, e, inclination, argOfPeriapsis, raan, meanAnomaly,
//                                         dt,
//                                         raanUpdated, argOfPeriapsisUpdated, meanAnomalyUpdated);

//            argOfPeriapsis = argOfPeriapsisUpdated;
//            meanAnomaly    = meanAnomalyUpdated;
//            raan           = raanUpdated;

//            // Calculating the perturbed trajectory
//            stateVector = reference + deviation;
//            q = deviation.position.dot(reference.position + 0.5 * deviation.position) / pow(reference.position.norm(), 2.0);

//#if 0
//            // Rectification of the reference trajectory, when the deviation is too large.
//            if (q > 0.01)
//            {
//                sta::KeplerianElements keplerian = cartesianTOorbital(mu, stateVector);

//                sma = keplerian.SemimajorAxis;
//                e = keplerian.Eccentricity;
//                inclination = keplerian.Inclination;
//                argOfPeriapsis = keplerian.ArgumentOfPeriapsis;
//                raan = keplerian.AscendingNode;
//                meanAnomaly = keplerian.MeanAnomaly;

//                q = 0;
//                reference = stateVector;
//                deviation = sta::StateVector(null, null);
//            }
//#endif

//            // -- Integrating the attitude -- //

//            //Integrating the body rates
//            AttitudeIntegration propagBodyRates;
//            MyVector3d bodyRates = propagBodyRates.propagateEulerEquation(attitudeVector.bodyRates, startTime, dt, inertiaMatrix);

//            //Integrating the quaternions
//            AttitudeIntegration propAttitudeVector;
//            MyVector4d theQuaternions(attitudeVector.quaternion.coeffs().coeffRef(0),
//                                      attitudeVector.quaternion.coeffs().coeffRef(1),
//                                      attitudeVector.quaternion.coeffs().coeffRef(2),
//                                      attitudeVector.quaternion.coeffs().coeffRef(3));

//            MyVector4d integratedQuaternions = propAttitudeVector.propagateQUATERNIONS(theQuaternions, startTime, dt, bodyRates);

//            //Saving integrated values to the new attitude vector
//            attitudeVector.quaternion.coeffs().coeffRef(0) = integratedQuaternions(0);
//            attitudeVector.quaternion.coeffs().coeffRef(1) = integratedQuaternions(1);
//            attitudeVector.quaternion.coeffs().coeffRef(2) = integratedQuaternions(2);
//            attitudeVector.quaternion.coeffs().coeffRef(3) = integratedQuaternions(3);
//            attitudeVector.bodyRates(0) = bodyRates(0);
//            attitudeVector.bodyRates(1) = bodyRates(1);
//            attitudeVector.bodyRates(2) = bodyRates(2);

//            // -- End of: Integrating the attitude -- //

//            // Append a trajectory sample every outputRate integration steps (and always at the last step.)
//            if (steps % outputRate == 0 || t >= timelineDuration)
//            {
//                sampleTimes << jd;
//                samples << stateVector;
//                samplesAtt << attitudeVector;
//            }
//            ++steps;
//        }
//    }
//    else if (propagator == "GAUSS")
//    {
//        for (double t = dt; t < timelineDuration + dt; t += dt)
//        {
//            JulianDate jd = startTime + sta::secsToDays(t);
//                        // Append a trajectory sample every outputRate integration steps (and always at the last step.)
//            if (steps % outputRate == 0 || t >= timelineDuration)
//            {
//                sampleTimes << jd;
//                samples << stateVector;
//                samplesAtt << attitudeVector;
//            }
//            ++steps;

//            stateVector = propagateGAUSS(mu, stateVector, dt, perturbationsList, jd, integrator);

//            // -- Integrating the attitude -- //

//            //Integrating the body rates
//            AttitudeIntegration propagBodyRates;
//            MyVector3d bodyRates = propagBodyRates.propagateEulerEquation(attitudeVector.bodyRates, startTime, dt, inertiaMatrix);

//            //Integrating the quaternions
//            AttitudeIntegration propAttitudeVector;
//            MyVector4d theQuaternions(attitudeVector.quaternion.coeffs().coeffRef(0),
//                                      attitudeVector.quaternion.coeffs().coeffRef(1),
//                                      attitudeVector.quaternion.coeffs().coeffRef(2),
//                                      attitudeVector.quaternion.coeffs().coeffRef(3));

//            MyVector4d integratedQuaternions = propAttitudeVector.propagateQUATERNIONS(theQuaternions, startTime, dt, bodyRates);

//            //Saving integrated values to the new attitude vector
//            attitudeVector.quaternion.coeffs().coeffRef(0) = integratedQuaternions(0);
//            attitudeVector.quaternion.coeffs().coeffRef(1) = integratedQuaternions(1);
//            attitudeVector.quaternion.coeffs().coeffRef(2) = integratedQuaternions(2);
//            attitudeVector.quaternion.coeffs().coeffRef(3) = integratedQuaternions(3);
//            attitudeVector.bodyRates(0) = bodyRates(0);
//            attitudeVector.bodyRates(1) = bodyRates(1);
//            attitudeVector.bodyRates(2) = bodyRates(2);

//            // -- End of: Integrating the attitude -- //
//        }
//    }
//    else
//    {
//        propFeedback.raiseError(QObject::tr("Unsupported propagator '%1'").arg(propagator));
//        return false;
//    }

//    return true;

//}
/////////////////////////////////////// PropagateLoiteringTrajectory /////////////////////////////
bool PropagateLoiteringTrajectory(ScenarioLoiteringType* loitering,
                                  QList<double>& sampleTimes,
                                  QList<sta::StateVector>& samples,
                                  PropagationFeedback& propFeedback)
{
    //Check the coordinate system of position
    QString loiteringLabel = loitering->ElementIdentifier()->Name();

    QString centralBodyName = loitering->Environment()->CentralBody()->Name();
    StaBody* centralBody = STA_SOLAR_SYSTEM->lookup(centralBodyName);
    if (!centralBody)
    {
        propFeedback.raiseError(QObject::tr("Unrecognized central body '%1'").arg(centralBodyName));
        return false;
    }

    QString coordSysName = loitering->InitialPosition()->CoordinateSystem();
    sta::CoordinateSystem coordSys(coordSysName);
    if (coordSys.type() == sta::COORDSYS_INVALID)
    {
        propFeedback.raiseError(QObject::tr("Unrecognized coordinate system '%1'").arg(coordSysName));
        return false;
    }

    // Get the initial state in two forms:
    //   - Keplerian elements for simple two-body propagation
    //   - A state vector for any other sort of propagation
    ScenarioAbstract6DOFPositionType* position = loitering->InitialPosition()->Abstract6DOFPosition().data();
    sta::StateVector initialState = AbstractPositionToStateVector(position, centralBody);
    sta::KeplerianElements initialStateKeplerian = AbstractPositionToKeplerianElements(position, centralBody);

    double mu = centralBody->mu();

    // Get the timeline information
    const ScenarioTimeLine* timeline = loitering->TimeLine().data();
    double timelineDuration = timeline->StartTime().secsTo(timeline->EndTime());
    double dt = loitering->PropagationPosition()->timeStep();

    if (dt == 0.0)
    {
        propFeedback.raiseError(QObject::tr("Time step is zero!"));
        return false;
    }

    // We don't output values at every integration step. Instead use the time step
    // from simulation parameters. The actual output step used will not necessarily
    // match the requested output step: the code below sets it to be an integer
    // multiple of the integration step.
    double requestedOutputTimeStep = timeline->StepTime();
    double outputTimeStep;
    unsigned int outputRate;
    if (requestedOutputTimeStep < dt)
    {
        outputRate = 1;
        outputTimeStep = dt;
    }
    else
    {
        outputRate = (unsigned int) floor(requestedOutputTimeStep / dt + 0.5);
        outputTimeStep = outputRate * dt;
    }

    if (timelineDuration / outputTimeStep > MAX_OUTPUT_STEPS)
    {
        propFeedback.raiseError(QObject::tr("Number of propagation steps exceeds %1. Try increasing the simulation time step.").arg(MAX_OUTPUT_STEPS));
        return false;
    }

    QList<Perturbations*> perturbationsList; // Create the list of perturbations that will influence the propagation

    sta::StateVector stateVector = initialState;

    // deviation, reference, and q will be used only in Encke propagation
    sta::StateVector deviation(Vector3d::Zero(), Vector3d::Zero());
    sta::StateVector reference = initialState;
    double q = 0.0;

    double startTime = sta::JdToMjd(sta::CalendarToJd(timeline->StartTime()));

    sampleTimes << startTime;
    samples << stateVector;

    QFile ciccio("data/PerturbationsData.stae");
    QTextStream cicciostream(&ciccio);
    ciccio.open(QIODevice::WriteOnly);

    //unsigned int steps = 0;
    unsigned int steps = 1; // patched by Ana on 18th June 2010

    QString propagator = loitering->PropagationPosition()->propagator();
    QString integrator = loitering->PropagationPosition()->integrator();

    if (propagator == "TWO BODY")
    {

        double sma            = initialStateKeplerian.SemimajorAxis;
        double e              = initialStateKeplerian.Eccentricity;
        double inclination    = initialStateKeplerian.Inclination*Pi()/180.0;
        double raan           = initialStateKeplerian.AscendingNode*Pi()/180.0;
        double argOfPeriapsis = initialStateKeplerian.ArgumentOfPeriapsis*Pi()/180.0;
        double trueAnomaly    = initialStateKeplerian.TrueAnomaly*Pi()/180.0;
        double meanAnomaly    = trueAnomalyTOmeanAnomaly(trueAnomaly, e);

        // Next lines patched by Guillermo on April 23 2010 to speed up calculations outside the for loop
        double argOfPeriapsisUpdated      = 0.0;
        double meanAnomalyUpdated         = 0.0;
        double raanUpdated                = 0.0;

        double perigee = sma * (1 - e);
        if (perigee < centralBody->meanRadius())
        {
            propFeedback.raiseError(QObject::tr("The perigee distance is smaller than the main body radius."));
            return false;
        }
        else
        {
            for (double t = dt; t < timelineDuration + dt; t += dt)
            {
                JulianDate jd = startTime + sta::secsToDays(t);

                stateVector = propagateTWObody(mu, sma, e, inclination, argOfPeriapsis, raan, meanAnomaly,
                                               dt,
                                               raanUpdated, argOfPeriapsisUpdated, meanAnomalyUpdated);

                argOfPeriapsis = argOfPeriapsisUpdated;
                meanAnomaly    = meanAnomalyUpdated;
                raan           = raanUpdated;

                // Append a trajectory sample every outputRate integration steps (and always at the last step.)
                if (steps % outputRate == 0 || t >= timelineDuration)
                {
                    sampleTimes << jd;
                    samples << stateVector;

                }
                ++steps;
            }
        }
    }
    else if (propagator == "COWELL")
    {
        for (double t = dt; t < timelineDuration + dt; t += dt)
        {
            JulianDate jd = startTime + sta::secsToDays(t);
            stateVector = propagateCOWELL(mu, stateVector, dt, perturbationsList, jd, integrator, propFeedback);

            // Append a trajectory sample every outputRate integration steps (and always at the last step.)
            if (steps % outputRate == 0 || t >= timelineDuration)
            {
                sampleTimes << jd;
                samples << stateVector;

            }
            ++steps;
        }
    }
    else if (propagator == "ENCKE")
    {
        double sma            = initialStateKeplerian.SemimajorAxis;
        double e              = initialStateKeplerian.Eccentricity;
        double inclination    = initialStateKeplerian.Inclination;
        double raan           = initialStateKeplerian.AscendingNode;
        double argOfPeriapsis = initialStateKeplerian.ArgumentOfPeriapsis;
        double meanAnomaly    = initialStateKeplerian.MeanAnomaly;

        for (double t = dt; t < timelineDuration + dt; t += dt)
        {
            JulianDate jd = startTime + sta::secsToDays(t);
            deviation = propagateENCKE(mu, reference, dt, perturbationsList, jd, stateVector, deviation,  q, integrator, propFeedback);

            // PropagateTWObody is used to propagate the reference trajectory
            double argOfPeriapsisUpdated      = 0.0;
            double meanAnomalyUpdated         = 0.0;
            double raanUpdated                = 0.0;
            reference = propagateTWObody(mu, sma, e, inclination, argOfPeriapsis, raan, meanAnomaly,
                                         dt,
                                         raanUpdated, argOfPeriapsisUpdated, meanAnomalyUpdated);

            argOfPeriapsis = argOfPeriapsisUpdated;
            meanAnomaly    = meanAnomalyUpdated;
            raan           = raanUpdated;

            // Calculating the perturbed trajectory
            stateVector = reference + deviation;
            q = deviation.position.dot(reference.position + 0.5 * deviation.position) / pow(reference.position.norm(), 2.0);

#if 0
            // Rectification of the reference trajectory, when the deviation is too large.
            if (q > 0.01)
            {
                sta::KeplerianElements keplerian = cartesianTOorbital(mu, stateVector);

                sma = keplerian.SemimajorAxis;
                e = keplerian.Eccentricity;
                inclination = keplerian.Inclination;
                argOfPeriapsis = keplerian.ArgumentOfPeriapsis;
                raan = keplerian.AscendingNode;
                meanAnomaly = keplerian.MeanAnomaly;

                q = 0;
                reference = stateVector;
                deviation = sta::StateVector(null, null);
            }
#endif

            // Append a trajectory sample every outputRate integration steps (and always at the last step.)
            if (steps % outputRate == 0 || t >= timelineDuration)
            {
                sampleTimes << jd;
                samples << stateVector;

            }
            ++steps;
        }
    }
    else if (propagator == "GAUSS")
    {
        for (double t = dt; t < timelineDuration + dt; t += dt)
        {
            JulianDate jd = startTime + sta::secsToDays(t);
            // Append a trajectory sample every outputRate integration steps (and always at the last step.)
            if (steps % outputRate == 0 || t >= timelineDuration)
            {
                sampleTimes << jd;
                samples << stateVector;

            }
            ++steps;

            stateVector = propagateGAUSS(mu, stateVector, dt, perturbationsList, jd, integrator);

        }
    }
    else
    {
        propFeedback.raiseError(QObject::tr("Unsupported propagator '%1'").arg(propagator));
        return false;
    }

    return true;

} // ------------------------------- End of the propagation method -----------------------
void LoiteringDialog::on_pushButtonAspect_clicked()
{
    loiteringAspect.exec();
}
