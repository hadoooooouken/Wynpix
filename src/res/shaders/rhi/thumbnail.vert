#version 440

// One thumbnail of the Qt Quick thumbnail views (ThumbnailItem), drawn as a
// textured quad over the painted rectangle in the item's coordinates.
//
// Every corner is moved to the nearest device pixel boundary of the viewport,
// like QPainter's translate-only pixmap blits in the widget ThumbnailWidget.
// A centred grid, a scroll position or a fractional device pixel ratio can
// place the quad between pixels, and the linear sampling would then blur
// a thumbnail that is otherwise drawn one texel per device pixel.

layout(location = 0) in vec4 qt_VertexPosition;
layout(location = 1) in vec2 qt_VertexTexCoord;

layout(location = 0) out vec2 texCoord;

// Mirrored by ThumbnailUniforms in gui/quick/render/thumbnailmaterial.cpp;
// identical in thumbnail.frag.
layout(std140, binding = 0) uniform ThumbnailParams {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 rectSize;
    float cornerRadius;
    float highlight;
    float edgeWidth;
    // Size of the viewport in device pixels; empty to draw unsnapped.
    vec2 viewportSize;
};

void main()
{
    texCoord = qt_VertexTexCoord;
    vec4 position = qt_Matrix * qt_VertexPosition;
    if (viewportSize.x > 0.0 && viewportSize.y > 0.0) {
        vec2 halfViewport = viewportSize * 0.5;
        vec2 pixel = (position.xy / position.w + 1.0) * halfViewport;
        vec2 snapped = floor(pixel + 0.5);
        position.xy += (snapped - pixel) / halfViewport * position.w;
    }
    gl_Position = position;
}
