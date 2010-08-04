/*
 * $Revision: 380 $ $Date: 2010-07-20 17:45:27 -0700 (Tue, 20 Jul 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#include "RenderContext.h"
#include "Units.h"
#include "TextureMap.h"
#include "Material.h"
#include "TextureFont.h"
#include "OGLHeaders.h"
#include "ShaderBuilder.h"
#include "Debug.h"
#include "glhelp/GLFramebuffer.h"
#include "particlesys/ParticleEmitter.h"
#include "particlesys/ParticleRenderer.h"
#include <Eigen/LU>
#include <vector>
#include <cmath>
#include <cassert>

using namespace vesta;
using namespace Eigen;
using namespace std;


namespace vesta
{
    class ParticleBuffer
    {
    public:
        vector<ParticleEmitter::Particle> particles;
    };
}

static VertexAttribute particleVertexAttributes[] =
{
    VertexAttribute(VertexAttribute::Position,     VertexAttribute::Float3),
    VertexAttribute(VertexAttribute::PointSize,    VertexAttribute::Float1),
    VertexAttribute(VertexAttribute::TextureCoord, VertexAttribute::Float2),
    VertexAttribute(VertexAttribute::Color,        VertexAttribute::Float4),
};

static VertexSpec ParticleVertexSpec(sizeof(particleVertexAttributes) / sizeof(particleVertexAttributes[0]),
                                     particleVertexAttributes);

// Texture unit assignments
static const unsigned int BaseTextureUnit          = 0;
static const unsigned int NormalTextureUnit        = 1;
static const unsigned int SpecularTextureUnit      = 2;
static const unsigned int EmissiveTextureUnit      = 3;
static const unsigned int ShadowTextureUnit        = 4;
static const unsigned int TransmittanceTextureUnit = 6;
static const unsigned int ScatterTextureUnit       = 7;
static const unsigned int ReflectionTextureUnit    = 8;



RenderContext::Environment::Environment() :
    m_activeLightCount(0),
    m_ambientLight(0.0f, 0.0f, 0.0f),
    m_shadowMapCount(0.0f),
    m_scatteringEnabled(false)
{

}


// A RenderContext should only be constructed *after* an OpenGL context
// has been created. Otherwise, information about OpenGL capabilities will
// not be available.
RenderContext::RenderContext() :
    m_cameraOrientation(Quaterniond::Identity()),
    m_pixelSize(0.0f),
    m_renderPass(OpaquePass),
    m_modelViewStackDepth(0),
    m_projectionStackDepth(0),
    m_modelTranslation(Vector3d::Zero()),
    m_particleBuffer(NULL),
    m_vertexStream(NULL),
    m_vertexStreamFloats(NULL),
    m_shaderCapability(FixedFunction),
    m_shaderStateCurrent(false),
    m_modelViewMatrixCurrent(false)
{
    m_matrixStack[0] = Matrix4f::Identity();

    const int MaxParticles = 4096;
    m_particleBuffer = new ParticleBuffer;
    m_particleBuffer->particles.reserve(MaxParticles);

    // Make the vertex stream buffer larger enough to hold a complete particle buffer
    m_vertexStreamFloats = 4 * MaxParticles * 10;
    m_vertexStream = new float[m_vertexStreamFloats];

    // Intialize all OpenGL extensions.
    if (glewInit() != GLEW_OK)
    {
        VESTA_WARNING("OpenGL extension initialization failed. RenderContext created without an OpenGL context?");
    }

    if (GLEW_ARB_shading_language_100 && GLEW_ARB_shader_objects)
    {
        m_shaderCapability = GLSL1;
        // TODO: query and parse GL_SHADING_LANGUAGE_VERSION to discover
        // additional GLSL capabilities.
        VESTA_LOG("Creating GLSL RenderContext");
    }
    else
    {
        VESTA_LOG("Creating fixed function RenderContext");
    }
}


RenderContext::~RenderContext()
{
    if (m_modelViewStackDepth > 0)
    {
        glMatrixMode(GL_MODELVIEW);
        for (unsigned int i = 0; i < m_modelViewStackDepth; ++i)
            glPopMatrix();
    }

    delete m_particleBuffer;
    delete[] m_vertexStream;
}


void
RenderContext::setPixelSize(float pixelSize)
{
    m_pixelSize = pixelSize;
}


void
RenderContext::pushModelView()
{
    assert(m_modelViewStackDepth < MaxMatrixStackDepth - 1);
    if (m_modelViewStackDepth < MaxMatrixStackDepth - 1)
    {
        m_modelViewStackDepth++;
        m_matrixStack[m_modelViewStackDepth] = m_matrixStack[m_modelViewStackDepth - 1];
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    invalidateModelViewMatrix();
}


void
RenderContext::popModelView()
{
    assert(m_modelViewStackDepth > 0);
    if (m_modelViewStackDepth > 0)
    {
        m_modelViewStackDepth--;
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    invalidateModelViewMatrix();
}


/** Set the current modelview matrix to identity.
  */
void
RenderContext::identityModelView()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    m_matrixStack[m_modelViewStackDepth] = Transform3f::Identity();

    invalidateModelViewMatrix();
}


void
RenderContext::translateModelView(const Vector3f& v)
{
    glMatrixMode(GL_MODELVIEW);
    glTranslatef(v.x(), v.y(), v.z());
    m_matrixStack[m_modelViewStackDepth] = m_matrixStack[m_modelViewStackDepth] * Translation3f(v);

    invalidateModelViewMatrix();
}


void
RenderContext::rotateModelView(const Quaternionf& q)
{
    glMatrixMode(GL_MODELVIEW);
    glMultMatrixf(Transform3f(q).data());
    m_matrixStack[m_modelViewStackDepth] = m_matrixStack[m_modelViewStackDepth] * q;

    invalidateModelViewMatrix();
}


void
RenderContext::scaleModelView(const Vector3f& v)
{
    glMatrixMode(GL_MODELVIEW);
    glScalef(v.x(), v.y(), v.z());
    m_matrixStack[m_modelViewStackDepth] = m_matrixStack[m_modelViewStackDepth] * Scaling3f(v);

    invalidateModelViewMatrix();
}


void
RenderContext::setModelView(const Matrix4f& m)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m.data());
    m_matrixStack[m_modelViewStackDepth] = m;

    invalidateModelViewMatrix();
}


/** Get the translation in camera space at high precision.
  */
Vector3d
RenderContext::modelTranslation() const
{
    return m_modelTranslation;
}


/** Set the translation in camera space at high precision.
  */
void
RenderContext::setModelTranslation(const Vector3d& translation)
{
    m_modelTranslation = translation;
}


void
RenderContext::pushProjection()
{
    assert(m_projectionStackDepth < MaxMatrixStackDepth - 1);
    if (m_projectionStackDepth < MaxMatrixStackDepth - 1)
    {
        m_projectionStackDepth++;
        m_projectionStack[m_projectionStackDepth] = m_projectionStack[m_projectionStackDepth - 1];
        m_frustumStack[m_projectionStackDepth] = m_frustumStack[m_projectionStackDepth - 1];
    }
}


void
RenderContext::popProjection()
{
    assert(m_projectionStackDepth > 0);
    if (m_projectionStackDepth > 0)
    {
        m_projectionStackDepth--;
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(m_projectionStack[m_projectionStackDepth].data());
        glMatrixMode(GL_MODELVIEW);
    }
}


/** Set a perspective projection matrix. This is equivalent to OpenGL's gluPerspective()
  * function, except that he matrix is not multiplied with the current projection matrix.
  *
  * @param fieldOfView the vertical field of view in radians
  * @param aspectRatio the ratio of the viewport's width to its height
  * @param nearDistance distance from the camera to the near plane (always a positive value)
  * @param farDistance distance from the camera to the far plane (always a positive value)
  */
void
RenderContext::perspectiveProjection(float fieldOfView, float aspectRatio, float nearDistance, float farDistance)
{
    float f = 1.0f / std::tan(fieldOfView / 2.0f);
    float span = farDistance - nearDistance;
    m_projectionStack[m_projectionStackDepth].matrix() <<
        f / aspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, (farDistance + nearDistance) / -span, (2.0f * farDistance * nearDistance) / -span,
        0.0f, 0.0f, -1.0f, 0.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // gluPerspective(toDegrees(fieldOfView), aspectRatio, nearDistance, farDistance);
    glMultMatrixf(m_projectionStack[m_projectionStackDepth].matrix().data());
    glMatrixMode(GL_MODELVIEW);

    Frustum frustum;
    frustum.nearZ = nearDistance;
    frustum.farZ = farDistance;
    double h = 1.0 / f;
    double w = h * abs(aspectRatio);
    frustum.planeNormals[0] = Vector3d( 1.0, 0.0, -w).normalized();
    frustum.planeNormals[1] = Vector3d(-1.0, 0.0, -w).normalized();
    frustum.planeNormals[2] = Vector3d(0.0,  1.0, -h).normalized();
    frustum.planeNormals[3] = Vector3d(0.0, -1.0, -h).normalized();
    m_frustumStack[m_projectionStackDepth] = frustum;
}


/** Set an orthographic projection matrix. This function is equivalent to OpenGL's glOrtho(),
  * except that the matrix is not multiplied with the current projection matrix.
  *
  * @param left the coordinate of the left vertical clip plane
  * @param right the coordinate of the right vertical clip plane
  * @param bottom the coordinate of the bottom horizontal clip plane
  * @param top the coordinate of the top horizontal clip plane
  * @param zNear the coordinate of the near clip plane
  * @param zFar the coordinate of the far clip plane
  */
void
RenderContext::orthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
    // Same matrix as the one produced by glOrtho(left, right, bottom, top, zNear, zFar);
    m_projectionStack[m_projectionStackDepth].matrix() <<
        2.0f / (right - left), 0.0f, 0.0f, -(right + left) / (right - left),
        0.0f, 2.0f / (top - bottom), 0.0f, -(top + bottom) / (top - bottom),
        0.0f, 0.0f, -2.0f / (zFar - zNear), -(zFar + zNear) / (zFar - zNear),
        0.0f, 0.0f, 0.0f, 1.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_projectionStack[m_projectionStackDepth].matrix().data());
    glMatrixMode(GL_MODELVIEW);
}


/** Set an orthographic projection appropriate for 2D rendering. Equivalent to
  * calling orthographicProjection() with zNear = -1 and zFar = 1
  */
void
RenderContext::orthographicProjection2D(float left, float right, float bottom, float top)
{
    orthographicProjection(left, right, bottom, top, -1.0f, 1.0f);
}


/** Set the projection matrix to m.
  */
void
RenderContext::setProjection(const Matrix4f& m)
{
    m_projectionStack[m_projectionStackDepth] = m;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m.data());
    glMatrixMode(GL_MODELVIEW);
}


void
RenderContext::bindVertexArray(const VertexArray* vertexArray)
{
    bindVertexArray(vertexArray->vertexSpec(), vertexArray->data(), vertexArray->stride());
}


void
RenderContext::bindVertexArray(const VertexSpec& spec, const void* vertexData, unsigned int stride)
{
    const char* data = static_cast<const char*>(vertexData);

    unsigned int positionIndex = spec.attributeIndex(VertexAttribute::Position);
    unsigned int normalIndex   = spec.attributeIndex(VertexAttribute::Normal);
    unsigned int texCoordIndex = spec.attributeIndex(VertexAttribute::TextureCoord);
    unsigned int colorIndex    = spec.attributeIndex(VertexAttribute::Color);
    unsigned int tangentIndex  = spec.attributeIndex(VertexAttribute::Tangent);

    // Position is required
    if (positionIndex == VertexSpec::InvalidAttribute)
        return;

    // Position must be float3
    VertexAttribute positionAttr = spec.attribute(positionIndex);
    if (positionAttr.format() != VertexAttribute::Float3)
        return;

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, data + spec.attributeOffset(positionIndex));


    // Normals
    m_vertexInfo.hasNormals = false;
    if (normalIndex != VertexSpec::InvalidAttribute)
    {
        VertexAttribute normalAttr = spec.attribute(normalIndex);
        if (normalAttr.format() == VertexAttribute::Float3)
        {
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(GL_FLOAT, stride, data + spec.attributeOffset(normalIndex));
            m_vertexInfo.hasNormals = true;
        }
    }

    if (!m_vertexInfo.hasNormals)
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    // Texture coordinates
    m_vertexInfo.hasTexCoords = false;
    if (texCoordIndex != VertexSpec::InvalidAttribute)
    {
        VertexAttribute texCoordAttr = spec.attribute(texCoordIndex);
        unsigned int formatSize = 0;
        switch (texCoordAttr.format())
        {
            case VertexAttribute::Float1: formatSize = 1; break;
            case VertexAttribute::Float2: formatSize = 2; break;
            case VertexAttribute::Float3: formatSize = 3; break;
            case VertexAttribute::Float4: formatSize = 4; break;
            default: formatSize = 0; break;
        }

        if (formatSize != 0)
        {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(formatSize, GL_FLOAT, stride, data + spec.attributeOffset(texCoordIndex));
            m_vertexInfo.hasTexCoords = true;
        }
    }

    if (!m_vertexInfo.hasTexCoords)
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    // Vertex colors
    m_vertexInfo.hasColors = false;
    if (colorIndex != VertexSpec::InvalidAttribute)
    {
        VertexAttribute colorAttr = spec.attribute(colorIndex);
        unsigned int formatSize = 0;
        unsigned int formatType = GL_FLOAT;

        switch (colorAttr.format())
        {
            case VertexAttribute::Float3: formatSize = 3; break;
            case VertexAttribute::Float4: formatSize = 4; break;
            case VertexAttribute::UByte4: formatSize = 4; formatType = GL_UNSIGNED_BYTE; break;
            default: formatSize = 0; break;
        }

        if (formatSize != 0)
        {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(formatSize, formatType, stride, data + spec.attributeOffset(colorIndex));
            m_vertexInfo.hasColors = true;
        }
    }

    if (!m_vertexInfo.hasColors)
    {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    // Tangents
    m_vertexInfo.hasTangents = false;
    if (m_shaderCapability != FixedFunction)
    {
        if (tangentIndex != VertexSpec::InvalidAttribute)
        {
            VertexAttribute tangentAttr = spec.attribute(tangentIndex);
            if (tangentAttr.format() == VertexAttribute::Float3)
            {
                glEnableVertexAttribArrayARB(ShaderBuilder::TangentAttributeLocation);
                glVertexAttribPointerARB(ShaderBuilder::TangentAttributeLocation,
                                         3, GL_FLOAT, GL_FALSE, stride, data + spec.attributeOffset(tangentIndex));
                m_vertexInfo.hasTangents = true;
            }
        }

        if (!m_vertexInfo.hasTangents)
        {
            glDisableVertexAttribArrayARB(ShaderBuilder::TangentAttributeLocation);
        }
    }

    invalidateShaderState();
}


void
RenderContext::bindVertexBuffer(const VertexSpec& spec, const GLVertexBuffer* vertexBuffer, unsigned int stride)
{
    vertexBuffer->bind();
    bindVertexArray(spec, NULL, stride);
}


void
RenderContext::unbindVertexArray()
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (m_shaderCapability != FixedFunction)
    {
        glDisableVertexAttribArrayARB(ShaderBuilder::TangentAttributeLocation);
    }
    m_vertexInfo.hasColors = false;
    m_vertexInfo.hasNormals = false;
    m_vertexInfo.hasTexCoords = false;
    m_vertexInfo.hasTangents = false;
}


void
RenderContext::drawPrimitives(const PrimitiveBatch& batch)
{
    updateShaderState();
    updateShaderTransformConstants();

    GLenum oglPrimitiveType = 0;

    switch (batch.primitiveType())
    {
        case PrimitiveBatch::Triangles:     oglPrimitiveType = GL_TRIANGLES;      break;
        case PrimitiveBatch::TriangleStrip: oglPrimitiveType = GL_TRIANGLE_STRIP; break;
        case PrimitiveBatch::TriangleFan:   oglPrimitiveType = GL_TRIANGLE_FAN;   break;
        case PrimitiveBatch::Lines:         oglPrimitiveType = GL_LINES;          break;
        case PrimitiveBatch::LineStrip:     oglPrimitiveType = GL_LINE_STRIP;     break;
        case PrimitiveBatch::Points:        oglPrimitiveType = GL_POINTS;         break;
        default:
            // Unknown primitive type
            return;
    }

    if (batch.isIndexed())
    {
        glDrawElements(oglPrimitiveType,
                       batch.indexCount(),
                       batch.indexSize() == PrimitiveBatch::Index16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                       batch.indexData());
    }
    else
    {
        glDrawArrays(oglPrimitiveType, batch.firstVertex(), batch.indexCount());
    }
}


void
RenderContext::bindMaterial(const Material* material)
{
    if (m_shaderCapability == FixedFunction)
    {
        setFixedFunctionMaterial(material);
    }
    else
    {
        setShaderMaterial(material);
    }
    m_currentMaterial = *material;
    invalidateShaderState();
}


/** Enable a custom shader which will override the standard shaders
  * that are automatically generated based on the material state.
  *
  * \param customShader the custom shader to use; passing a null disables
  * the custom shader.
  */
void
RenderContext::enableCustomShader(GLShaderProgram* customShader)
{
    if (customShader != m_customShader.ptr())
    {
        if (shaderCapability() == FixedFunction && customShader != NULL)
        {
            // Print a warning; the caller should detect that shaders are unavailable and
            // provide a non-shader fallback.
            VESTA_WARNING("Using a custom shader with a fixed-function only render context.");
        }

        m_customShader = customShader;
        invalidateShaderState();
    }
}


/** Disable the current custom shader. This is equivalent to calling
  * enableCustomShader(NULL).
  */
void
RenderContext::disableCustomShader()
{
    enableCustomShader(NULL);
}


static void setBlendMode(const Material* material)
{
    if (material->opacity() < 1.0f || material->blendMode() != Material::Opaque)
    {
        glEnable(GL_BLEND);
        switch (material->blendMode())
        {
        case Material::Opaque:
        case Material::AlphaBlend:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case Material::AdditiveBlend:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case Material::PremultipliedAlphaBlend:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        }

        glDepthMask(GL_FALSE);
    }
    else
    {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
}


void
RenderContext::setFixedFunctionMaterial(const Material* material)
{
    if (!m_vertexInfo.hasNormals)
    {
        glDisable(GL_LIGHTING);

        // When there are no normals, set the color equal to the sum
        // of the diffuse and emissive
        Spectrum colorSum = material->diffuse() + material->emission();
        glColor4f(colorSum.red(), colorSum.green(), colorSum.blue(), material->opacity());
    }
    else
    {
        glEnable(GL_LIGHTING);

        // Combine diffuse spectrum with opacity for OpenGL
        Vector4f diffuse(material->diffuse().data());
        diffuse[3] = material->opacity();

        glColor4fv(diffuse.data());
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse.data());
        glMaterialfv(GL_FRONT, GL_AMBIENT, material->diffuse().data());
        glMaterialfv(GL_FRONT, GL_SPECULAR, material->specular().data());
        glMaterialfv(GL_FRONT, GL_EMISSION, material->emission().data());
        if (!material->specular().isBlack())
        {
            glMaterialf(GL_FRONT, GL_SHININESS, material->phongExponent());
        }
    }

    setBlendMode(material);

    // Bind the texture
    GLuint texId = 0;
    if (material->baseTexture())
    {
        material->baseTexture()->makeResident();
        texId = material->baseTexture()->id();
    }

    if (texId)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    // Update the lights
    for (unsigned int lightIndex = 0; lightIndex < m_environment.m_activeLightCount; ++lightIndex)
    {
        const Light& light = m_environment.m_lights[lightIndex];

        glEnable(GL_LIGHT0 + lightIndex);
        float lightDirection[] = { light.position.x(),
                                   light.position.y(),
                                   light.position.z(),
                                   0.0f };
        glLightfv(GL_LIGHT0 + lightIndex, GL_POSITION, lightDirection);
        glLightfv(GL_LIGHT0 + lightIndex, GL_DIFFUSE, light.color.data());
    }

    // Disable all unused lights
    for (unsigned int lightIndex = m_environment.m_activeLightCount; lightIndex < MaxLights; ++lightIndex)
    {
        glDisable(GL_LIGHT0 + lightIndex);
    }

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_environment.m_ambientLight.data());
}


static ShaderInfo
computeShaderInfo(const Material* material,
                  const RenderContext::VertexInfo* vertexInfo,
                  const RenderContext::Environment& environment)
{
    ShaderInfo shaderInfo;

    if (vertexInfo->hasColors)
    {
        shaderInfo.setVertexColors(true);
    }

    bool hasSpecular = false;

    if (!vertexInfo->hasNormals || environment.m_activeLightCount == 0)
    {
        // Only very limited lighting models are available when we don't have
        // surface normals.
        shaderInfo.setReflectanceModel(ShaderInfo::Emissive);
    }
    else
    {
        if (!material->specular().isBlack())
        {
            shaderInfo.setReflectanceModel(ShaderInfo::BlinnPhong);
            if (material->baseTexture() && material->specularModifier() == Material::DiffuseTextureAlpha)
            {
                shaderInfo.setSpecularMaskInDiffuseAlpha(true);
            }
            hasSpecular = true;
        }
        else
        {
            shaderInfo.setReflectanceModel(ShaderInfo::Lambert);
        }

        shaderInfo.setLightCount(environment.m_activeLightCount);
        shaderInfo.setShadowCount(std::min(environment.m_activeLightCount, environment.m_shadowMapCount));
    }

    // Set the texture properties for the shader. All textures
    // require that texture coordinates are present.
    if (vertexInfo->hasTexCoords)
    {
        if (material->baseTexture())
        {
            if (material->baseTexture()->makeResident())
            {
                shaderInfo.setTextures(ShaderInfo::DiffuseTexture);
                switch (material->baseTexture()->properties().usage)
                {
                case TextureProperties::AlphaTexture:
                    shaderInfo.setAlphaTexture(true);
                    break;
                default:
                    break;
                }
            }

            if (material->specularTexture() && material->specularTexture()->makeResident())
            {
                shaderInfo.setTextures(ShaderInfo::SpecularTexture);
            }
        }

        if (vertexInfo->hasTangents && vertexInfo->hasNormals)
        {
            if (material->normalTexture())
            {
                material->normalTexture()->makeResident();
                if (material->normalTexture()->id() != 0)
                {
                    shaderInfo.setTextures(ShaderInfo::NormalTexture);
                    // TODO: add support for compressed normal maps
                }
            }
        }
    }

    if (environment.m_scatteringEnabled)
    {
        shaderInfo.setScattering(true);
    }

    if (environment.m_sphericalGeometry)
    {
        shaderInfo.setSphericalGeometry(true);
    }

    bool useReflectionMap = !environment.m_environmentMap.isNull() && vertexInfo->hasNormals && material->isReflective();

    // Environment mapping requires normals but not texture coordinates
    if (useReflectionMap)
    {
        shaderInfo.setTextures(ShaderInfo::ReflectionTexture);
    }

    if ((hasSpecular || useReflectionMap) && material->fresnelReflectance() < 1.0f)
    {
        shaderInfo.setFresnelFalloff(true);
    }

    return shaderInfo;
}


void
RenderContext::setShaderMaterial(const Material* material)
{
    // TODO: For efficiency, we should cache the indexes of uniform variables used
    // in shaders.

    ShaderInfo shaderInfo = computeShaderInfo(material, &m_vertexInfo, m_environment);
    GLShaderProgram* shader = ShaderBuilder::GLSL()->getShader(shaderInfo);
    if (!shader)
    {
        return;
    }

    // If we've got a new shader, then the model view transform constants will have to
    // be resent.
    if (m_currentShader != shader)
    {
        invalidateModelViewMatrix();
    }

    shader->bind();
    m_currentShaderInfo = shaderInfo;
    m_currentShader = shader;

    ShaderInfo::ReflectanceModel model = shaderInfo.reflectanceModel();
    bool isViewDependent = false;
    if (model == ShaderInfo::BlinnPhong || shaderInfo.hasScattering() || shaderInfo.hasTexture(ShaderInfo::ReflectionTexture))
    {
        isViewDependent = true;
    }

    if (model == ShaderInfo::Emissive)
    {
        shader->setConstant("color", material->diffuse() + material->emission());
    }
    else
    {
        shader->setConstant("color", material->diffuse());
    }

    shader->setConstant("opacity", material->opacity());
    if (shaderInfo.hasTexture(ShaderInfo::DiffuseTexture))
    {
        glBindTexture(GL_TEXTURE_2D, material->baseTexture()->id());
        shader->setSampler("diffuseTex", 0);
    }

    if (shaderInfo.hasTexture(ShaderInfo::NormalTexture))
    {
        glActiveTexture(GL_TEXTURE0 + NormalTextureUnit);
        glBindTexture(GL_TEXTURE_2D, material->normalTexture()->id());
        glActiveTexture(GL_TEXTURE0);
        shader->setSampler("normalTex", NormalTextureUnit);
    }

    if (shaderInfo.hasTexture(ShaderInfo::SpecularTexture))
    {
        glActiveTexture(GL_TEXTURE0 + SpecularTextureUnit);
        glBindTexture(GL_TEXTURE_2D, material->specularTexture()->id());
        glActiveTexture(GL_TEXTURE0);
        shader->setSampler("specularTex", SpecularTextureUnit);
    }

    if (shaderInfo.hasTexture(ShaderInfo::ReflectionTexture))
    {
        glActiveTexture(GL_TEXTURE0 + ReflectionTextureUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, m_environment.m_environmentMap->id());
        glActiveTexture(GL_TEXTURE0);
        shader->setSampler("reflectionTex", ReflectionTextureUnit);

        // We want the model to world transformation; get it by multiplying the modelview
        // matrix by the inverse of the camera transformation.
        Matrix3f objToWorldMat = (m_cameraOrientation.toRotationMatrix() * modelview().linear());
        shader->setConstant("objToWorldMat", objToWorldMat);
    }

    // TODO: It might be more efficient to maintain the inverse modelview
    // matrix.
    Matrix4f inverseModelView = modelview().inverse();

    // Gather the light positions and colors into arrays that we can
    // pass as shader constants.
    Vector3f lightPositions[MaxLights];
    Vector3f lightColors[MaxLights];
    for (unsigned int lightIndex = 0; lightIndex < shaderInfo.lightCount(); ++lightIndex)
    {
        const Light& light = m_environment.m_lights[lightIndex];

        // Transform light positions into model space and pre-normalize
        // directional lights.
        lightPositions[lightIndex] = Transform3f(inverseModelView) * light.position;
        if (light.type == DirectionalLight)
        {
            lightPositions[lightIndex].normalize();
        }
        lightColors[lightIndex] = Vector3f(light.color.data());
    }

    if (model == ShaderInfo::Lambert || model == ShaderInfo::BlinnPhong)
    {
        shader->setConstantArray("lightPosition", lightPositions, shaderInfo.lightCount());
        shader->setConstantArray("lightColor", lightColors,  shaderInfo.lightCount());
        shader->setConstant("ambientLight", m_environment.m_ambientLight);
    }

    if (model == ShaderInfo::BlinnPhong)
    {
        shader->setConstant("specularColor", material->specular());
        shader->setConstant("phongExponent", material->phongExponent());
    }

    if (shaderInfo.hasFresnelFalloff())
    {
        shader->setConstant("fresnelReflectance", material->fresnelReflectance());
    }

    if (isViewDependent)
    {
        Vector3f eyePosition = (inverseModelView * Vector4f::UnitW()).start<3>();
        shader->setConstant("eyePosition", eyePosition);
    }

    if (shaderInfo.hasShadows())
    {
        // TODO: support multiple shadows
        if (!m_environment.m_shadowMaps[0].isNull())
        {
            glActiveTexture(GL_TEXTURE0 + ShadowTextureUnit);
            glBindTexture(GL_TEXTURE_2D, m_environment.m_shadowMaps[0]->depthTexHandle());
            glActiveTexture(GL_TEXTURE0);
            shader->setSampler("shadowTex0", ShadowTextureUnit);
            shader->setConstant("shadowTexelSize", 1.0f / float(m_environment.m_shadowMaps[0]->width()));
        }

        // Note: shadow transform is set in updateShaderTransformConstants
    }

    if (shaderInfo.hasScattering())
    {
        Vector3f Br = m_environment.m_scattering.rayleighCoeff;
        Vector3f scatterCoeffRatios = Vector3f::Constant(Br.x()).cwise() / Br;

        shader->setConstant("atmosphereRadius", m_environment.m_scattering.atmosphereRadius);
        shader->setConstant("planetRadius", m_environment.m_scattering.planetRadius);
        shader->setConstant("atmosphereColor", m_environment.m_scattering.color);
        shader->setConstant("scaleHeight", m_environment.m_scattering.rayleighScaleHeight);
        shader->setConstant("scatterCoeffRatios", scatterCoeffRatios);
        shader->setConstant("mieG", m_environment.m_scattering.mieAsymmetry);
        if (!m_environment.m_scattering.transmittanceTexture.isNull())
        {
            glActiveTexture(GL_TEXTURE0 + TransmittanceTextureUnit);
            glBindTexture(GL_TEXTURE_2D, m_environment.m_scattering.transmittanceTexture->id());
            glActiveTexture(GL_TEXTURE0);
            shader->setSampler("transmittanceTex", TransmittanceTextureUnit);
        }
        if (!m_environment.m_scattering.scatterTexture.isNull())
        {
            glActiveTexture(GL_TEXTURE0 + ScatterTextureUnit);
            glBindTexture(GL_TEXTURE_3D, m_environment.m_scattering.scatterTexture->id());
            glActiveTexture(GL_TEXTURE0);
            shader->setSampler("scatterTex", ScatterTextureUnit);
        }
    }

    setBlendMode(material);
}


// Update the shader constants (for GLSL contexts) or the fixed function state
// (for fixed function render constants.) The update is only performed if something
// has changed, causing the state invalid flag to be set.
void
RenderContext::updateShaderState()
{
    if (!m_shaderStateCurrent)
    {
        m_shaderStateCurrent = true;

        // Changing vertex attributes may require a new shader, in which case we
        // need to re-send the material parameters.
        if (m_shaderCapability != FixedFunction)
        {
            if (m_customShader.isValid())
            {
                m_customShader->bind();
            }
            else
            {
                setShaderMaterial(&m_currentMaterial);
            }
        }
        else
        {
            setFixedFunctionMaterial(&m_currentMaterial);
        }
    }
}


// Update shader constants related to the modelview transformation. This is called
// whenever either the modelview transformation or shader has changed since the
// last render call.
void
RenderContext::updateShaderTransformConstants()
{
    if (!m_modelViewMatrixCurrent)
    {
        m_modelViewMatrixCurrent = true;
        if (m_shaderCapability != FixedFunction)
        {
            // The shadow matrix must be updated whenever the modelview matrix changes; it is
            // the product of the model-to-world and world-to-shadow matrixes.
            if (m_currentShaderInfo.hasShadows())
            {
                Matrix4f shadowMatrices[MaxLights];
                for (unsigned int i = 0; i < 1; ++i)
                {
                    shadowMatrices[i] = m_environment.m_shadowMapMatrices[i] * modelview();
                }
                m_currentShader->setConstantArray("shadowMatrix", shadowMatrices, 1);
            }
        }
    }
}


void
RenderContext::invalidateShaderState()
{
    m_shaderStateCurrent = false;
}


/** Set the vertex information flags from
 *  a vertex specification. This is used by various methods
 *  that use immedate mode rendering instead of setting a vertex
 *  array. This code will eventually be revised to used vertex
 *  arrays exclusively.
 */
void
RenderContext::setVertexInfo(const VertexSpec& spec)
{
    m_vertexInfo.hasColors = false;
    m_vertexInfo.hasNormals = false;
    m_vertexInfo.hasTangents = false;
    m_vertexInfo.hasTexCoords = false;

    unsigned int normalIndex   = spec.attributeIndex(VertexAttribute::Normal);
    unsigned int texCoordIndex = spec.attributeIndex(VertexAttribute::TextureCoord);
    unsigned int colorIndex    = spec.attributeIndex(VertexAttribute::Color);
    unsigned int tangentIndex  = spec.attributeIndex(VertexAttribute::Tangent);

    if (normalIndex != VertexSpec::InvalidAttribute)
    {
        VertexAttribute normalAttr = spec.attribute(normalIndex);
        if (normalAttr.format() == VertexAttribute::Float3)
        {
            m_vertexInfo.hasNormals = true;
        }
    }

    if (texCoordIndex != VertexSpec::InvalidAttribute)
    {
        VertexAttribute texCoordAttr = spec.attribute(texCoordIndex);
        unsigned int formatSize = 0;
        switch (texCoordAttr.format())
        {
            case VertexAttribute::Float1: formatSize = 1; break;
            case VertexAttribute::Float2: formatSize = 2; break;
            case VertexAttribute::Float3: formatSize = 3; break;
            case VertexAttribute::Float4: formatSize = 4; break;
            default: formatSize = 0; break;
        }

        if (formatSize != 0)
        {
            m_vertexInfo.hasTexCoords = true;
        }
    }

    // Vertex colors
    if (colorIndex != VertexSpec::InvalidAttribute)
    {
        VertexAttribute colorAttr = spec.attribute(colorIndex);
        unsigned int formatSize = 0;

        switch (colorAttr.format())
        {
            case VertexAttribute::Float3: formatSize = 3; break;
            case VertexAttribute::Float4: formatSize = 4; break;
            case VertexAttribute::UByte4: formatSize = 4; break;
            default: formatSize = 0; break;
        }

        if (formatSize != 0)
        {
            m_vertexInfo.hasColors = true;
        }
    }

    if (tangentIndex != VertexSpec::InvalidAttribute && m_shaderCapability != FixedFunction)
    {
        if (spec.attribute(tangentIndex).format() == VertexAttribute::Float3)
        {
            m_vertexInfo.hasTangents = true;
        }
    }

    invalidateShaderState();
}


void
RenderContext::setActiveLightCount(unsigned int count)
{
    if (count < MaxLights)
    {
        if (count != m_environment.m_activeLightCount)
        {
            m_environment.m_activeLightCount = count;
            invalidateShaderState();
        }
    }
}


void
RenderContext::setLight(unsigned int index, const Light& light)
{
    if (index < MaxLights)
    {
        m_environment.m_lights[index] = light;
        m_environment.m_lights[index].position = modelview() * light.position;
        invalidateShaderState();
    }
}


void
RenderContext::setAmbientLight(const Spectrum& ambient)
{
    if (!(m_environment.m_ambientLight == ambient))
    {
        m_environment.m_ambientLight = ambient;
        invalidateShaderState();
    }
}


void
RenderContext::setShadowMapCount(unsigned int count)
{
    if (count < MaxLights)
    {
        if (count != m_environment.m_shadowMapCount)
        {
            m_environment.m_shadowMapCount = count;
            invalidateShaderState();
        }
    }
}


void
RenderContext::setShadowMapMatrix(unsigned int index, const Eigen::Matrix4f& shadowMatrix)
{
    if (index < MaxLights)
    {
        m_environment.m_shadowMapMatrices[index] = shadowMatrix;
    }
}


void
RenderContext::setShadowMap(unsigned int index, GLFramebuffer* shadowMap)
{
    if (index < MaxLights)
    {
        m_environment.m_shadowMaps[index] = shadowMap;
    }
}


void
RenderContext::setScattering(bool enabled)
{
    if (enabled != m_environment.m_scatteringEnabled)
    {
        m_environment.m_scatteringEnabled = enabled;
        invalidateShaderState();
    }
}


void
RenderContext::setScatteringParameters(const ScatteringParameters& params)
{
    m_environment.m_scattering = params;
    invalidateShaderState();
}


void
RenderContext::setSphericalGeometryHint(bool enabled)
{
    if (enabled != m_environment.m_sphericalGeometry)
    {
        m_environment.m_sphericalGeometry = enabled;
        invalidateShaderState();
    }
}


void
RenderContext::setEnvironmentMap(TextureMap* environmentMap)
{
    if (environmentMap != m_environment.m_environmentMap.ptr())
    {
        m_environment.m_environmentMap = environmentMap;
    }
}


void
RenderContext::drawBillboard(const Vector3f& position, float size)
{
    Vector3f billboardVertices[4] =
    {
        Vector3f(-0.5f, -0.5f, 0.0f),
        Vector3f( 0.5f, -0.5f, 0.0f),
        Vector3f( 0.5f,  0.5f, 0.0f),
        Vector3f(-0.5f,  0.5f, 0.0f)
    };

    Vector2f billboardTexCoords[4] =
    {
        Vector2f(0.0f, 1.0f),
        Vector2f(1.0f, 1.0f),
        Vector2f(1.0f, 0.0f),
        Vector2f(0.0f, 0.0f)
    };

    Matrix3f m = m_matrixStack[m_modelViewStackDepth].linear().transpose();

    for (int i = 0; i < 4; i++)
    {
        billboardVertices[i] = m * billboardVertices[i];
    }

    setVertexInfo(VertexSpec::PositionTex);
    updateShaderState();

    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < 4; i++)
    {
        Vector3f v = position + billboardVertices[i] * size;
        glTexCoord2fv(billboardTexCoords[i].data());
        glVertex3fv(v.data());
    }
    glEnd();
}


void
RenderContext::drawText(const Vector3f& position, const std::string& text, const TextureFont* font, const Spectrum& color)
{
    Material material;
    material.setDiffuse(color);
    material.setBlendMode(Material::AlphaBlend);
    material.setBaseTexture(font->glyphTexture());
    setVertexInfo(VertexSpec::PositionTex);
    bindMaterial(&material);
    updateShaderState();

    Vector3f origin = m_matrixStack[m_modelViewStackDepth].translation();

    // Project the text origin into normalized device coordinates
    Vector3f ndc = m_projectionStack[m_projectionStackDepth] * origin;

    // Compute the position in viewport coordinates
    Vector3f p = (ndc + Vector3f::Ones()) * 0.5f;

    pushProjection();
    orthographicProjection2D(0.0f, float(m_viewportWidth), 0.0f, float(m_viewportHeight));
    pushModelView();
    identityModelView();

    // Slight offset to keep texel centers from landing right on
    // pixel boundaries and causing poor text quality. We'll also
    // set the z position of the text so that it's hidden by
    // any objects in front of it.
    translateModelView(position + Vector3f(0.125f, 0.125f, -ndc.z()));

    font->render(text, Vector2f(std::floor(p.x() * m_viewportWidth + 0.5f), std::floor(p.y() * m_viewportHeight + 0.5f)));

    popModelView();
    popProjection();
}


void
RenderContext::drawCone(float apexAngle,
                        const Vector3f& axis,
                        const Spectrum& color,
                        float opacity,
                        unsigned int radialSubdivision, unsigned int axialSubdivision)
{
    const float twoPi = (float) (2 * PI);
    float slope = std::tan(apexAngle / 2.0f);
    float height = axis.norm();

    pushModelView();

    Quaternionf rotation = Quaternionf::Identity();
    rotation.setFromTwoVectors(Vector3f::UnitZ(), axis.normalized());
    rotateModelView(rotation);

    Material material;
    material.setEmission(color);
    material.setOpacity(opacity);
    bindMaterial(&material);
    updateShaderState();

    for (unsigned int i = 0; i < radialSubdivision; ++i)
    {
        float theta0 = (float) i / (float) radialSubdivision * twoPi;
        float theta1 = (float) (i + 1) / (float) radialSubdivision * twoPi;
        float s0 = std::sin(theta0);
        float c0 = std::cos(theta0);
        float s1 = std::sin(theta1);
        float c1 = std::cos(theta1);

        float hStep = height / axialSubdivision;

        glBegin(GL_TRIANGLE_STRIP);
        for (unsigned j = 0; j < axialSubdivision; j++)
        {
            float z0 = j * hStep;
            float z1 = (j + 1) * hStep;
            float r0 = z0 * slope;
            float r1 = z1 * slope;
            glVertex3f(r0 * c0, r0 * s0, z0);
            glVertex3f(r0 * c1, r0 * s1, z0);
            glVertex3f(r1 * c0, r1 * s0, z1);
            glVertex3f(r1 * c1, r1 * s1, z1);
        }
        glEnd();
    }

    popModelView();
}


void
RenderContext::drawCircle(float radius,
                          const Vector3f& center,
                          const Vector3f& planeNormal,
                          const Spectrum& color,
                          float opacity,
                          unsigned int subdivision)
{
    const float twoPi = (float) (2 * PI);

    pushModelView();

    translateModelView(center);
    Quaternionf rotation = Quaternionf::Identity();
    rotation.setFromTwoVectors(Vector3f::UnitZ(), planeNormal.normalized());
    rotateModelView(rotation);

    Material material;
    material.setEmission(color);
    material.setOpacity(opacity);
    bindMaterial(&material);
    updateShaderState();

    glBegin(GL_LINE_LOOP);
    for (unsigned int i = 0; i < subdivision; i++)
    {
        float theta = twoPi * i / (float) subdivision;
        glVertex3f(radius * std::cos(theta), radius * std::sin(theta), 0.0f);
    }
    glEnd();

    popModelView();
}


void
RenderContext::drawBox(const Vector3f& sideLengths, const Spectrum& color)
{
    setVertexInfo(VertexSpec::Position);

    Material material;
    material.setEmission(color);
    bindMaterial(&material);
    updateShaderState();

    Vector3f half = sideLengths * 0.5f;

    glBegin(GL_LINE_LOOP);
    glVertex3f(-half.x(), -half.y(), -half.z());
    glVertex3f( half.x(), -half.y(), -half.z());
    glVertex3f( half.x(),  half.y(), -half.z());
    glVertex3f(-half.x(),  half.y(), -half.z());
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-half.x(), -half.y(),  half.z());
    glVertex3f( half.x(), -half.y(),  half.z());
    glVertex3f( half.x(),  half.y(),  half.z());
    glVertex3f(-half.x(),  half.y(),  half.z());
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(-half.x(), -half.y(),  half.z());
    glVertex3f(-half.x(), -half.y(),  -half.z());
    glVertex3f( half.x(), -half.y(),  half.z());
    glVertex3f( half.x(), -half.y(),  -half.z());
    glVertex3f( half.x(),  half.y(),  half.z());
    glVertex3f( half.x(),  half.y(),  -half.z());
    glVertex3f(-half.x(),  half.y(),  half.z());
    glVertex3f(-half.x(),  half.y(),  -half.z());
    glEnd();
}


class PointParticleRenderer : public ParticleRenderer
{
public:
    PointParticleRenderer(RenderContext* rc, float* vertexStream, const Matrix3f& screenAlignTransform) :
        m_rc(rc),
        m_vertexStream(vertexStream),
        m_screenAlignTransform(screenAlignTransform)
    {
    }

    virtual void renderParticles(const std::vector<ParticleEmitter::Particle>& particles)
    {
        // Render particles as screen-aligned quads. There's no good alternative
        // with the fixed-function pipeline. The size of point sprites can't be
        // set per primitive without drawing single primitive batches and calling
        // glPointSize() repeatedly. The maximum point sprite size also interferes.
        // Geometry shaders might be the only way to avoid having to generate
        // four vertices per particle. Lastly, point sprites are clipped as if
        // they're points, so the sprites may abruptly appear when the center is
        // outside the view frustum.
        Vector3f quadVertices[4] =
        {
            Vector3f(-1.0f, -1.0f, 0.0f),
            Vector3f( 1.0f, -1.0f, 0.0f),
            Vector3f( 1.0f,  1.0f, 0.0f),
            Vector3f(-1.0f,  1.0f, 0.0f)
        };

        Vector2f quadTexCoords[4] =
        {
            Vector2f(0.0f, 1.0f),
            Vector2f(1.0f, 1.0f),
            Vector2f(1.0f, 0.0f),
            Vector2f(0.0f, 0.0f)
        };

        for (int i = 0; i < 4; i++)
        {
            quadVertices[i] = m_screenAlignTransform * quadVertices[i];
        }

        for (unsigned int i = 0; i < particles.size(); ++i)
        {
            const ParticleEmitter::Particle& particle = particles[i];
            for (unsigned int j = 0; j < 4; ++j)
            {
                Vector3f v = particle.position + quadVertices[j] * particle.size;
                unsigned int base = (i * 4 + j) * 10;
                m_vertexStream[base + 0] = v.x();
                m_vertexStream[base + 1] = v.y();
                m_vertexStream[base + 2] = v.z();
                m_vertexStream[base + 4] = quadTexCoords[j].x();
                m_vertexStream[base + 5] = quadTexCoords[j].y();
                m_vertexStream[base + 6] = particle.color.x();
                m_vertexStream[base + 7] = particle.color.y();
                m_vertexStream[base + 8] = particle.color.z();
                m_vertexStream[base + 9] = particle.opacity;
            }
        }

        m_rc->bindVertexArray(ParticleVertexSpec, m_vertexStream, ParticleVertexSpec.size());
        glDrawArrays(GL_QUADS, 0, particles.size() * 4);
    }

private:
    RenderContext* m_rc;
    float* m_vertexStream;
    Matrix3f m_screenAlignTransform;
};


void
RenderContext::drawParticles(ParticleEmitter* emitter, double clock)
{
    PointParticleRenderer particleRenderer(this,
                                           m_vertexStream,
                                           m_matrixStack[m_modelViewStackDepth].linear().transpose());

    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // OpenGL point sprites limited by max point size, so we're using
    // screen-aligned quads until a workaround is available.
    // glEnable(GL_POINT_SPRITE_ARB);
    // glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);

    setVertexInfo(ParticleVertexSpec);
    updateShaderState();
    emitter->generateParticles(clock, m_particleBuffer->particles, &particleRenderer);

    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    //glDisable(GL_POINT_SPRITE_ARB);
}


/** Disable the currently active shader program.
  */
void
RenderContext::unbindShader()
{
    if (m_shaderCapability != FixedFunction)
    {
        glUseProgramObjectARB(0);
    }
}