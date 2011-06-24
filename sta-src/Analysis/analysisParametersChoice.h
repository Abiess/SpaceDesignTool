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
 ------ Copyright (C) 2011 STA Steering Board (space.trajectory.analysis AT gmail.com) ----
*/


/*
 ------------------ Author:       Guillermo Ortega               -------------------
 ------------------ Affiliation:  European Space Agency (ESA)    -------------------
 -----------------------------------------------------------------------------------

 */


#ifndef ANALYSISPARAMETERSCHOICE_H
#define ANALYSISPARAMETERSCHOICE_H

#include "ui_analysisParametersChoice.h"

#include <QFont>
#include <QDialog>
#include <QComboBox>
#include <QTreeWidgetItem>
#include <QObject>
#include <QWidget>

#include "Scenario/staschema.h"
#include "Main/propagatedscenario.h"



class analysisParametersChoice : public QWidget , private Ui::analysisParametersChoiceTabWidget
{
Q_OBJECT
public:
    analysisParametersChoice(QWidget * parent = NULL);
    ~analysisParametersChoice();
    void loadTheSTAscenario(SpaceScenario* scenario, PropagatedScenario* propagatedScenario);

public slots:
    void addSelectedParameters();
    void removeSelectedParameters();
    void on_AddDefaultPushButton_clicked();
    void on_EditTimePushButton_clicked();
    void on_DeleteTimePushButton_clicked();

protected slots:

public:

private:
    void addParameter(QTreeWidgetItem* item);
    void comboBoxOptions(QTreeWidgetItem* item);
    int objectsIndex(QStringList AllObjects, int Index, QString ObjectType);
    bool checkIfMissionArc();

    QComboBox *TimeFramesBox();
    QComboBox *TimeUnitsBox();
    QComboBox *AngleUnitsBox();
    QComboBox *DistanceUnitsBox();
    QComboBox *CoordinateBox();
    QComboBox *VelocityUnitsBox();
    QComboBox *CoveredAreaUnitsBox();
    QComboBox *NoUnitsBox();

private:
    QList<ScenarioTransmitterPayloadType*>TxSC;
    QList<ScenarioTransmitterPayloadType*>TxGS;
    QList<ScenarioReceiverPayloadType*>RxSC;
    QList<ScenarioReceiverPayloadType*>RxGS;
    QList<ScenarioGroundStationEnvironment*>GSEnvironment;
    SpaceScenario* m_scenario;
    PropagatedScenario* m_propagatedScenario;


};


#endif //ANALYSISPARAMETERSCHOICE_H



