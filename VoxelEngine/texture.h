#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>

#include <stb/stb_image.h>

class Texture {
public:
    Texture(std::string m_Name);

    // initialize with image path and type
    Texture(std::string dir, std::string path);
    // generate texture id
    void generate();

    // load texture from path
    void load(bool flip = true);

    void allocate(GLenum format, GLuint width, GLuint height, GLenum type);

    static void setParams(GLenum texMinFilter = GL_NEAREST,
        GLenum texMagFilter = GL_NEAREST,
        GLenum wrapS = GL_REPEAT,
        GLenum wrapT = GL_REPEAT);

    // bind texture id
    void bind();

    void cleanup();

    /*
        texture object values
    */

    // texture id
    unsigned int m_Id;
    // name
    std::string m_Name;
    // directory of image
    std::string dir;
    // name of image
    std::string path;
};

#endif