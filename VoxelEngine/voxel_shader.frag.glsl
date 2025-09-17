#version 460 core

out vec4 Fragcolor;
flat in uint face;
flat in uint color;


void main() 
{
    float r = float((color >> 24) & 0xFFu) / 255.0;
    float g = float((color >> 16) & 0xFFu) / 255.0;
    float b = float((color >> 8) & 0xFFu) / 255.0;
    float a = float(color & 0xFFu) / 255.0;

    Fragcolor = vec4(r, g, b, a);

    //if(face == 0u) {
    //    // +z face
    //    Fragcolor = vec4(1.0, 0.0, 0.0, 1.0f);
    //}
    //else if(face == 1u) {
    //    // -z face
    //    Fragcolor = vec4(1.0, 1.0, 0.0, 1.0f);
    //}
    //else if(face == 2u) {
    //    // +x face
    //    Fragcolor = vec4(0.0, 1.0, 0.0, 1.0f);
    //}
    //else if(face == 3u) {
    //    // -x face
    //    Fragcolor = vec4(0.0, 1.0, 1.0, 1.0f);
    //}
    //else if(face == 4u) {
    //    // +y face
    //    Fragcolor = vec4(0.0, 0.0, 1.0, 1.0f);
    //}
    //else if(face == 5u) {
    //    // -y face
    //    Fragcolor = vec4(0.0, 0.0, 0.0, 1.0f);
    //}
    //else {
	//    Fragcolor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    //}

}