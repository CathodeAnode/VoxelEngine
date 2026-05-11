#include "gizmos.h"

#include "camera.h"
#include "profiler.h"
#include "logger.h"

glm::vec4 Gizmos::s_Color = { 1,1,1,1 };
glm::mat4 Gizmos::s_ViewProj;
GLuint Gizmos::s_VAO = 0;
GLuint Gizmos::s_StaticVBO = 0;
Shader Gizmos::s_Shader;

uint32_t Gizmos::s_LineOffset = 0;
uint32_t Gizmos::s_CubeOffset = 0;
uint32_t Gizmos::s_FrustumOffset = 0;

Gizmos::InstanceBuffer Gizmos::s_InstanceBuffer;
Gizmos::IndirectBuffer Gizmos::s_IndirectBuffer;

uint32_t Gizmos::s_InstanceCount = 0;
uint32_t Gizmos::s_CommandCount = 0;

void Gizmos::Init(uint32_t maxInstances)
{
    PROFILE_FUNCTION();

    s_Shader = Shader({
    { "gizmos.vert.glsl", GL_VERTEX_SHADER },
    { "gizmos.frag.glsl", GL_FRAGMENT_SHADER }
        });

    s_InstanceBuffer.Create(GL_ARRAY_BUFFER, maxInstances);
    s_IndirectBuffer.Create(GL_DRAW_INDIRECT_BUFFER, maxInstances);

    glGenVertexArrays(1, &s_VAO);
    glBindVertexArray(s_VAO);

    std::vector<glm::vec3> verts;

    // Unit Line
    s_LineOffset = verts.size();
    verts.push_back({ 0,0,0 });
    verts.push_back({ 1,0,0 });

    // Wire Cube (12 edges)
    s_CubeOffset = verts.size();

    glm::vec3 c[8] =
    {
        {-0.5,-0.5,-0.5},
        { 0.5,-0.5,-0.5},
        { 0.5, 0.5,-0.5},
        {-0.5, 0.5,-0.5},
        {-0.5,-0.5, 0.5},
        { 0.5,-0.5, 0.5},
        { 0.5, 0.5, 0.5},
        {-0.5, 0.5, 0.5},
    };

    int edges[24] =
    {
        0,1,1,2,2,3,3,0,
        4,5,5,6,6,7,7,4,
        0,4,1,5,2,6,3,7
    };

    for (int i = 0; i < 24; i++)
        verts.push_back(c[edges[i]]);

    // Frustum (12 edges)
    s_FrustumOffset = verts.size();

    glm::vec3 f[8] =
    {
        {-1,-1,-1},
        { 1,-1,-1},
        { 1, 1,-1},
        {-1, 1,-1},

        {-1,-1, 1},
        { 1,-1, 1},
        { 1, 1, 1},
        {-1, 1, 1},
    };

    int fedges[24] =
    {
        0,1, 1,2, 2,3, 3,0, // near
        4,5, 5,6, 6,7, 7,4, // far
        0,4, 1,5, 2,6, 3,7  // sides
    };

    for (int i = 0; i < 24; i++)
        verts.push_back(f[fedges[i]]);

    glGenBuffers(1, &s_StaticVBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_StaticVBO);
    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(glm::vec3),
        verts.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec3), (void*)0);

    // Instance attributes
    glBindBuffer(GL_ARRAY_BUFFER, s_InstanceBuffer.GetName());
    for (int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(i + 1);
        glVertexAttribPointer(i + 1, 4, GL_FLOAT, GL_FALSE,
            sizeof(GizmoInstance),
            (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(i + 1, 1);
    }

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE,
        sizeof(GizmoInstance),
        (void*)offsetof(GizmoInstance, Color));
    glVertexAttribDivisor(5, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Gizmos::Shutdown()
{
    PROFILE_FUNCTION();

    if (s_StaticVBO)
    {
        glDeleteBuffers(1, &s_StaticVBO);
        s_StaticVBO = 0;
    }

    if (s_VAO)
    {
        glDeleteVertexArrays(1, &s_VAO);
        s_VAO = 0;
    }

    s_InstanceBuffer.Destroy();
    s_IndirectBuffer.Destroy();

    s_InstanceCount = 0;
    s_CommandCount = 0;
}

void Gizmos::Begin(const Camera& camera)
{
    PROFILE_FUNCTION();

    s_ViewProj = camera.GetProjMatrix() * camera.GetViewMatrix();

    s_InstanceCount = 0;
    s_CommandCount = 0;
}

void Gizmos::End()
{
    PROFILE_FUNCTION();

    _Flush();
}

void Gizmos::SetColor(const glm::vec4& color)
{
    s_Color = color;
}

void Gizmos::DrawLine(const glm::vec3& p0, const glm::vec3& p1)
{
    PROFILE_FUNCTION();

    glm::vec3 dir = p1 - p0;
    float len = glm::length(dir);
    if (len <= 0.00001f) return;

    glm::vec3 n = glm::normalize(dir);

    glm::vec3 base = glm::vec3(1, 0, 0);

    float cosTheta = glm::dot(base, n);
    glm::vec3 axis = glm::cross(base, n);

    glm::mat4 model(1.0f);
    model = glm::translate(model, p0);

    if (glm::length(axis) > 0.00001f)
    {
        axis = glm::normalize(axis);
        float angle = acos(cosTheta);
        model = glm::rotate(model, angle, axis);
    }

    model = glm::scale(model, glm::vec3(len, 1, 1));

    _SubmitLine(model);
}

void Gizmos::DrawCube(const glm::vec3& center, const glm::vec3& size)
{
    PROFILE_FUNCTION();

    glm::mat4 model(1.f);
    model = glm::translate(model, center);
    model = glm::scale(model, size);

    _SubmitCube(model);
}

void Gizmos::DrawFrustum(const Camera& camera)
{
    PROFILE_FUNCTION();

    glm::mat4 invVP = glm::inverse(camera.GetProjMatrix() * camera.GetViewMatrix());
    _SubmitFrustum(invVP);
}

void Gizmos::_SubmitLine(const glm::mat4& model)
{
    GizmoInstance* instances = s_InstanceBuffer.GetCurrentContents();
    instances[s_InstanceCount].Model = model;
    instances[s_InstanceCount].Color = s_Color;

    DrawArraysIndirectCommand* commands = s_IndirectBuffer.GetCurrentContents();

    commands[s_CommandCount].count = 2;
    commands[s_CommandCount].instanceCount = 1;
    commands[s_CommandCount].first = s_LineOffset;
    commands[s_CommandCount].baseInstance = s_InstanceCount;

    s_InstanceCount++;
    s_CommandCount++;
}

void Gizmos::_SubmitCube(const glm::mat4& model)
{
    GizmoInstance* instances = s_InstanceBuffer.GetCurrentContents();
    instances[s_InstanceCount].Model = model;
    instances[s_InstanceCount].Color = s_Color;

    DrawArraysIndirectCommand* commands = s_IndirectBuffer.GetCurrentContents();

    commands[s_CommandCount].count = 24; // 12 edges
    commands[s_CommandCount].instanceCount = 1;
    commands[s_CommandCount].first = s_CubeOffset;
    commands[s_CommandCount].baseInstance = s_InstanceCount;

    s_InstanceCount++;
    s_CommandCount++;
}

void Gizmos::_SubmitFrustum(const glm::mat4& model)
{
    GizmoInstance* instances = s_InstanceBuffer.GetCurrentContents();
    instances[s_InstanceCount].Model = model;
    instances[s_InstanceCount].Color = s_Color;

    DrawArraysIndirectCommand* commands =
        s_IndirectBuffer.GetCurrentContents();

    commands[s_CommandCount].count = 24;
    commands[s_CommandCount].instanceCount = 1;
    commands[s_CommandCount].first = s_FrustumOffset;
    commands[s_CommandCount].baseInstance = s_InstanceCount;

    s_InstanceCount++;
    s_CommandCount++;
}

void Gizmos::_Flush()
{
    if (s_CommandCount == 0)
        return;

    s_Shader.Use();

    s_Shader.SetMat4("uViewProj", s_ViewProj);
    s_Shader.SetBool("uAlwaysOnTop", false);

    glBindVertexArray(s_VAO);

    glBindVertexArray(s_VAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_IndirectBuffer.GetName());

    glMultiDrawArraysIndirect(
        GL_LINES,
        s_IndirectBuffer.GetPreviousFrameOffset(),
        s_CommandCount,
        0);

    s_InstanceBuffer.Commit();
    s_IndirectBuffer.Commit();

    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}
