#ifndef GIZMOS_H
#define GIZMOS_H

#include <glm/fwd.hpp>

#include "gpu_buffer_allocator.h"
#include "shader.h"
#include "types.h"

class Camera;

using GLuint = unsigned int;

struct GizmoInstance
{
    glm::mat4 Model;
    glm::vec4 Color;
};


class Gizmos
{
public:
    static void Init(uint32_t maxInstances = 20000);
    static void Shutdown();

    static void Begin(const Camera& camera);
    static void End();

    static void SetColor(const glm::vec4& color);

    static void DrawLine(const glm::vec3& p0, const glm::vec3& p1);

    static void DrawCube(const glm::vec3& center, const glm::vec3& size);

    static void DrawFrustum(const Camera& camera);

    static void DrawTransform(const glm::mat4& transform);

private:
    static void _SubmitLine(const glm::mat4& model);
    static void _SubmitCube(const glm::mat4& model);
    static void _SubmitFrustum(const glm::mat4& model);

    static void _Flush();

private:
    static glm::vec4 s_Color;
    static glm::mat4 s_ViewProj;

    static GLuint s_VAO;
    static GLuint s_StaticVBO;
    static Shader s_Shader;

    static uint32_t s_LineOffset;
    static uint32_t s_CubeOffset;
    static uint32_t s_FrustumOffset;

    using InstanceBuffer = GPUOrphanBuffer<GizmoInstance, 3>;
    using IndirectBuffer = GPUOrphanBuffer<DrawArraysIndirectCommand, 3>;

    static InstanceBuffer s_InstanceBuffer;
    static IndirectBuffer s_IndirectBuffer;

    static uint32_t s_InstanceCount;
    static uint32_t s_CommandCount;
};

#endif