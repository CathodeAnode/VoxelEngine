#include "gizmos.h"

#include "camera.h"

glm::vec4 Gizmos::s_Color = { 1,1,1,1 };
glm::mat4 Gizmos::s_ViewProj;
GLuint Gizmos::s_VAO = 0;
GLuint Gizmos::s_StaticVBO = 0;
Shader Gizmos::s_Shader;

uint32_t Gizmos::s_LineOffset = 0;
uint32_t Gizmos::s_CubeOffset = 0;

Gizmos::InstanceBuffer Gizmos::s_InstanceBuffer;
Gizmos::IndirectBuffer Gizmos::s_IndirectBuffer;

uint32_t Gizmos::s_InstanceCount = 0;
uint32_t Gizmos::s_CommandCount = 0;

void Gizmos::Init(uint32_t maxInstances)
{
    s_Shader = Shader({
    { "gizmos.vert", GL_VERTEX_SHADER },
    { "gizmos.frag", GL_FRAGMENT_SHADER }
        });

    s_InstanceBuffer.Create(GL_ARRAY_BUFFER, maxInstances, 3);
    s_IndirectBuffer.Create(GL_DRAW_INDIRECT_BUFFER, maxInstances, 3);

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
    s_ViewProj = camera.GetProjMatrix() * camera.GetViewMatrix();

    s_InstanceBuffer.AdvanceHead();
    s_IndirectBuffer.AdvanceHead();

    s_InstanceCount = 0;
    s_CommandCount = 0;
}

void Gizmos::End()
{
    _Flush();
}

void Gizmos::SetColor(const glm::vec4& color)
{
    s_Color = color;
}

void Gizmos::DrawLine(const glm::vec3& p0,
    const glm::vec3& p1)
{
    glm::vec3 dir = p1 - p0;
    float len = glm::length(dir);

    glm::mat4 model(1.f);
    model = glm::translate(model, p0);
    model = glm::scale(model, { len,1,1 });

   _SubmitLine(model);
}

void Gizmos::DrawCube(const glm::vec3& center, const glm::vec3& size)
{
    glm::mat4 model(1.f);
    model = glm::translate(model, center);
    model = glm::scale(model, size);

    _SubmitCube(model);
}

void Gizmos::_SubmitLine(const glm::mat4& model)
{
    GizmoInstance* instances = s_InstanceBuffer.GetHeadContents();
    instances[s_InstanceCount].Model = model;
    instances[s_InstanceCount].Color = s_Color;

    DrawArraysIndirectCommand* commands = s_IndirectBuffer.GetHeadContents();

    commands[s_CommandCount].count = 2;
    commands[s_CommandCount].instanceCount = 1;
    commands[s_CommandCount].first = s_LineOffset;
    commands[s_CommandCount].baseInstance = s_InstanceCount;

    s_InstanceCount++;
    s_CommandCount++;
}

void Gizmos::_SubmitCube(const glm::mat4& model)
{
    GizmoInstance* instances = s_InstanceBuffer.GetHeadContents();
    instances[s_InstanceCount].Model = model;
    instances[s_InstanceCount].Color = s_Color;

    DrawArraysIndirectCommand* commands = s_IndirectBuffer.GetHeadContents();

    commands[s_CommandCount].count = 24; // 12 edges
    commands[s_CommandCount].instanceCount = 1;
    commands[s_CommandCount].first = s_CubeOffset;
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
        s_IndirectBuffer.GetHeadOffset(),
        s_CommandCount,
        0);

    s_InstanceBuffer.AdvanceTail();
    s_IndirectBuffer.AdvanceTail();

    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}
