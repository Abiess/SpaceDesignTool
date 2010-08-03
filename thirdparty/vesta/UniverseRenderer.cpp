/*
 * $Revision: 397 $ $Date: 2010-07-30 23:47:00 -0700 (Fri, 30 Jul 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#include "UniverseRenderer.h"
#include "RenderContext.h"
#include "Observer.h"
#include "Geometry.h"
#include "Debug.h"
#include "BoundingSphere.h"
#include "Visualizer.h"
#include "OGLHeaders.h"
#include "Framebuffer.h"
#include "CubeMapFramebuffer.h"
#include "glhelp/GLFramebuffer.h"
#include "Units.h"
#include <Eigen/Geometry>
#include <algorithm>

using namespace vesta;
using namespace Eigen;
using namespace std;

#define DEBUG_SHADOW_MAP 0

static const float MinimumNearPlaneDistance = 0.00001f;  // 1 centimeter
static const float MinimumNearFarRatio = 0.001f;
static const float PreferredNearFarRatio = 0.002f;


UniverseRenderer::UniverseRenderer() :
    m_renderContext(NULL),
    m_universe(NULL),
    m_currentTime(0.0),
    m_shadowsEnabled(false),
    m_visualizersEnabled(true),
    m_skyLayersEnabled(true),
    m_renderViewport(1, 1)
{
    m_renderContext = new RenderContext();
}


UniverseRenderer::~UniverseRenderer()
{
    delete m_renderContext;
}


/** Return true if shadows are supported for this renderer. In order to support shadows,
 *  the OpenGL implementation must support both shaders and framebuffer objects.
 */
bool
UniverseRenderer::shadowsSupported() const
{
    return Framebuffer::supported() && m_renderContext->shaderCapability() != RenderContext::FixedFunction;
}


/** Enable or disable the drawing of shadows.
  */
void
UniverseRenderer::setShadowsEnabled(bool enable)
{
    if (!m_shadowMap.isNull() && m_shadowMap->isValid())
    {
        m_shadowsEnabled = enable;
    }
}


/** Enable or disable the drawing of visualizers.
  */
void
UniverseRenderer::setVisualizersEnabled(bool enable)
{
    m_visualizersEnabled = enable;
}


/** Enable or disable the drawing of sky layers. Layers may
  * also be shown or hidden individually by calling setVisibility()
  * on the layer. In order for a layer to be drawn, sky layers
  * must be enabled in the renderer and the visibility of the
  * layer must be set to true.
  */
void
UniverseRenderer::setSkyLayersEnabled(bool enable)
{
    m_skyLayersEnabled = enable;
}


/** Initialize shadows for this renderer.
  *
  * @param shadowMapSize dimension of the square shadow map. A higher value will produce
  * better shadows but consume more memory. A smaller map may be allocated if the requested
  * size is larger than the maximum texture size supported by hardware
  *
  * @param maxShadowMaps number of shadow maps to allocate. The number of shadows cast on
  *                      any one body is limited by this value.
  */
bool
UniverseRenderer::initializeShadowMaps(unsigned int shadowMapSize, unsigned int maxShadowMaps)
{
    if (!shadowsSupported())
    {
        VESTA_LOG("Shadows not supported by graphic hardware and/or drivers.");
        return false;
    }

    // Constrain the shadow map size to the maximum size permitted by the hardware
    GLint maxTexSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    shadowMapSize = min((unsigned int) maxTexSize, shadowMapSize);

    m_shadowsEnabled = false;
    m_shadowMap = Framebuffer::CreateDepthOnlyFramebuffer(shadowMapSize, shadowMapSize);

    if (m_shadowMap.isNull())
    {
        return false;
    }

    VESTA_LOG("Created %d %dx%d shadow buffer(s) for UniverseRenderer.", maxShadowMaps, shadowMapSize, shadowMapSize);

    return true;
}


UniverseRenderer::RenderStatus
UniverseRenderer::beginViewSet(const Universe& universe, double t)
{
    if (m_universe)
    {
        return RenderViewSetAlreadyStarted;
    }

    m_universe = &universe;
    m_currentTime = t;

    return RenderOk;
}


UniverseRenderer::RenderStatus
UniverseRenderer::endViewSet()
{
    if (!m_universe)
    {
        return RenderNoViewSet;
    }

    m_universe = NULL;

    return RenderOk;
}


static bool
visibleItemPredicate(const vesta::UniverseRenderer::VisibleItem& item0,
                     const vesta::UniverseRenderer::VisibleItem& item1)
{
    return item0.farDistance < item1.farDistance;
}


#if DEBUG_SHADOW_MAP
// Debugging code for shadows
static void
showShadowMap(Framebuffer* shadowMap,
              float quadSize,
              float viewportWidth, float viewportHeight)
{
    if (shadowMap->isValid())
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, viewportWidth, 0, viewportHeight);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_LIGHTING);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glDisable(GL_DEPTH_TEST);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, shadowMap->depthTexHandle());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(quadSize, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(quadSize, quadSize);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, quadSize);
        glEnd();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    }
}
#endif // DEBUG_SHADOW_MAP


/** Render visible bodies in the universe using the specified camera position
  * and orientation.
  *
  * @param lighting information about lights and shadows that could affect objects in view
  * @param cameraPosition the camera position
  * @param cameraOrientation the camera orientation
  * @param fieldOfView the horizontal field of view in radians
  * @param viewport rectangular region of the rendering surface to draw into
  * @param renderSurface target framebuffer; the default value of NULL means that the default back buffer will be used.
  */
UniverseRenderer::RenderStatus
UniverseRenderer::renderView(const LightingEnvironment* lighting,
                             const Vector3d& cameraPosition,
                             const Quaterniond& cameraOrientation,
                             double fieldOfView,
                             const Viewport& viewport,
                             Framebuffer* renderSurface)
{
    if (!m_universe)
    {
        return RenderNoViewSet;
    }

    // Save the viewport and render surface so that they can be reset after
    // shadow and reflection rendering.
    m_renderSurface = renderSurface;
    m_renderViewport = viewport;

    glViewport(viewport.x(), viewport.y(), viewport.width(), viewport.height());

    Matrix3f toCameraSpace = cameraOrientation.conjugate().cast<float>().toRotationMatrix();
    float aspectRatio = viewport.aspectRatio();

    // A negative field of view indicates that a left-handed perspective projection
    // should be used.
    // TODO: replace this with a mechanism for specifying a general projection
    // matrix.
    if (fieldOfView < 0.0)
    {
        fieldOfView = -fieldOfView;
        aspectRatio = -aspectRatio;
        glFrontFace(GL_CW);
    }

    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);

    m_renderContext->setCameraOrientation(cameraOrientation.cast<float>());
    m_renderContext->setPixelSize((float) (2 * tan(fieldOfView / 2.0) / viewport.height()));
    m_renderContext->setViewportSize(viewport.width(), viewport.height());

    m_renderContext->pushModelView();
    m_renderContext->rotateModelView(cameraOrientation.conjugate().cast<float>());

    // Draw sky layers grids
    glDepthMask(GL_FALSE);
    glDisable(GL_TEXTURE_2D);

    m_renderContext->perspectiveProjection((float) fieldOfView, aspectRatio, 0.1f, 10.0f);

    if (m_skyLayersEnabled)
    {
        for (vector<counted_ptr<SkyLayer> >::iterator iter = m_skyLayers.begin();
             iter != m_skyLayers.end(); ++iter)
        {
            glDisable(GL_LIGHTING);
            iter->ptr()->render(*m_renderContext);
        }
    }
    glEnable(GL_LIGHTING);

    glDepthMask(GL_TRUE);

    // Fixed function state setup
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);

    m_renderContext->setActiveLightCount(1);
    m_renderContext->setAmbientLight(m_ambientLight);

    // This factor will ensure that the view frustum near plane
    // doesn't intersect the geometry of a body.
    float nearAdjust = (float) (cos(fieldOfView / 2.0) / sqrt(1.0 + aspectRatio * aspectRatio));

    const vector<Entity*>& entities = m_universe->entities();

    m_visibleItems.clear();
    m_splittableItems.clear();
    m_lightSources.clear();

    m_lighting = lighting;

    // Add a light source for the Sun
    // TODO: Consider whether it might be good to *not* set this automatically
    LightSourceItem sunItem;
    sunItem.lightSource = 0;
    sunItem.position = Vector3d::Zero();
    sunItem.cameraRelativePosition = -cameraPosition;
    m_lightSources.push_back(sunItem);

    // Simply scan through all entities in the universe. For much better performance with
    // many entities, we should maintain a bounding sphere hierarchy.
    for (vector<Entity*>::const_iterator iter = entities.begin(); iter != entities.end(); ++iter)
    {
        const Entity* entity = *iter;

        if (entity->isVisible(m_currentTime))
        {
            Vector3d position = entity->position(m_currentTime);

            // Calculate the difference at double precision, then convert to single
            // precision for the rest of the work.
            Vector3d cameraRelativePosition = (position - cameraPosition);

            // If the item is a light, add it to the light source list
            const LightSource* light = entity->lightSource();
            if (light)
            {
                LightSourceItem lsi;
                lsi.lightSource = light;
                lsi.position = position;
                lsi.cameraRelativePosition = cameraRelativePosition;
                m_lightSources.push_back(lsi);
            }

            // Cull objects based on size. If an object is less than one pixel in size,
            // we don't draw its geometry. Visualizers have sizes that may be unrelated
            // to the size of the object, so we don't cull them.
            // TODO: Add a method to the visualizer class that specifies whether the size
            // culling test (i.e. if the visualizer geometry has a fixed size on screen--such
            // as a label--then it shouldn't be culled.)
            bool sizeCull = false;
            if (entity->geometry())
            {
                float projectedSize = (entity->geometry()->boundingSphereRadius() / float(cameraRelativePosition.norm())) / m_renderContext->pixelSize();
                sizeCull = projectedSize < 1.0f;
            }
            else
            {
                // Objects without geometry are always culled.
                sizeCull = true;
            }

            // We need the camera space position of the object in order to depth
            // sort the objects.
            Vector3f cameraSpacePosition = toCameraSpace * cameraRelativePosition.cast<float>();

            if (!sizeCull)
            {
                addVisibleItem(entity, entity->geometry(),
                               position, cameraRelativePosition, cameraSpacePosition,
                               entity->orientation(m_currentTime).cast<float>(),
                               nearAdjust);
            }

            if (entity->hasVisualizers() && m_visualizersEnabled)
            {
                for (Entity::VisualizerTable::const_iterator iter = entity->visualizers()->begin();
                     iter != entity->visualizers()->end(); ++iter)
                {
                    const Visualizer* visualizer = iter->second.ptr();
                    if (visualizer->isVisible())
                    {
                        Vector3d adjustedPosition = cameraRelativePosition;
                        Vector3f adjustedCameraSpacePosition = cameraSpacePosition;

                        if (visualizer->depthAdjustment() == Visualizer::AdjustToFront)
                        {
                            // Adjust the position of the visualizer so that it is drawn in
                            // front of the object to which it is attached.
                            if (entity->geometry())
                            {
                                float z = -cameraSpacePosition.z() - entity->geometry()->boundingSphereRadius();
                                float f = z / -cameraSpacePosition.z();
                                adjustedPosition *= f;
                                adjustedCameraSpacePosition *= f;
                            }
                        }

                        addVisibleItem(entity, visualizer->geometry(),
                                       position, adjustedPosition, adjustedCameraSpacePosition,
                                       visualizer->orientation(entity, m_currentTime).cast<float>(),
                                       nearAdjust);
                    }
                }
            }
        }
    }

    // Depth sort all visible items
    sort(m_visibleItems.begin(), m_visibleItems.end(), visibleItemPredicate);

    splitDepthBuffer();
    coalesceDepthBuffer();

    // If there is splittable geometry, we need to add extra depth spans
    // at the front and back, otherwise it may be clipped.
    if (!m_splittableItems.empty() && !m_depthBufferSpans.empty())
    {
        // Use a different near/far ratio for these extra spans
        const float MaxFarNearRatio = 10000.0f;

        while (m_mergedDepthBufferSpans.back().nearDistance > MinimumNearPlaneDistance)
        {
            // Some potentially confusing naming here: spans are stored in
            // reverse order, so that the foreground span is actually the
            // *last* one in the list.
            DepthBufferSpan front;
            front.backItemIndex = 0;
            front.itemCount = 0;
            front.farDistance = m_mergedDepthBufferSpans.back().nearDistance;
            front.nearDistance = std::max(MinimumNearPlaneDistance, front.farDistance / MaxFarNearRatio);
            m_mergedDepthBufferSpans.push_back(front);
        }

        DepthBufferSpan back;
        back.backItemIndex = 0;
        back.itemCount = 0;
        back.nearDistance = m_mergedDepthBufferSpans.front().farDistance;
        back.farDistance = back.nearDistance * MaxFarNearRatio;
        m_mergedDepthBufferSpans.insert(m_mergedDepthBufferSpans.begin(), back);
    }

#if DEBUG_DEPTH_SPANS
    // cerr << "split: " << m_depthBufferSpans.size() << ", merged: " << m_mergedDepthBufferSpans.size() << endl;
    cerr << "spans: ";
    for (unsigned int i = 0; i < m_depthBufferSpans.size(); ++i)
    {
        cerr << "( " << m_depthBufferSpans[i].nearDistance << ", " << m_depthBufferSpans[i].farDistance << " ) ";
    }
    cerr << endl;

    cerr << "merged: ";
    for (unsigned int i = 0; i < m_mergedDepthBufferSpans.size(); ++i)
    {
        cerr << "( " << m_mergedDepthBufferSpans[i].nearDistance << ", " << m_mergedDepthBufferSpans[i].farDistance << " ) ";
    }
    cerr << endl;
#endif // DEBUG_DEPTH_SPANS

    // Draw depth buffer spans from back to front
    unsigned int spanIndex = m_mergedDepthBufferSpans.size() - 1;
    float spanRange = 1.0f;
    if (!m_mergedDepthBufferSpans.empty())
    {
        spanRange /= (float) m_mergedDepthBufferSpans.size();
    }

    for (vector<DepthBufferSpan>::const_iterator iter = m_mergedDepthBufferSpans.begin();
         iter != m_mergedDepthBufferSpans.end(); ++iter, --spanIndex)
    {
        setDepthRange(spanIndex * spanRange, (spanIndex + 1) * spanRange);
        renderDepthBufferSpan(*iter, (float) fieldOfView, aspectRatio);
    }
    setDepthRange(0.0f, 1.0f);

    m_renderContext->popModelView();

    m_renderContext->unbindShader();

    // Reset the front face
    glFrontFace(GL_CCW);

#if DEBUG_SHADOW_MAP
    if (m_shadowsEnabled && !m_shadowMap.isNull())
    {
        showShadowMap(m_shadowMap.ptr(), 320.0f, viewportWidth, viewportHeight);
    }
#endif

    // Don't hold on to the lighting environment pointer
    m_lighting = NULL;

    return RenderOk;
}


/** Render visible bodies in the universe from the point of view of the
  * specified observer.
  *
  * @param information about lights and shadows that could affect objects in view
  * @param observer the observer.
  * @param fieldOfView the horizontal field of view in radians
  * @param viewport rectangular region of the rendering surface to draw into
  * @param renderSurface target framebuffer; the default value of NULL means that the default back buffer will be used.
  */
UniverseRenderer::RenderStatus
UniverseRenderer::renderView(const LightingEnvironment* lighting,
                             const Observer* observer,
                             double fieldOfView,
                             const Viewport& viewport,
                             Framebuffer* renderSurface)
{
    return renderView(lighting,
                      observer->absolutePosition(m_currentTime),
                      observer->absoluteOrientation(m_currentTime),
                      fieldOfView,
                      viewport,
                      renderSurface);
}


/** Render visible bodies in the universe from the point of view of the
  * specified observer. This method is just a shortcut for the renderView
  * method that accepts a render surface and viewport parameter.
  *
  * @param observer the observer.
  * @param fieldOfView the horizontal field of view in radians
  * @param viewportWidth the width of the viewport in pixels
  * @param viewportHeight the height of the viewport in pixels
  */
UniverseRenderer::RenderStatus
UniverseRenderer::renderView(const Observer* observer,
                             double fieldOfView,
                             int viewportWidth,
                             int viewportHeight)
{
    return renderView(NULL, observer, fieldOfView, Viewport(viewportWidth, viewportHeight), NULL);
}


void UniverseRenderer::setDepthRange(float front, float back)
{
    m_depthRangeFront = front;
    m_depthRangeBack = back;
    glDepthRange(front, back);
}


void
UniverseRenderer::addVisibleItem(const Entity* entity,
                                 const Geometry* geometry,
                                 const Vector3d& position,
                                 const Vector3d& cameraRelativePosition,
                                 const Vector3f& cameraSpacePosition,
                                 const Quaternionf& orientation,
                                 float nearAdjust)
{
    // Compute the signed distance from the camera plane to the most
    // distant part of the entity. A distance < 0 indicates that the
    // entity lies completely behind the camera.
    float boundingRadius = geometry->boundingSphereRadius();
    float farDistance = -cameraSpacePosition.z() + boundingRadius;

    // Calculate a near distance that's as far from the camera as possible.
    float nearDistance = cameraRelativePosition.norm() - boundingRadius;

    // Generally, the near distance for an individual object will never be less
    // than MinimumNearFarRatio times the bounding diameter. Exceptions are things
    // like trajectories, which should never be clipped by the near plane. This
    // is handled by marking trajectories as splittable, so that they will be
    // drawn into multiple depth buffer spans when necessary.
    switch (geometry->clippingPolicy())
    {
    case Geometry::PreserveDepthPrecision:
        nearDistance = std::max(nearDistance, boundingRadius * MinimumNearFarRatio * 2.0f);
        break;

    case Geometry::PreventClipping:
    case Geometry::SplitToPreventClipping:
        nearDistance = std::max(nearDistance, MinimumNearPlaneDistance);
        break;
    }

    // ...but make sure that the near plane of the view frustum doesn't
    // intersect the object's geometry. Note that if nearDistance is greater
    // farDistance, it means that the object lies outside the view frustum.
    nearDistance *= nearAdjust;

    // Add entities in front of the camera to the list of visible items
    if (farDistance > 0 && nearDistance < farDistance)
    {
        VisibleItem visibleItem;
        visibleItem.entity = entity;
        visibleItem.geometry = geometry;
        visibleItem.position = position;
        visibleItem.cameraRelativePosition = cameraRelativePosition;
        visibleItem.orientation = orientation;
        visibleItem.boundingRadius = boundingRadius;
        visibleItem.nearDistance = nearDistance;
        visibleItem.farDistance = farDistance;

        if (geometry->clippingPolicy() == Geometry::SplitToPreventClipping)
        {
            m_splittableItems.push_back(visibleItem);
        }
        else
        {
            m_visibleItems.push_back(visibleItem);
        }
    }
}


/** Render six views into the faces of a cube map.
  */
UniverseRenderer::RenderStatus
UniverseRenderer::renderCubeMap(const LightingEnvironment* lighting, const Vector3d& position, CubeMapFramebuffer* cubeMap)
{
    Quaterniond z180 = Quaterniond(AngleAxisd(toRadians(180.0), Vector3d::UnitZ()));
    Quaterniond rotations[6] =
    {
        Quaterniond(AngleAxisd(toRadians(-90.0), Vector3d::UnitY())) * z180,
        Quaterniond(AngleAxisd(toRadians( 90.0), Vector3d::UnitY())) * z180,
        Quaterniond(AngleAxisd(toRadians( 90.0), Vector3d::UnitX())) * z180,
        Quaterniond(AngleAxisd(toRadians(-90.0), Vector3d::UnitX())) * z180,
        Quaterniond(AngleAxisd(toRadians(  0.0), Vector3d::UnitY())) * z180,
        Quaterniond(AngleAxisd(toRadians(180.0), Vector3d::UnitY())) * z180,
    };

    Viewport viewport(cubeMap->size(), cubeMap->size());

    for (int face = 0; face < 6; ++face)
    {
        Framebuffer* fb = cubeMap->face(CubeMapFramebuffer::Face(face));
        if (fb)
        {
            fb->bind();
            glDepthMask(GL_TRUE);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderStatus status = renderView(lighting, position, rotations[face], toRadians(-90.0), viewport, fb);
            if (status != RenderOk)
            {
                Framebuffer::unbind();
                return status;
            }
        }
    }

    Framebuffer::unbind();

    return RenderOk;
}


// Split the depth buffer up into one or more spans.
void
UniverseRenderer::splitDepthBuffer()
{
    m_depthBufferSpans.clear();

    // Iterate over the visible items from back to front
    for (int i = (int) m_visibleItems.size() - 1; i >= 0; --i)
    {
        const VisibleItem& item = m_visibleItems[i];

        float nearDistance = item.nearDistance;
        if (m_depthBufferSpans.empty())
        {
            DepthBufferSpan span;
            span.backItemIndex = (unsigned int) i;
            span.itemCount = 1;
            span.farDistance = item.farDistance;
            span.nearDistance = nearDistance;

            m_depthBufferSpans.push_back(span);
        }
        else
        {
            DepthBufferSpan& span = m_depthBufferSpans.back();
            bool isDisjoint = item.farDistance < span.nearDistance;

            if (isDisjoint)
            {
                // Item doesn't overlap the current depth buffer span. Create two
                // new spans: one containing item, and one for the empty range in
                // between the new span and the current span.
                DepthBufferSpan emptySpan;
                emptySpan.farDistance = span.nearDistance;
                emptySpan.nearDistance = item.farDistance;
                emptySpan.itemCount = 0;
                emptySpan.backItemIndex = (unsigned int) i;

                // Start a new span
                DepthBufferSpan newSpan;
                newSpan.farDistance = item.farDistance;
                newSpan.nearDistance = nearDistance;
                newSpan.backItemIndex = (unsigned int) i;
                newSpan.itemCount = 1;

                m_depthBufferSpans.push_back(emptySpan);
                m_depthBufferSpans.push_back(newSpan);
            }
            else
            {
                span.itemCount++;
                if (nearDistance < span.nearDistance)
                {
                    span.nearDistance = nearDistance;
                }
            }
        }
    }
}


// Coalesce adjacent depth buffer spans that are of approximately
// the same size. This will prevent over-partitioning of the the
// depth buffer while still preserving a maximum far/near ratio.
void
UniverseRenderer::coalesceDepthBuffer()
{
    m_mergedDepthBufferSpans.clear();

    unsigned int i = 0;
    while (i < m_depthBufferSpans.size())
    {
        float farDistance = m_depthBufferSpans[i].farDistance;
        unsigned int itemCount = m_depthBufferSpans[i].itemCount;

        // Coalesce all spans into a single span that's as large as possible
        // without near/far being less than the preferred near-far ratio. This
        // will reduce the number of depth buffer spans without sacrificing
        // depth buffer precision.
        unsigned int j = i;
        while (j < m_depthBufferSpans.size() - 1)
        {
            if (m_depthBufferSpans[j + 1].nearDistance / farDistance < PreferredNearFarRatio)
            {
                break;
            }

            itemCount += m_depthBufferSpans[j + 1].itemCount;
            ++j;
        }

        DepthBufferSpan span;
        span.farDistance = farDistance;
        span.nearDistance = m_depthBufferSpans[j].nearDistance;
        span.backItemIndex = m_depthBufferSpans[i].backItemIndex;
        span.itemCount = itemCount;

        m_mergedDepthBufferSpans.push_back(span);

        i = j + 1;
    }
}


static void
beginShadowRendering()
{
    // Use depth-only rendering for shadows
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    // Reduce 'shadow acne' by rendering the backfaces. This doesn't
    // eliminate the artifacts, but moves them to the unilluminated
    // side of the object, where they're less visible.
    glCullFace(GL_FRONT);
}


void
finishShadowRendering(Framebuffer* renderSurface)
{
    if (renderSurface)
    {
        renderSurface->bind();
    }
    else
    {
        Framebuffer::unbind();
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glCullFace(GL_BACK);
}



// Render all of the items in a depth buffer span
void UniverseRenderer::renderDepthBufferSpan(const DepthBufferSpan& span,
                                             float fieldOfView,
                                             float aspectRatio)
{
    if (span.itemCount == 0 && m_splittableItems.empty())
    {
        return;
    }

    bool shadowsOn = false;
    if (m_shadowsEnabled && !m_lightSources.empty())
    {
        shadowsOn = renderDepthBufferSpanShadows(span, m_lightSources[0].cameraRelativePosition);
    }

    // Enforce the minimum near plane distance
    float nearDistance = std::max(MinimumNearPlaneDistance, span.nearDistance);

    m_renderContext->perspectiveProjection(fieldOfView, aspectRatio, nearDistance, span.farDistance);

    // Rendering of some translucent objects is order dependent. We can eliminate the
    // worst artifacts by drawing opaque items first and translucent items second.
    for (int pass = 0; pass < 2; ++pass)
    {
        m_renderContext->setPass(pass == 0 ? RenderContext::OpaquePass : RenderContext::TranslucentPass);

        // Draw all items in the span
        for (unsigned int i = 0; i < span.itemCount; i++)
        {
            const VisibleItem& item = m_visibleItems[span.backItemIndex - i];

            if (pass == 0 || !item.geometry->isOpaque())
            {
                if (shadowsOn && item.entity->geometry()->isShadowReceiver())
                {
                    m_renderContext->setShadowMapCount(1);
                }
                else
                {
                    m_renderContext->setShadowMapCount(0);
                }

                if (m_lighting && !m_lighting->reflectionRegions().empty())
                {
                    m_renderContext->setEnvironmentMap(m_lighting->reflectionRegions().front().cubeMap);
                }
                else
                {
                    m_renderContext->setEnvironmentMap(NULL);
                }
                drawItem(item);
            }
        }

        // Disable all shadows
        m_renderContext->setShadowMapCount(0);

        // Draw all splittable items that fall at least partly within this span.
        for (unsigned int i = 0; i < m_splittableItems.size(); ++i)
        {
            const VisibleItem& item = m_splittableItems[m_splittableItems.size() - i - 1];

            if (item.nearDistance < span.farDistance && item.farDistance > span.nearDistance)
            {
                if (pass == 0 || !item.geometry->isOpaque())
                {
                    drawItem(item);
                }
            }
        }
    }
}


// Render all shadow casters in a depth buffer span into the shadow map. Return true if
// any shadows were actually drawn.
//
// Parameters:
//   span - the depth buffer span for which to draw shadows
//   lightPosition - the position of the light source relative to the camera
bool
UniverseRenderer::renderDepthBufferSpanShadows(const DepthBufferSpan& span,
                                               const Vector3d& lightPosition)
{
    if (!m_shadowsEnabled)
    {
        return false;
    }

    // Check for shadow support
    if (!Framebuffer::supported() ||
        m_shadowMap.isNull() ||
        !m_shadowMap->isValid())
    {
        return false;
    }

    BoundingSphere<float> shadowReceiverBounds;
    bool shadowCastersPresent = false;

    // Find the minimum radius bounding sphere that contains all of the
    // shadow receivers in this span. Also, determine whether there are
    // any shadow casters in the span.
    for (unsigned int i = 0; i < span.itemCount; ++i)
    {
        const VisibleItem& item = m_visibleItems[span.backItemIndex - i];
        const Geometry* geometry = item.geometry;

        if (geometry->isShadowReceiver())
        {
            shadowReceiverBounds.merge(BoundingSphere<float>(item.cameraRelativePosition.cast<float>(), item.boundingRadius));
        }

        if (geometry->isShadowCaster())
        {
            shadowCastersPresent = true;
        }
    }

    // Don't draw shadows if there are no receivers or no casters
    if (!shadowCastersPresent || shadowReceiverBounds.isEmpty())
    {
        return false;
    }

    glDepthRange(0.0f, 1.0f);
    beginShadowRendering();

    Vector3f shadowGroupCenter = shadowReceiverBounds.center();
    float shadowGroupBoundingRadius = shadowReceiverBounds.radius();

    // Compute the light direction. Here it assumed that all objects in the shadow group
    // are far enough from the light source that the rays are nearly parallel and the
    // light source direction is effectively constant.
    Vector3f lightDirection = (lightPosition + shadowGroupCenter.cast<double>()).cast<float>().normalized();

    // Compute the shadow transform, which will convert coordinates from "shadow group space" to
    // shadow space. Shadow group space has axes aligned with world space but has an origin located
    // at the center of the collection of mutually shadowing objects.
    Matrix4f invCameraTransform = m_renderContext->modelview().matrix().transpose();
    Matrix4f shadowTransform = setupShadowRendering(m_shadowMap.ptr(), lightDirection, shadowGroupBoundingRadius);
    shadowTransform = shadowTransform * Transform3f(Translation3f(-shadowGroupCenter)).matrix() * invCameraTransform;

    // Render shadows for all casters
    for (unsigned int i = 0; i < span.itemCount; ++i)
    {
        const VisibleItem& item = m_visibleItems[span.backItemIndex - i];
        const Geometry* geometry = item.entity->geometry();

        if (geometry->isShadowCaster())
        {
            Vector3f itemPosition = item.cameraRelativePosition.cast<float>();
            m_renderContext->pushModelView();
            m_renderContext->translateModelView(itemPosition - shadowGroupCenter);
            m_renderContext->rotateModelView(item.orientation);
            item.geometry->renderShadow(*m_renderContext, item.cameraRelativePosition.norm() - item.boundingRadius, m_currentTime);
            m_renderContext->popModelView();
        }
    }

    // Pop the matrices pushed in setupShadowRendering()
    m_renderContext->popProjection();
    m_renderContext->popModelView();

    finishShadowRendering(m_renderSurface.ptr());

    // Reset the viewport
    glDepthRange(m_depthRangeFront, m_depthRangeBack);
    glViewport(m_renderViewport.x(), m_renderViewport.y(), m_renderViewport.width(), m_renderViewport.height());

    // Set shadow state in the render context
    m_renderContext->setShadowMapMatrix(0, shadowTransform);
    m_renderContext->setShadowMap(0, m_shadowMap->glFramebuffer());
    m_renderContext->setShadowMapCount(1);

    return true;
}


void
UniverseRenderer::drawItem(const VisibleItem& item)
{       
    m_renderContext->setModelTranslation(m_renderContext->modelview().linear().cast<double>() * item.cameraRelativePosition);

    // Set up the light sources
    unsigned int lightCount = 0;
    if (!m_lightSources.empty())
    {
        for (vector<LightSourceItem>::const_iterator iter = m_lightSources.begin(); iter != m_lightSources.end(); ++iter)
        {
            if (!iter->lightSource)
            {
                // Special case for the Sun
                m_renderContext->setLight(0, RenderContext::Light(RenderContext::DirectionalLight,
                                                                  iter->cameraRelativePosition.cast<float>(),
                                                                  Spectrum(1.0f, 1.0f, 1.0f)));
                ++lightCount;
            }
            else
            {
                Vector3f lightPosition = (iter->position - item.position).cast<float>();
                float distanceToLight = lightPosition.norm() - item.boundingRadius;
                if (distanceToLight < iter->lightSource->range())
                {
                    m_renderContext->setLight(lightCount,
                                              RenderContext::Light(RenderContext::PointLight,
                                                                   iter->cameraRelativePosition.cast<float>(),
                                                                   iter->lightSource->spectrum()));
                    ++lightCount;
                }
            }
        }
    }

    m_renderContext->setActiveLightCount(lightCount);

    m_renderContext->pushModelView();
    m_renderContext->translateModelView(item.cameraRelativePosition.cast<float>());
    m_renderContext->rotateModelView(item.orientation);

    item.geometry->render(*m_renderContext, item.cameraRelativePosition.norm() - item.boundingRadius, m_currentTime);

    m_renderContext->popModelView();
}


void
UniverseRenderer::setAmbientLight(const Spectrum& spectrum)
{
    m_ambientLight = spectrum;
}


void
UniverseRenderer::addSkyLayer(SkyLayer* grid)
{
    m_skyLayers.push_back(counted_ptr<SkyLayer>(grid));
}


/** Remove the sky layer at the specified index. The method has no effect if
  * the index is invalid.
  */
void
UniverseRenderer::removeSkyLayer(unsigned int index)
{
    if (index < m_skyLayers.size())
    {
        m_skyLayers.erase(m_skyLayers.begin() + index);
    }
}


/** Get the sky layer at the specified index. If the index is out
  * of range, the method returns null.
  */
SkyLayer*
UniverseRenderer::skyLayer(unsigned int index) const
{
    if (index >= m_skyLayers.size())
    {
        return 0;
    }
    else
    {
        return m_skyLayers[index].ptr();
    }
}


/** Get the number of sky layers
  */
unsigned int
UniverseRenderer::skyLayerCount() const
{
    return m_skyLayers.size();
}


// Create a view matrix for drawing the scene from the
// point of view of a light source.
static Matrix4f
shadowView(const Vector3f& lightDirection)
{
    Vector3f u = lightDirection.unitOrthogonal();
    Vector3f v = u.cross(lightDirection);
    Matrix4f lightView;
    lightView << v.transpose(),              0.0,
                 u.transpose(),              0.0,
                 lightDirection.transpose(), 0.0,
                 0.0, 0.0, 0.0,  1.0;

    return lightView;
}


// Shadow bias matrix for mapping to a unit cube with
// one corner at the origin (since texture coordinates
// are in [ 0, 1 ] instead of [ -1, 1 ])
static Matrix4f
shadowBias()
{
    Matrix4f bias;
    bias << 0.5f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.5f, 0.0f, 0.5f,
            0.0f, 0.0f, 0.5f, 0.5f,
            0.0f, 0.0f, 0.0f, 1.0f;

    return bias;
}


static Matrix4f
orthoProjectionMatrix(float left, float right,
                      float bottom, float top,
                      float zNear, float zFar)
{
    Vector3f extents(right - left, top - bottom, zFar - zNear);
    Matrix4f ortho;

    ortho << 2.0f / extents.x(), 0.0f, 0.0f,  (right + left) / -extents.x(),
             0.0f, 2.0f / extents.y(), 0.0f,  (top + bottom) / -extents.y(),
             0.0f, 0.0f, -2.0f / extents.z(), (zFar + zNear) / -extents.z(),
             0.0f, 0.0f, 0.0f,                1.0f;

    return ortho;
}


// Set up graphics state for rendering shadows. Return the matrix that
// should be used for drawing geometry with this shadow map.
Matrix4f
UniverseRenderer::setupShadowRendering(const Framebuffer* shadowMap,
                                       const Vector3f& lightDirection,
                                       float shadowGroupSize)
{
    if (!shadowMap->isValid())
    {
        return Matrix4f::Identity();
    }

    shadowMap->bind();

#if DEBUG_SHADOW_MAP
    GLenum errCode = glGetError();
    if (errCode != GL_NO_ERROR)
    {
        VESTA_LOG("glError in shadow setup: %s", gluErrorString(errCode));
    }
#endif

    Matrix4f projection = orthoProjectionMatrix(-shadowGroupSize, shadowGroupSize,
                                                -shadowGroupSize, shadowGroupSize,
                                                -shadowGroupSize, shadowGroupSize);
    Matrix4f modelView = shadowView(lightDirection);

    glClear(GL_DEPTH_BUFFER_BIT);

    m_renderContext->pushProjection();
    m_renderContext->setProjection(projection);
    m_renderContext->pushModelView();
    m_renderContext->setModelView(modelView);

    glViewport(0, 0, shadowMap->width(), shadowMap->height());
    glDepthRange(0.0f, 1.0f);

    return shadowBias() * projection * modelView;
}



