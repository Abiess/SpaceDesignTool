/*
 Further information about the European Union Public Licence - EUPL v.1.1 can
 also be found on the world wide web at http://ec.europa.eu/idabc/eupl
 */

/*
 ------ Copyright (C) 2010 STA Steering Board (space.trajectory.analysis AT gmail.com) ----
 ------------------ Author: Ricardo Noriega  ----------------------------------------------
 ------------------ email: ricardonor@gmail.com  ------------------------------------------

 Patched by Guillermo on April 2011 t finish this up properly

 */

#include "transmitterPayloadDialog.h"

#include <Astro-Core/constants.h>
#include <Coverage/commanalysis.h>

#include <QDebug>


int antennaRadioButtonTransmitter;
QString polarisationTypeTransmitter="Linear";
QString beamTypeTx="Parabolic";



transmitterPayloadDialog::transmitterPayloadDialog( QWidget * parent, Qt::WindowFlags f) : QDialog(parent,f)
{
	setupUi(this);

    QDoubleValidator* positiveDoubleValidator = new QDoubleValidator(this);
    positiveDoubleValidator->setBottom(0.0);

    QDoubleValidator* efficiencyValidator = new QDoubleValidator(this);
    efficiencyValidator->setBottom(0.0);
    efficiencyValidator->setTop(100.0);

    // changed this to range -90, 90 to turn antennas also in direction of space (need it for ground stations with antennas)
    QDoubleValidator* elevationValidator = new QDoubleValidator(this);
    elevationValidator->setRange(-90, 90, 2);

    ElLineEdit->setValidator(elevationValidator);
    //ElLineEdit->setValidator(positiveDoubleValidator);

    TxFeederLossLineEdit->setValidator(positiveDoubleValidator);
    TxDepointingLossLineEdit->setValidator(positiveDoubleValidator);
    GainLineEdit->setValidator(positiveDoubleValidator);
    DiameterLineEdit->setValidator(positiveDoubleValidator);
    BeamLineEdit->setValidator(positiveDoubleValidator);
    TiltLineEdit->setValidator(positiveDoubleValidator);
    EfficiencyLineEdit->setValidator(efficiencyValidator);
    FrequencyLineEdit->setValidator(positiveDoubleValidator);
    PowerLineEdit->setValidator(positiveDoubleValidator);
    DataRateLineEdit->setValidator(positiveDoubleValidator);

    ConeAngleLineEdit->setValidator(positiveDoubleValidator);
    HorAngleLineEdit->setValidator(positiveDoubleValidator);
    VertAngleLineEdit->setValidator(positiveDoubleValidator);
}




transmitterPayloadDialog::~transmitterPayloadDialog()
{
}



// This is the function that load the values of the XML schema on the GUI
bool transmitterPayloadDialog::loadValues(ScenarioTransmitterPayloadType* transmitterPayload)
{
    m_transmitterPayload=transmitterPayload;

    //These lines allow the GUI to remember which choice the user did for the antenna parameters
    if(antennaRadioButtonTransmitter==1)
        DiameterRadioButton->toggle();
    if(antennaRadioButtonTransmitter==2)
        BeamWidthRadioButton->toggle();

    // Observation? yes or no
    observationCheckBox->setChecked(transmitterPayload->Transmitter()->ObservationChecked());

    double elevation=transmitterPayload->Transmitter()->PointingDirection()->elevation();
    elevation=elevation*RAD2DEG;
    if(elevation>90)
        ElLineEdit->setText(QString::number(90.0));
    else if (elevation<-90)
        ElLineEdit->setText(QString::number(-90.0));
    else
        ElLineEdit->setText(QString::number(elevation));

    double azimuth=transmitterPayload->Transmitter()->PointingDirection()->azimuth();
    azimuth=azimuth*RAD2DEG;
    AzLineEdit->setText(QString::number(azimuth));

    double coneAngle = transmitterPayload->Transmitter()->PointingDirection()->coneAngle();
    coneAngle = coneAngle*RAD2DEG;
    ConeAngleLineEdit->setText(QString::number(coneAngle));

    int coneShape = transmitterPayload->Transmitter()->PointingDirection()->coneShape();
    ConeShapeComboBox->setCurrentIndex(coneShape);

    TxFeederLossLineEdit->setText(QString::number(transmitterPayload->Transmitter()->FedderLossTx()));
    TxDepointingLossLineEdit->setText(QString::number(transmitterPayload->Transmitter()->DepointingLossTx()));

    GainLineEdit->setText(QString::number(transmitterPayload->Transmitter()->EMproperties()->GainMax()));
    DiameterLineEdit->setText(QString::number(transmitterPayload->Transmitter()->EMproperties()->Diameter()));
    BeamLineEdit->setText(QString::number(transmitterPayload->Transmitter()->EMproperties()->AngularBeamWidth()));
    EfficiencyLineEdit->setText(QString::number(transmitterPayload->Transmitter()->EMproperties()->Efficiency()));
    PowerLineEdit->setText(QString::number(transmitterPayload->Transmitter()->TransmittingPower()));

    double tilt=transmitterPayload->Transmitter()->EMproperties()->TiltAngle();
    tilt=tilt*RAD2DEG;
    TiltLineEdit->setText(QString::number(tilt));

    double frustumAngle1 = transmitterPayload->Transmitter()->Coverage()->FrustumAngle1();
    double frustumAngle2 = transmitterPayload->Transmitter()->Coverage()->FrustumAngle2();
    QString frustumShape = transmitterPayload->Transmitter()->Coverage()->FrustumShape().toLower();
    HorAngleLineEdit->setText(QString::number(frustumAngle1));
    VertAngleLineEdit->setText(QString::number(frustumAngle2));
    if (frustumShape == "ellipse")
    {
        ConeShapeComboBox->setCurrentIndex(1);
    }
    else
    {
        ConeShapeComboBox->setCurrentIndex(0);
    }

    double frequency=transmitterPayload->Budget()->FrequencyBand();
    frequency=frequency/1000000000;//from Hz to GHz
    FrequencyLineEdit->setText(QString::number(frequency));//This is the frequency taken from the budget!!

    double bandWidth=transmitterPayload->Transmitter()->EMproperties()->BandWidth();
    bandWidth=bandWidth/1000000;//from Hz to MHz
    BandWidthLineEdit->setText(QString::number(bandWidth));

    double dataRate=transmitterPayload->Transmitter()->Modulation()->DataRate();
    dataRate=dataRate/1000000;//from bps to Mbps
    DataRateLineEdit->setText(QString::number(dataRate));
    //For the Modulation combo box
    QString modType;

    int i=0;
    modType=transmitterPayload->Transmitter()->Modulation()->ModulationType();


    if(modType=="BPSK")
        i=0;
    else if (modType=="DE-BPSK")
        i=1;
    else if (modType=="D-BPSK")
        i=2;
    else if (modType=="QPSK")
        i=3;
    else if (modType=="DE-QPSK")
        i=4;
    else if (modType=="D-QPSK")
        i=5;
    else if (modType=="OQPSK")
        i=6;

    TypeModComboBox->setCurrentIndex(i);

    antennaCalculations(transmitterPayload);

    //These lines allow the GUI to remember which choice the user did for the type of polarisation
    if(polarisationTypeTransmitter=="Linear"){
        TiltLineEdit->setEnabled(true);
        PolarisationComboBox->setCurrentIndex(0);}
    if(polarisationTypeTransmitter=="rightCircular"){
        TiltLineEdit->setText(QString::number(45));
        TiltLineEdit->setDisabled(true);
        transmitterPayload->Transmitter()->EMproperties()->setTiltAngle(45*DEG2RAD);
        PolarisationComboBox->setCurrentIndex(1);}
    if(polarisationTypeTransmitter=="leftCircular"){
        TiltLineEdit->setText(QString::number(45));
        TiltLineEdit->setDisabled(true);
        transmitterPayload->Transmitter()->EMproperties()->setTiltAngle(45*DEG2RAD);
        PolarisationComboBox->setCurrentIndex(2);}

    //These lines allow the GUI to remember which choice the user did for the type of beam
    if(beamTypeTx=="Parabolic"){
        TypeBeamComboBox->setCurrentIndex(0);
        transmitterPayload->Transmitter()->EMproperties()->setBeamType("Parabolic");
        AntennaSizeGroupBox->setEnabled(true);
    } else if (beamTypeTx=="Omni-directional"){
        TypeBeamComboBox->setCurrentIndex(1);
        transmitterPayload->Transmitter()->EMproperties()->setBeamType("Omni-directional");
        AntennaSizeGroupBox->setDisabled(true);
    }


    // Patched by Guillermo to load the budgets
    double myMass = transmitterPayload->Budget()->Mass();
    MassLineEdit->setText(QString::number(myMass));
    double myLength = transmitterPayload->Budget()->Size()->Length();
    LengthLineEdit->setText(QString::number(myLength));
    double myWidth = transmitterPayload->Budget()->Size()->Width();
    WidthLineEdit->setText(QString::number(myWidth));
    double myHeight = transmitterPayload->Budget()->Size()->Height();
    HightLineEdit->setText(QString::number(myHeight));

    return true;
}







// This is the function that load the values of the GUI into the XML schema
bool transmitterPayloadDialog::saveValues(ScenarioTransmitterPayloadType* transmitterPayload)
{

    // Observation? yes or no
    transmitterPayload->Transmitter()->setObservationChecked(observationCheckBox->isChecked());

    double elevation=ElLineEdit->text().toDouble();
    if(elevation<=90){
        elevation=elevation*DEG2RAD;
    }else{
        elevation=90*DEG2RAD;
    }
    transmitterPayload->Transmitter()->PointingDirection()->setElevation(elevation);

    double azimuth=AzLineEdit->text().toDouble();
    azimuth=azimuth*DEG2RAD;
    transmitterPayload->Transmitter()->PointingDirection()->setAzimuth(azimuth);

    double coneAngle = ConeAngleLineEdit->text().toDouble();
    coneAngle = coneAngle*DEG2RAD;
    transmitterPayload->Transmitter()->PointingDirection()->setConeAngle(coneAngle);

    int coneShape = ConeShapeComboBox->currentIndex();
    transmitterPayload->Transmitter()->PointingDirection()->setConeShape(coneShape);

    transmitterPayload->Transmitter()->EMproperties()->setGainMax(GainLineEdit->text().toDouble());
    transmitterPayload->Transmitter()->EMproperties()->setDiameter(DiameterLineEdit->text().toDouble());
    transmitterPayload->Transmitter()->EMproperties()->setAngularBeamWidth(BeamLineEdit->text().toDouble());
    transmitterPayload->Transmitter()->EMproperties()->setEfficiency(EfficiencyLineEdit->text().toDouble());

    double tilt=TiltLineEdit->text().toDouble();
    tilt=tilt*DEG2RAD;
    transmitterPayload->Transmitter()->EMproperties()->setTiltAngle(tilt);

    double frequency=FrequencyLineEdit->text().toDouble();
    frequency=frequency*1000000000;
    transmitterPayload->Budget()->setFrequencyBand(frequency);

    double bandWidth=BandWidthLineEdit->text().toDouble();
    bandWidth=bandWidth*1000000;
    transmitterPayload->Transmitter()->EMproperties()->setBandWidth(bandWidth);


    transmitterPayload->Transmitter()->setTransmittingPower(PowerLineEdit->text().toDouble());
    transmitterPayload->Transmitter()->setFedderLossTx(TxFeederLossLineEdit->text().toDouble());
    transmitterPayload->Transmitter()->setDepointingLossTx(TxDepointingLossLineEdit->text().toDouble());

    double dataRate=DataRateLineEdit->text().toDouble();
    dataRate=dataRate*1000000;
    transmitterPayload->Transmitter()->Modulation()->setDataRate(dataRate);


    transmitterPayload->Transmitter()->Modulation()->setModulationType(TypeModComboBox->currentText());


    antennaCalculations(transmitterPayload);

    //These lines allow the GUI to remember which choice the user did for the antenna parameters
    if(GainMaxRadioButton->isChecked()==true)
        antennaRadioButtonTransmitter=0;
    if(DiameterRadioButton->isChecked()==true)
        antennaRadioButtonTransmitter=1;
    if(BeamWidthRadioButton->isChecked()==true)
        antennaRadioButtonTransmitter=2;

    //These lines allow the GUI to remember which choice the user did for the type of polarisation
    if(PolarisationComboBox->currentText()=="Linear"){
        polarisationTypeTransmitter="Linear";
        transmitterPayload->Transmitter()->EMproperties()->setPolarisation("Linear");
    }
    if(PolarisationComboBox->currentText()=="Right Circular"){
        polarisationTypeTransmitter="rightCircular";
        transmitterPayload->Transmitter()->EMproperties()->setPolarisation("rightCircular");
    }
    if(PolarisationComboBox->currentText()=="Left Circular"){
        transmitterPayload->Transmitter()->EMproperties()->setPolarisation("leftCircular");
        polarisationTypeTransmitter="leftCircular";
    }

    if(TypeBeamComboBox->currentText()=="Parabolic"){
        beamTypeTx="Parabolic";
        transmitterPayload->Transmitter()->EMproperties()->setBeamType("Parabolic");
    } else if(TypeBeamComboBox->currentText()=="Omni-directional"){
        beamTypeTx="Omni-directional";
        transmitterPayload->Transmitter()->EMproperties()->setBeamType("Omni-directional");
    }

    double frustumAngle1 = HorAngleLineEdit->text().toDouble();
    double frustumAngle2 = VertAngleLineEdit->text().toDouble();
    transmitterPayload->Transmitter()->Coverage()->setFrustumAngle1(frustumAngle1);
    transmitterPayload->Transmitter()->Coverage()->setFrustumAngle2(frustumAngle2);
    if (ConeShapeComboBox->currentIndex() == 0)
    {
        transmitterPayload->Transmitter()->Coverage()->setFrustumShape("rectangle");
    }
    else
    {
        transmitterPayload->Transmitter()->Coverage()->setFrustumShape("ellipse");
    }

    // Patched by Guillermo to load the budgets
    double myMass = MassLineEdit->text().toDouble();
    transmitterPayload->Budget()->setMass(myMass);
    double myLength = LengthLineEdit->text().toDouble();
    transmitterPayload->Budget()->Size()->setLength(myLength);
    double myWidth = WidthLineEdit->text().toDouble();
    transmitterPayload->Budget()->Size()->setWidth(myWidth);
    double myHeight = HightLineEdit->text().toDouble();
    transmitterPayload->Budget()->Size()->setHeight(myHeight);

    return true;
}


void transmitterPayloadDialog::antennaCalculations(ScenarioTransmitterPayloadType* transmitterPayload)
{

    double Pi=3.141592;
    double lightSpeed=SPEED_OF_LIGHT;
    double efficiency=transmitterPayload->Transmitter()->EMproperties()->Efficiency();
    double frequency=transmitterPayload->Budget()->FrequencyBand();


    double diameter, beamwidth, gainMaxDb, gainMax;

    if(GainMaxRadioButton->isChecked()==true)
    {
        //Since we have the Gain max
        gainMaxDb=transmitterPayload->Transmitter()->EMproperties()->GainMax();
        gainMax=pow(10, gainMaxDb/10);
        diameter=(lightSpeed/(Pi*frequency))*sqrt(gainMax/(efficiency/100));
        beamwidth=58*(lightSpeed/(frequency*diameter));


        //Put the values in the GUI
        DiameterLineEdit->setText(QString::number(diameter));
        BeamLineEdit->setText(QString::number(beamwidth));

    }
    if(DiameterRadioButton->isChecked()==true)
    {

        diameter=transmitterPayload->Transmitter()->EMproperties()->Diameter();


        gainMax=(efficiency/100)*pow((Pi*diameter*frequency/lightSpeed),2); //natural units
        gainMaxDb=10*log10(gainMax); //in dB
        beamwidth=58*(lightSpeed/(frequency*diameter));

        //Put the values in the GUI
        BeamLineEdit->setText(QString::number(beamwidth));
        GainLineEdit->setText(QString::number(gainMaxDb));

    }
    if(BeamWidthRadioButton->isChecked()==true)
    {

        beamwidth=transmitterPayload->Transmitter()->EMproperties()->AngularBeamWidth();

        gainMax=(efficiency/100)*33201/pow(beamwidth,2);
        gainMaxDb=10*log10(gainMax); //in dB
        diameter=(lightSpeed/(Pi*frequency))*sqrt(gainMax/(efficiency/100));



        //Put the values in the GUI
        DiameterLineEdit->setText(QString::number(diameter));
        GainLineEdit->setText(QString::number(gainMaxDb));

    }

}



void transmitterPayloadDialog::on_buttonBox_helpRequested()
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_buttonBox_clicked(QAbstractButton*)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);


}

void transmitterPayloadDialog::on_tabWidget_currentChanged(int)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_MassLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_LengthLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_WidthLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_HightLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_VertAngleLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_HorAngleLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_ConeAngleLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_ElLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_AzLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_ConeShapeComboBox_activated(const QString&)
{

    //qWarning("TODO: %s	%d",__FILE__,__LINE__);

    if(ConeShapeComboBox->currentText()=="Circular")
        stackedWidget->setCurrentIndex(0);
    else if(ConeShapeComboBox->currentText()=="Rectangular")
        stackedWidget->setCurrentIndex(1);
}


void transmitterPayloadDialog::on_PolarisationGroupBox_toggled(bool)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_PolarisationComboBox_activated(const QString&)
{
    if(PolarisationComboBox->currentText()=="Right Circular" || PolarisationComboBox->currentText()=="Left Circular")
    {
        TiltLineEdit->setText(QString::number(45));
        TiltLineEdit->setDisabled(true);
    }
    if(PolarisationComboBox->currentText()=="Linear"){
        TiltLineEdit->setText(QString::number(0));
        TiltLineEdit->setEnabled(true);}
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_TiltLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_ModulationGroupBox_toggled(bool)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_TypeModComboBox_activated(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_DataRateLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_EquipmentGroupBox_toggled(bool)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_PowerLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_TxFeederLossLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_TxDepointingLossLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_AntennaSizeGroupBox_toggled(bool)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_GainMaxRadioButton_toggled(bool)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);

}

void transmitterPayloadDialog::on_DiameterRadioButton_toggled(bool)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_BeamWidthRadioButton_toggled(bool)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_GainLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_DiameterLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_BeamLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_groupBox_toggled(bool)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_EfficiencyLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_FrequencyLineEdit_textChanged(const QString&)
{
    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}

void transmitterPayloadDialog::on_TypeBeamComboBox_activated(const QString&)
{
    if(TypeBeamComboBox->currentText()=="Omni-directional")
    {
        GainLineEdit->setText(QString::number(0));
        AntennaSizeGroupBox->setDisabled(1);
    } else if(TypeBeamComboBox->currentText()=="Parabolic"){
        AntennaSizeGroupBox->setEnabled(1);
        GainLineEdit->setText(QString::number(30));

    }

    //qWarning("TODO: %s	%d",__FILE__,__LINE__);
}



