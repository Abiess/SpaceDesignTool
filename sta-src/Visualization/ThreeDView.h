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
------------------ Author: Chris Laurel  -------------------------------------------------
 ------------------ E-mail: (claurel@gmail.com) ----------------------------
 */

#ifndef _THREED_VIEW_H_
#define _THREED_VIEW_H_

#include "Astro-Core/stabody.h"
#include <vesta/Object.h>
#include <QGLWidget>
#include <QHash>

namespace vesta
{
    class Universe;
    class UniverseRenderer;
    class Observer;
    class ObserverController;
    class Entity;
    class Body;
    class Frame;
    class TextureMap;
    class TextureMapLoader;
    class TextureFont;
    class MeshGeometry;
    class Atmosphere;
    class PlanetGridLayer;
    class CubeMapFramebuffer;
    class Spectrum;
}

class StaBody;
class PropagatedScenario;
class SpaceObject;
class GroundObject;
class MissionArc;
class NetworkTextureLoader;


/** ThreeDView is a widget with a rendered 3D view of the current space scenario.
  */
class ThreeDView : public QGLWidget
{
public:
    Q_OBJECT

public:
    ThreeDView(const QGLFormat& format, QWidget *parent = 0);
    ~ThreeDView();

   QSize minimumSizeHint() const;
   QSize sizeHint() const;

   double currentTime() const
   {
       return m_currentTime;
   }

   enum StereoMode
   {
       NoStereo              = 0,
       AnaglyphCyanRed       = 1,
       AnaglyphRedCyan       = 2,
       AnaglyphGreenMagenta  = 3,
       AnaglyphMagentaGreen  = 4,
   };

   StereoMode stereoMode() const
   {
       return m_stereoMode;
   }

public slots:
   void setCurrentTime(double t);
   void tick(double dt);

   void setViewChanged();

   void gotoBody(const StaBody* body);
   void setCameraViewpoint(const QString& viewName);
   void setScenario(PropagatedScenario* scenario);
   void selectParticipant(SpaceObject* spaceObject);

   // Rendering
   void setStars(bool enabled);
   void setAtmospheres(bool enabled);
   void setClouds(bool enabled);
   void setShadows(bool enabled);
   void setReflections(bool enabled);

   // Visualizers
   void setEquatorialPlane(bool enabled);
   void setEquatorialGrid(bool enabled);
   void setPlanetGrid(bool enabled);
   void setSensorFovs(bool enabled);
   void setSatelliteTrajectories(bool enabled);
   void setReentryTrajectories(bool enabled);
   void setPlanetOrbits(bool enabled);
   void setMoonOrbits(bool enabled);
   void setPlanetLabels(bool enabled);

   void setCurrentSkyLayer(int layerIndex);
   void setAmbientLight(float lightLevel);

   void setInertialObserver();
   void setBodyFixedObserver();

   void setStereoMode(int mode);

   void setVisibleTrajectoryPortion(double days);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent* event);
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);

private:
    void initializeUniverse();
    void initializeMapLayers();
    void initializeLayers();
    void initializeStandardResources();
    void initializeStarCatalog(const QString& fileName);
    vesta::Body* addSolarSystemBody(const StaBody* body, vesta::Entity* center);
    vesta::Entity* findSolarSystemBody(const StaBody* body);
    vesta::Entity* findSolarSystemBody(StaBodyId id);
    vesta::Frame* createFrame(const MissionArc* arc);
    vesta::Entity* pickObject(const QPoint& pos);
    vesta::MeshGeometry* getSpacecraftModel(const QString& name);

    void createOrbitVisualizer(const StaBody* body, const vesta::Spectrum& color);

    void drawOverlay();

    vesta::Body* createSpaceObject(const SpaceObject* spaceObj);
    vesta::Body* createGroundObject(const GroundObject* groundObj);
    void clearScenarioObjects();

    void updateSolarSystemTrajectoryPlots();

    void setObserverCenter(vesta::Entity* center);

private:
    double m_currentTime;
    vesta::Universe* m_universe;
    vesta::UniverseRenderer* m_renderer;
    vesta::counted_ptr<vesta::Observer> m_observer;
    vesta::counted_ptr<vesta::ObserverController> m_controller;
    vesta::counted_ptr<NetworkTextureLoader> m_textureLoader;
    vesta::counted_ptr<vesta::TextureFont> m_labelFont;
    vesta::counted_ptr<vesta::MeshGeometry> m_defaultSpacecraftMesh;

    vesta::counted_ptr<vesta::TextureMap> m_spacecraftIcon;
    vesta::counted_ptr<vesta::TextureMap> m_planetIcon;
    vesta::counted_ptr<vesta::CubeMapFramebuffer> m_reflectionMap;

    vesta::counted_ptr<vesta::PlanetGridLayer> m_planetGrid;

    double m_fov;

    QPointF m_mousePosition;
    float m_mouseMotion;

    QHash<SpaceObject*, vesta::Entity*> m_scenarioSpaceObjects;
    QHash<GroundObject*, vesta::Entity*> m_scenarioGroundObjects;
    QHash<vesta::Entity*, vesta::counted_ptr<vesta::Atmosphere> > m_atmospheres;
    QHash<QString, vesta::counted_ptr<vesta::MeshGeometry> > m_spacecraftMeshes;
    QList<vesta::Atmosphere*> m_newAtmospheres;

    int m_viewChanged;
    SpaceObject* m_selectedSpacecraft;

    bool m_satelliteTrajectoriesEnabled;
    bool m_shadowsEnabled;
    bool m_reflectionsEnabled;
    bool m_glInitialized;
    bool m_shadowsInitialized;
    bool m_sensorFovsEnabled;

    StereoMode m_stereoMode;

    bool m_rollLeftKey;
    bool m_rollRightKey;
    bool m_pitchDownKey;
    bool m_pitchUpKey;

signals:
    void stereoModeChanged(StereoMode mode);  // Guillermo
};

#endif // _THREED_VIEW_H_
