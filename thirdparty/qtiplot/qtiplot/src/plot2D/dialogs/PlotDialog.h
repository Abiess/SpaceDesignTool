/***************************************************************************
    File                 : PlotDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Custom curves dialog

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef PLOTDIALOG_H
#define PLOTDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "../MultiLayer.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QWidget;
class QStringList;
class QGroupBox;
class QDoubleSpinBox;
class QRadioButton;
class QTreeWidget;

class LayerItem;
class CurveTreeItem;
class ColorBox;
class PatternBox;
class ColorButton;
class MultiLayer;
class SymbolBox;
class ColorMapEditor;
class QwtPlotItem;
class DoubleSpinBox;
class PenStyleBox;

static const char* folder_closed[]={
    "16 16 9 1",
    "g c #808080",
    "b c #c0c000",
    "e c #c0c0c0",
    "# c #000000",
    "c c #ffff00",
    ". c None",
    "a c #585858",
    "f c #a0a0a4",
    "d c #ffffff",
    "..###...........",
    ".#abc##.........",
    ".#daabc#####....",
    ".#ddeaabbccc#...",
    ".#dedeeabbbba...",
    ".#edeeeeaaaab#..",
    ".#deeeeeeefe#ba.",
    ".#eeeeeeefef#ba.",
    ".#eeeeeefeff#ba.",
    ".#eeeeefefff#ba.",
    ".##geefeffff#ba.",
    "...##gefffff#ba.",
    ".....##fffff#ba.",
    ".......##fff#b##",
    ".........##f#b##",
    "...........####."};

static const char* folder_open[]={
    "16 16 11 1",
    "# c #000000",
    "g c #c0c0c0",
    "e c #303030",
    "a c #ffa858",
    "b c #808080",
    "d c #a0a0a4",
    "f c #585858",
    "c c #ffdca8",
    "h c #dcdcdc",
    "i c #ffffff",
    ". c None",
    "....###.........",
    "....#ab##.......",
    "....#acab####...",
    "###.#acccccca#..",
    "#ddefaaaccccca#.",
    "#bdddbaaaacccab#",
    ".eddddbbaaaacab#",
    ".#bddggdbbaaaab#",
    "..edgdggggbbaab#",
    "..#bgggghghdaab#",
    "...ebhggghicfab#",
    "....#edhhiiidab#",
    "......#egiiicfb#",
    "........#egiibb#",
    "..........#egib#",
    "............#ee#"};

static const char * layer_disabled_xpm[] = {
"14 14 3 1",
" 	c None",
".	c #666666",
"+	c #FFFFFF",
"..............",
".++++++++++++.",
".+..+++++++++.",
".+..+++++++++.",
".+..+++++++++.",
".+..+++++++++.",
".+..+++++++++.",
".+..+++++++++.",
".+..+++++++++.",
".+..+++++++++.",
".+..........+.",
".+..........+.",
".++++++++++++.",
".............."};

/* XPM */
static const char * layer_enabled_xpm[] = {
"14 14 4 1",
" 	c None",
".	c #1A1A1A",
"+	c #FFFFFF",
"@	c #CB1D08",
"..............",
".++++++++++++.",
".+@@+++++++++.",
".+@@+++++++++.",
".+@@+++++++++.",
".+@@+++++++++.",
".+@@+++++++++.",
".+@@+++++++++.",
".+@@+++++++++.",
".+@@+++++++++.",
".+@@@@@@@@@@+.",
".+@@@@@@@@@@+.",
".++++++++++++.",
".............."};

static const char * graph_disabled_xpm[] = {
"14 14 5 1",
" 	c None",
".	c #666666",
"+	c #FFFFFF",
"@	c #999999",
"#	c #747474",
"..............",
".++++@@++++++.",
".++++@@++++++.",
".+++@++@+++++.",
".++@++++@@+++.",
".+@+++++@@++#.",
".++++##+++@+#.",
".++++##++++#+.",
".+++#++#++#+@.",
".++#++++##+++.",
".+#+++++##+++.",
".++++++++++++.",
".++++++++++++.",
".............."};

static const char * graph_enabled_xpm[] = {
"14 14 6 1",
" 	c None",
".	c #030003",
"+	c #FFFFFF",
"@	c #FC0D00",
"#	c #2F28EF",
"$	c #0000FC",
"..............",
".++++@@++++++.",
".++++@@++++++.",
".+++@++@+++++.",
".++@++++@@+++.",
".+@+++++@@++#.",
".++++##+++@+#.",
".++++##++++$+.",
".+++#++#++$+@.",
".++#++++##+++.",
".+#+++++##+++.",
".++++++++++++.",
".++++++++++++.",
".............."};

//! Custom plot/curves dialog
class PlotDialog : public QDialog
{
    Q_OBJECT

public:
    PlotDialog(bool showExtended, QWidget* parent = 0, Qt::WFlags fl = 0 );
    void initFonts(const QFont& titlefont, const QFont& axesfont, const QFont& numbersfont, const QFont& legendfont);
	void insertColumnsList(const QStringList& names){columnNames = names;};
	void setMultiLayer(MultiLayer *ml);

public slots:
	void showAll(bool all);
	void selectCurve(int index);

private slots:
	void showStatistics();
	void customVectorsPage(bool angleMag);
	void updateEndPointColumns(const QString& text);

	void fillBoxSymbols();
	void fillSymbols();
	bool acceptParams();
	void showWorksheet();
	void quit();

	int setPlotType(CurveTreeItem *item);
	void changePlotType(int plotType);
	void setActiveCurve(CurveTreeItem *item);

	void insertTabs(int plot_type);
	void updateTabWindow(QTreeWidgetItem *currentItem, QTreeWidgetItem *previousItem);
	void showAreaColor(bool show);

	void removeSelectedCurve();

	/******* error bars options **************/
	void pickErrorBarsColor();
	void changeErrorBarsType();
	void changeErrorBarsPlus();
	void changeErrorBarsMinus();
	void changeErrorBarsThrough();

	void setAutomaticBinning();
	bool validInput();
	void showPlotAssociations(QTreeWidgetItem *item, int);

	//box plots
	void setBoxType(int index);
	void setBoxRangeType(int index);
	void setWhiskersRange(int index);

	//spectrograms
  	void showDefaultContourLinesBox(bool show);
	void showColorMapEditor(bool show);

	//layer geometry
	void adjustLayerHeight(double width);
	void adjustLayerWidth(double height);
	void displayCoordinates(int unit, Graph *g = 0);

protected slots:
    void setActiveLayer(LayerItem *item);
    void updateTreeWidgetItem(QTreeWidgetItem *item);
    void updateBackgroundTransparency(int alpha);
    void updateCanvasTransparency(int alpha);
	void setTitlesFont();
	void setAxesLabelsFont();
	void setAxesNumbersFont();
	void setLegendsFont();
	void editCurve();
	void chooseLabelsFont();
	void applyLayerFormat();
	void setLayerDefaultValues();

private:
	void applyFormatToLayer(Graph *g);
    int labelsAlignment();
	void closeEvent(QCloseEvent* e);

    void clearTabWidget();
	void initAxesPage();
	void initLinePage();
	void initSymbolsPage();
	void initHistogramPage();
	void initErrorsPage();
	void initSpacingPage();
	void initVectPage();
	void initBoxPage();
	void initPercentilePage();
	void initSpectrogramPage();
	void initLayerPage();
	void initLayerGeometryPage();
	void initFontsPage();
	void initPiePage();
	void initPieGeometryPage();
	void initPieLabelsPage();
	void initPrintPage();
	void initLabelsPage();
    void contextMenuEvent(QContextMenuEvent *e);

	double aspect_ratio;

    QFont titleFont, legendFont, axesFont, numbersFont;

    MultiLayer *d_ml;
	QStringList columnNames;

	DoubleSpinBox* boxX, *boxY, *boxLayerWidth, *boxLayerHeight;
	QCheckBox *keepRatioBox;

    QPushButton *btnTitle, *btnAxesLabels, *btnAxesNumbers, *btnLegend;
	ColorMapEditor *colorMapEditor;
    QWidget *curvePlotTypeBox, *layerPage, *layerGeometryPage, *piePage, *fontsPage, *printPage;
    QTreeWidget* listBox;
    QCheckBox *boxAntialiasing, *boxScaleLayers, *boxPrintCrops;
    ColorButton *boxBorderColor, *boxBackgroundColor, *boxCanvasColor;
	QSpinBox *boxBackgroundTransparency, *boxCanvasTransparency, *boxBorderWidth, *boxMargin;
	QSpinBox *boxRadius;
	DoubleSpinBox *boxPieLineWidth;
    ColorBox *boxFirstColor, *boxPieLineColor;
    PatternBox *boxPiePattern;
    PenStyleBox* boxPieLineStyle;

    QPushButton* buttonApply, *btnWorksheet;
    QPushButton* buttonOk, *btnMore;
    QPushButton* buttonCancel;
    QComboBox* boxPlotType;
    QWidget* linePage;
    QComboBox* boxConnect;
    PenStyleBox* boxLineStyle;
    DoubleSpinBox *boxLineWidth, *boxPenWidth;
    ColorBox* boxLineColor, *boxAreaColor;
    QWidget* symbolPage;
    QSpinBox* boxSymbolSize;
    ColorBox* boxSymbolColor,*boxFillColor;
    SymbolBox* boxSymbolStyle;
    PatternBox *boxPattern;
	QTabWidget* privateTabWidget;
	QWidget *errorsPage, *spectrogramPage;
	QGroupBox* fillGroupBox;
    QCheckBox* plusBox;
    QCheckBox* minusBox;
    QCheckBox* xBox;
    ColorButton *colorBox, *levelsColorBox;
	ColorBox* vectColorBox;
    DoubleSpinBox* widthBox;
    QComboBox* capBox;
    QCheckBox* throughBox;
	QLabel *labelPosition, *labelXEnd, *labelYEnd;
	QGroupBox* GroupBoxH;
	QWidget *histogramPage, *spacingPage;
	QLineEdit *binSizeBox, *histogramBeginBox, *histogramEndBox;
	QCheckBox *automaticBox;
	QPushButton* buttonStatistics, *btnEditCurve;
	QSpinBox* gapBox, *offsetBox, *boxWidth;
	QWidget *vectPage, *boxPage, *percentilePage, *axesPage;
	QComboBox *xEndBox, *yEndBox, *boxType, *boxWhiskersType, *boxWhiskersRange, *boxRange;
	QSpinBox* headAngleBox, *headLengthBox, *boxPercSize;
	DoubleSpinBox *vectWidthBox, *boxEdgeWidth;
	QCheckBox *filledHeadBox;
	QSpinBox *boxCoef, *boxWhiskersCoef;
	QCheckBox *boxFillSymbols, *boxFillSymbol;
	ColorBox *boxPercFillColor, *boxEdgeColor;
	QLabel 	*whiskerCoeffLabel, *whiskerRangeLabel, *boxCoeffLabel;
	QLabel *boxRangeLabel, *whiskerCntLabel, *boxCntLabel;
	QGroupBox *GroupBoxVectEnd;
	QComboBox *vectPosBox, *boxXAxis, *boxYAxis, *colorScaleBox;
	PenStyleBox	*boxContourStyle;
  	QSpinBox *levelsBox, *colorScaleWidthBox;
	DoubleSpinBox *contourWidthBox;
  	QGroupBox *levelsGroupBox, *axisScaleBox, *imageGroupBox;
  	QGroupBox *defaultPenBox;
  	QRadioButton *defaultScaleBox, *grayScaleBox, *customScaleBox, *defaultContourBox, *autoContourBox;

    SymbolBox *boxMaxStyle, *boxMinStyle, *boxMeanStyle, *box99Style, *box1Style;
    QDoubleSpinBox *whiskerCnt, *boxCnt;
    //!Labels page
    QGroupBox *labelsGroupBox;
    DoubleSpinBox *boxLabelsAngle;
    QSpinBox *boxLabelsXOffset, *boxLabelsYOffset;
    QCheckBox *boxLabelsWhiteOut;
    QPushButton *btnLabelsFont;
    QComboBox *boxLabelsAlign, *boxLabelsColumn;
    ColorBox* boxLabelsColor;
    QWidget *labelsPage;

    QGroupBox *pieAutoLabelsBox, *boxPieWedge;
	DoubleSpinBox *boxPieStartAzimuth, *boxPieEdgeDist, *boxPieViewAngle, *boxPieThickness;
	QCheckBox *boxPieConterClockwise, *boxPieValues, *boxPiePercentages, *boxPieCategories;
	QWidget *pieLabelsPage;
    QSpinBox *boxPieOffset;
	QWidget *pieGeometryPage;

	QComboBox *unitBox;
	QComboBox *backgroundApplyToBox;
	QPushButton *layerDefaultBtn;
};

/*****************************************************************************
 *
 * Class LayerItem
 *
 *****************************************************************************/
//! LayerItem tree widget item class
class LayerItem : public QTreeWidgetItem
{
public:
    enum {LayerTreeItem = 1001};
    LayerItem(Graph *g, QTreeWidgetItem *parent, const QString& s);

    Graph *graph() { return d_graph; };
    void setActive(bool select);

protected:
    void insertCurvesList();
    Graph *d_graph;
};

/*****************************************************************************
 *
 * Class CurveTreeItem
 *
 *****************************************************************************/
//! CurveTreeItem tree widget item class
class CurveTreeItem : public QTreeWidgetItem
{
public:
    enum {PlotCurveTreeItem = 1002};
    CurveTreeItem(QwtPlotItem *curve, LayerItem *parent, const QString& s);

    Graph* graph(){return ((LayerItem *)parent())->graph();};
    void setActive(bool on);

    const QwtPlotItem *plotItem() { return d_curve; };
    int plotItemType();
    int plotItemIndex();

protected:
    QwtPlotItem *d_curve;
};

#endif