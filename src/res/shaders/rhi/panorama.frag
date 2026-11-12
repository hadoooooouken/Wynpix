#version 440

// Equirectangular panorama seen from its centre: port of the widget viewer's
// res/shaders/panorama.frag (PanoramaGraphicsItem), same ray construction,
// rotation order and texture mapping.
//
// Differences from the widget shader:
// - Images larger than the texture size limit are drawn per tile: every tile
//   covers the whole colour buffer and keeps the rays that hit its core.
// - The texture is sampled with explicit gradients whose longitude part is
//   corrected for the wrap at the panorama's back seam, so trilinear
//   sampling uses the mip level of the footprint instead of the smallest one
//   along that seam.
// - With trilinear sampling the texture is read with anisotropic filtering:
//   several taps along the major axis of the pixel's footprint ellipse, each
//   with the mip level of the minor axis, so surfaces seen at a grazing angle
//   keep their detail. The ellipse axes are the singular vectors of the
//   screen-to-texture Jacobian, so they stay correct where the two screen
//   gradients are nearly parallel (near the zenith and the nadir). The mip
//   level is biased towards the sharper level by kLodBias; the bias changes
//   the mip level only, not the area the taps cover.
// - Texels are premultiplied; the colour matrix applies to straight colour
//   like in image.frag, and the result is blended over the background.

layout(location = 0) in vec2 screenPos;

layout(location = 0) out vec4 fragColor;

// Mirrored by PanoramaUniforms in gui/quick/render/rhipassuniforms.h;
// identical in panorama.vert.
layout(std140, binding = 0) uniform PanoramaParams {
    mat4 mvp;
    // Rows of the colour adjustment matrix (xyz) applied to straight colour.
    vec4 colorRow0;
    vec4 colorRow1;
    vec4 colorRow2;
    // Tile core in normalized image coordinates: left, top, right, bottom
    // (half-open; edges of the image lie outside [0, 1]).
    vec4 core;
    // Tile texture in normalized image coordinates: x, y, width, height.
    vec4 texRect;
    vec2 targetSize;
    float tanHalfFov;
    float aspect;
    float sinYaw;
    float cosYaw;
    float sinPitch;
    float cosPitch;
    float colorOffset;
    int colorEnabled;
    // The texture spans the whole image width and its sampler repeats: no
    // horizontal core test.
    int wrapsHorizontally;
    // Vibrance in [-1, 1] applied after the colour matrix.
    float vibrance;
    // Taps along the footprint's major axis at most; 1 samples once (nearest
    // sampling). Padded to 16 bytes by std140.
    float maxAnisotropy;
};

layout(binding = 1) uniform sampler2D imageTexture;

const float kPi = 3.14159265358979323846;
// Below this alpha the pixel is not un-premultiplied for the colour matrix.
const float kMinUnpremultiplyAlpha = 0.0001;
// Rec.709 luma weights and the vibrance thresholds of image.frag.
const vec3 kLuma = vec3(0.2126, 0.7152, 0.0722);
const float kVibranceEpsilon = 0.001;
// Vibrance curve constants, mirrors of utils/coloradjustments.h.
const float kVibrancePosBoost = 0.28;
const float kVibrancePosFalloff = 0.28;
const float kVibrancePosFalloffStart = 0.6;
const float kVibranceNegBase = 0.5;
const float kVibranceNegSlope = 0.25;
const float kVibranceMinPeak = 1e-4;
// Longitude differences of neighbouring pixels larger than half the image
// width are the wrap at the seam, not a footprint.
const float kHalfTurn = 0.5;
// Loop bound of the anisotropic filter; maxAnisotropy never exceeds it.
const int kMaxAnisotropyTaps = 16;
// Mip level bias in levels; negative values pick sharper levels.
const float kLodBias = -1.0;
// Smallest minor footprint axis in texels, guards the axis ratio.
const float kMinFootprint = 1e-6;
const float kHalf = 0.5;

// Mirror of applyColorAdjustmentVibrance() in utils/coloradjustments.h.
vec3 applyVibrance(vec3 color, float amount)
{
    float peak = max(color.r, max(color.g, color.b));
    float low = min(color.r, min(color.g, color.b));
    float chroma = peak - low;
    if (amount >= 0.0) {
        // Positive: scale HSV saturation around the peak channel.
        float sat = chroma / max(peak, kVibranceMinPeak);
        float t = clamp((sat - kVibrancePosFalloffStart) / (1.0 - kVibrancePosFalloffStart),
                        0.0, 1.0);
        float factor = 1.0 + amount * (kVibrancePosBoost - kVibrancePosFalloff * t);
        return clamp(vec3(peak) + (color - vec3(peak)) * factor, 0.0, 1.0);
    }
    float luma = dot(color, kLuma);
    float weight = kVibranceNegBase + kVibranceNegSlope * chroma;
    float factor = max(0.0, 1.0 + amount * weight);
    return clamp(vec3(luma) + (color - vec3(luma)) * factor, 0.0, 1.0);
}

// Reads the texture with the footprint given by the gradients in texture
// coordinates. With maxAnisotropy > 1 the footprint is covered by several
// taps along its longer axis (anisotropic filtering, which QRhiSampler does
// not offer); the hardware chooses the mip level of each tap from the
// longest gradient, which is the minor axis after the split.
vec4 sampleFootprint(vec2 tc, vec2 gradX, vec2 gradY)
{
    if (maxAnisotropy <= 1.0)
        return textureGrad(imageTexture, tc, gradX, gradY);

    // The pixel's footprint is the ellipse that the screen-to-texture
    // Jacobian J = [gradX gradY] (in texels) makes of a unit circle. Its
    // axes are the singular values and left singular vectors of J, which
    // are the eigen-decomposition of the symmetric matrix M = J * J^T.
    vec2 texSize = vec2(textureSize(imageTexture, 0));
    vec2 texelX = gradX * texSize;
    vec2 texelY = gradY * texSize;
    float mxx = dot(vec2(texelX.x, texelY.x), vec2(texelX.x, texelY.x));
    float myy = dot(vec2(texelX.y, texelY.y), vec2(texelX.y, texelY.y));
    float mxy = texelX.x * texelX.y + texelY.x * texelY.y;
    float mean = (mxx + myy) * kHalf;
    float spread = sqrt((mxx - myy) * (mxx - myy) * kHalf * kHalf + mxy * mxy);
    float majorLen = sqrt(mean + spread);
    float minorLen = max(sqrt(max(mean - spread, 0.0)), kMinFootprint);
    // Direction of the major axis; a round footprint has no preferred one.
    float angle = spread > 0.0 ? kHalf * atan(2.0 * mxy, mxx - myy) : 0.0;
    vec2 majorDir = vec2(cos(angle), sin(angle));
    vec2 minorDir = vec2(-majorDir.y, majorDir.x);

    // Axes as texture coordinate offsets, spanning the whole footprint.
    vec2 majorGrad = majorDir * majorLen / texSize;
    vec2 minorGrad = minorDir * minorLen / texSize;
    float ratio = clamp(majorLen / minorLen, 1.0, maxAnisotropy);
    int taps = min(int(ceil(ratio)), kMaxAnisotropyTaps);
    float invTaps = 1.0 / float(taps);

    // Each tap covers 1 / taps of the major axis. Scaling the gradients by
    // 2^bias shifts the selected mip level by bias without moving the taps.
    float lodScale = exp2(kLodBias);
    vec2 tapGrad = majorGrad * invTaps;
    vec2 mipTapGrad = tapGrad * lodScale;
    vec2 mipMinorGrad = minorGrad * lodScale;

    // Tap centres are spread evenly over the major axis around tc.
    vec2 tapTc = tc - majorGrad * (kHalf - kHalf * invTaps);
    vec4 sum = vec4(0.0);
    for (int i = 0; i < kMaxAnisotropyTaps; ++i) {
        if (i >= taps)
            break;
        sum += textureGrad(imageTexture, tapTc, mipTapGrad, mipMinorGrad);
        tapTc += tapGrad;
    }
    return sum * invTaps;
}

void main()
{
    // Screen position in [-1, 1], y down (the widget's texCoord * 2 - 1).
    vec2 sc = screenPos * 2.0 - 1.0;

    // Ray direction in camera space.
    vec3 ray = normalize(vec3(sc.x * aspect * tanHalfFov,
                              -sc.y * tanHalfFov,
                              1.0));

    // Pitch around the X axis.
    vec3 r = ray;
    ray.y = r.y * cosPitch - r.z * sinPitch;
    ray.z = r.y * sinPitch + r.z * cosPitch;

    // Yaw around the Y axis.
    r = ray;
    ray.x = r.x * cosYaw + r.z * sinYaw;
    ray.z = -r.x * sinYaw + r.z * cosYaw;

    // Spherical (equirectangular) coordinates in [0, 1].
    float lon = atan(ray.x, ray.z);
    float lat = asin(clamp(ray.y, -1.0, 1.0));
    vec2 uv = vec2(0.5 + lon / (2.0 * kPi), 0.5 - lat / kPi);

    // Footprint, computed before any fragment is discarded.
    vec2 dx = dFdx(uv);
    vec2 dy = dFdy(uv);
    if (abs(dx.x) > kHalfTurn)
        dx.x -= sign(dx.x);
    if (abs(dy.x) > kHalfTurn)
        dy.x -= sign(dy.x);

    bool outsideCore = uv.y < core.y || uv.y >= core.w;
    if (wrapsHorizontally == 0)
        outsideCore = outsideCore || uv.x < core.x || uv.x >= core.z;
    if (outsideCore)
        discard;

    vec2 tc = (uv - texRect.xy) / texRect.zw;
    vec4 color = sampleFootprint(tc, dx / texRect.zw, dy / texRect.zw);
    // The matrix (colorEnabled) and vibrance switch independently.
    bool vibranceActive = abs(vibrance) > kVibranceEpsilon;
    if (colorEnabled != 0 || vibranceActive) {
        float a = color.a;
        vec3 straight = (a > kMinUnpremultiplyAlpha) ? color.rgb / a
                                                     : color.rgb;
        if (colorEnabled != 0) {
            straight = clamp(vec3(dot(colorRow0.xyz, straight),
                                  dot(colorRow1.xyz, straight),
                                  dot(colorRow2.xyz, straight)) + vec3(colorOffset),
                             0.0, 1.0);
        } else {
            // Vibrance alone: the same [0, 1] input the matrix path gives it.
            straight = clamp(straight, 0.0, 1.0);
        }
        if (vibranceActive)
            straight = applyVibrance(straight, vibrance);
        color.rgb = straight * a;
    }
    fragColor = color;
}
