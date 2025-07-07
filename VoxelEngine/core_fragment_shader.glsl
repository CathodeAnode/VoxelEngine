#version 460 core

out vec4 Fragcolor;
in vec2 TextCoord;
flat in uint face;
//in uint Type;


void main() {


    if(face == 0u) {
        // +z face
        Fragcolor = vec4(1.0, 0.0, 0.0, 1.0f);
    }
    else if(face == 1u) {
        // -z face
        Fragcolor = vec4(1.0, 1.0, 0.0, 1.0f);
    }
    else if(face == 2u) {
        // +x face
        Fragcolor = vec4(0.0, 1.0, 0.0, 1.0f);
    }
    else if(face == 3u) {
        // -x face
        Fragcolor = vec4(0.0, 1.0, 1.0, 1.0f);
    }
    else if(face == 4u) {
        // +y face
        Fragcolor = vec4(0.0, 0.0, 1.0, 1.0f);
    }
    else if(face == 5u) {
        // -y face
        Fragcolor = vec4(0.0, 0.0, 0.0, 1.0f);
    }
    else {
	    Fragcolor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

}