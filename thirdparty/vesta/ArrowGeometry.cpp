/*
 * $Revision: 223 $ $Date: 2010-03-30 05:44:44 -0700 (Tue, 30 Mar 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#include "ArrowGeometry.h"
#include "RenderContext.h"
#include "Material.h"
#include "Units.h"
#include "IntegerTypes.h"
#include <Eigen/Core>
#include <cassert>

using namespace vesta;
using namespace Eigen;
using namespace std;


static const unsigned int ARROW_SECTIONS = 20;


ArrowGeometry::ArrowGeometry(float shaftLength,
                             float shaftRadius,
                             float headLength,
                             float headRadius) :
    m_scale(1.0),
    m_opacity(1.0f),
    m_visibleArrows(XAxis),
    m_geometryBoundingRadius(0.0f),
    m_cap(0),
    m_shaft(0),
    m_annulus(0),
    m_point(0),
    m_vertices(0)
{
    m_arrowColors[0] = Spectrum(1.0f, 0.0f, 0.0f);
    m_arrowColors[1] = Spectrum(0.0f, 1.0f, 0.0f);
    m_arrowColors[2] = Spectrum(0.0f, 0.0f, 1.0f);
    buildArrowGeometry(shaftLength, shaftRadius, headLength, headRadius);
}


ArrowGeometry::~ArrowGeometry()
{
    delete m_cap;
    delete m_shaft;
    delete m_annulus;
    delete m_point;
    delete m_vertices;
}


Spectrum
ArrowGeometry::arrowColor(unsigned int which) const
{
    if (which < 3)
    {
        return m_arrowColors[which];
    }
    else
    {
        return Spectrum();
    }
}


/** Set the color of the one of the arrows.
 *
 *  @param which a value between 0 and 2 (inclusive), with x = 0, y = 1, z = 2
 *  @param color the color of the arrow
 */
void
ArrowGeometry::setArrowColor(unsigned int which, const Spectrum& color)
{
    if (which < 3)
    {
        m_arrowColors[which] = color;
    }
}


void
ArrowGeometry::render(RenderContext& rc,
                      float /* cameraDistance */,
                      double /* animationTime */) const
{
    Material materials[3];
    for (unsigned int i = 0; i < 3; i++)
    {
        materials[i].setEmission(m_arrowColors[i]);
        materials[i].setOpacity(m_opacity);
    }

    rc.bindVertexArray(m_vertices);

    rc.pushModelView();
    rc.scaleModelView(Vector3f::Constant((float) m_scale));

    if (visibleArrows() & XAxis)
    {
        rc.pushModelView();
        rc.rotateModelView(Quaternionf(AngleAxisf((float) PI / 2.0f, Vector3f::UnitY())));
        rc.bindMaterial(&materials[0]);
        drawArrow(rc);
        rc.popModelView();
    }

    if (visibleArrows() & YAxis)
    {
        rc.pushModelView();
        rc.rotateModelView(Quaternionf(AngleAxisf(-(float) PI / 2.0f, Vector3f::UnitX())));
        rc.bindMaterial(&materials[1]);
        drawArrow(rc);
        rc.popModelView();
    }

    if (visibleArrows() & ZAxis)
    {
        rc.pushModelView();
        rc.bindMaterial(&materials[2]);
        drawArrow(rc);
        rc.popModelView();
    }

    rc.unbindVertexArray();

    rc.popModelView(); // pop scaling transformation    
}


float
ArrowGeometry::boundingSphereRadius() const
{
    return m_geometryBoundingRadius * (float) m_scale;
}


void
ArrowGeometry::drawArrow(RenderContext& rc) const
{
    rc.drawPrimitives(*m_cap);
    rc.drawPrimitives(*m_shaft);
    rc.drawPrimitives(*m_annulus);
    rc.drawPrimitives(*m_point);
}


void
ArrowGeometry::buildArrowGeometry(float shaftLength,
                                  float shaftRadius,
                                  float headLength,
                                  float headRadius)
{
    const unsigned int vertexCount = (ARROW_SECTIONS + 1) * 3 + 2;
    const unsigned int capCenterIndex = vertexCount - 2;
    const unsigned int tipIndex = vertexCount - 1;

    // Vertex data must be allocated as char, not Vector3; Eigen uses a custom allocator for
    // objects.
    unsigned char* byteData = new unsigned char[vertexCount * sizeof(Vector3f)];
    Vector3f* data = reinterpret_cast<Vector3f*>(byteData);//new Vector3f[vertexCount];
    int vertexIndex = 0;

    // End cap
    for (unsigned int i = 0; i <= ARROW_SECTIONS; ++i)
    {
        float theta = (float) i / (float) ARROW_SECTIONS * 2 * (float) PI;
        data[vertexIndex++] = Vector3f(std::cos(theta) * shaftRadius, std::sin(theta) * shaftRadius, 0.0f);
    }

    // Shaft
    for (unsigned int i = 0; i <= ARROW_SECTIONS; ++i)
    {
        float theta = (float) i / (float) ARROW_SECTIONS * 2 * (float) PI;
        data[vertexIndex++] = Vector3f(std::cos(theta) * shaftRadius, std::sin(theta) * shaftRadius, shaftLength);
    }

    // Annulus
    for (unsigned int i = 0; i <= ARROW_SECTIONS; ++i)
    {
        float theta = (float) i / (float) ARROW_SECTIONS * 2 * (float) PI;
        data[vertexIndex++] = Vector3f(std::cos(theta) * headRadius, std::sin(theta) * headRadius, shaftLength);
    }

    data[vertexIndex++] = Vector3f::Zero();
    data[vertexIndex++] = Vector3f(0.0f, 0.0f, shaftLength + headLength);

    // Calculate the bounding radius of the arrow geometry
    m_geometryBoundingRadius = 0.0f;
    for (unsigned i = 0; i < vertexCount; i++)
    {
        m_geometryBoundingRadius = max(m_geometryBoundingRadius, data[i].norm());
    }

    m_vertices = new VertexArray(data, vertexCount, VertexSpec::Position, sizeof(data[0]));

    // End cap of the arrow shaft
    v_uint16* capIndices = new v_uint16[ARROW_SECTIONS + 2];
    capIndices[0] = (v_uint16) capCenterIndex;
    for (unsigned int i = 0; i <= ARROW_SECTIONS; i++)
    {
        capIndices[i + 1] = (v_uint16) (ARROW_SECTIONS - i);
    }
    m_cap = new PrimitiveBatch(PrimitiveBatch::TriangleFan, capIndices, ARROW_SECTIONS);
    delete[] capIndices;

    // Shaft of the arrow
    v_uint16* shaftIndices = new v_uint16[ARROW_SECTIONS * 2 + 2];
    for (unsigned int i = 0; i <= ARROW_SECTIONS; i++)
    {
        shaftIndices[i * 2    ] = (v_uint16) (ARROW_SECTIONS + 1 + i);
        shaftIndices[i * 2 + 1] = (v_uint16) i;
    }
    m_shaft = new PrimitiveBatch(PrimitiveBatch::TriangleStrip, shaftIndices, ARROW_SECTIONS * 2);
    delete[] shaftIndices;

    // Annulus (ring connecting head of arrow to the shaft)
    v_uint16* annulusIndices = new v_uint16[ARROW_SECTIONS * 2 + 2];
    for (unsigned int i = 0; i <= ARROW_SECTIONS; i++)
    {
        annulusIndices[i * 2    ] = (v_uint16) ((ARROW_SECTIONS + 1) * 2 + i);
        annulusIndices[i * 2 + 1] = (v_uint16) ((ARROW_SECTIONS + 1) + i);
    }
    m_annulus = new PrimitiveBatch(PrimitiveBatch::TriangleStrip, annulusIndices, ARROW_SECTIONS * 2);
    delete[] annulusIndices;

    // Point of the arrow
    v_uint16* pointIndices = new v_uint16[ARROW_SECTIONS + 2];
    pointIndices[0] = (v_uint16) tipIndex;
    for (unsigned int i = 0; i <= ARROW_SECTIONS; i++)
    {
        pointIndices[i + 1] = (v_uint16) ((ARROW_SECTIONS + 1) * 2 + i);
    }
    m_point = new PrimitiveBatch(PrimitiveBatch::TriangleFan, pointIndices, ARROW_SECTIONS);
    delete[] pointIndices;
}

