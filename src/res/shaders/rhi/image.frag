#version 440

// Samples one image tile (premultiplied alpha), applies sharpening and the
// colour adjustment matrix, and optionally composites the result over the
// transparency checkerboard. The output is premultiplied and blended with
// ONE, ONE_MINUS_SRC_ALPHA over the cleared background.
//
// The sharpening and colour code is a port of the widget viewer's
// res/shaders/filter.frag (FilterPixmapItem) and keeps its constants.

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 devicePos;

layout(location = 0) out vec4 fragColor;

// Mirrored by TileUniforms in gui/quick/render/rhipassuniforms.h; identical
// in image.vert.
layout(std140, binding = 0) uniform TileParams {
    mat4 mvp;
    vec4 targetRect;
    vec4 texRect;
    vec4 checkerLight;
    vec4 checkerDark;
    // Rows of the colour adjustment matrix (xyz) applied to straight colour.
    vec4 colorRow0;
    vec4 colorRow1;
    vec4 colorRow2;
    vec2 checkerOrigin;
    // One device pixel in texture coordinates of the bound texture.
    vec2 texelStep;
    float checkerTile;
    float checkerFirstCell;
    float colorOffset;
    float casSharpening;
    float casContrast;
    int checkerEnabled;
    int colorEnabled;
    // kSharpenNone / kSharpenCas / kSharpenSmart.
    int sharpenMode;
    // Non-zero when the bound texture is the full-resolution mip chain shown
    // below 1:1: the sharpening taps are widened and sampled from a sharper
    // mip level.
    int downscaleTaps;
    float smartEnhance;
    int smartEnhanceEnabled;
    vec4 wbScale;
    vec4 wbBias;
};

layout(binding = 1) uniform sampler2D imageTexture;

// Values of RenderEnums::Sharpening.
const int kSharpenNone = 0;
const int kSharpenCas = 1;
const int kSharpenSmart = 2;

// Rec.709 luma weights. CAS and smart sharpening derive one scalar weight
// from luma and apply it to all three channels, which preserves hue on
// saturated edges.
const vec3 kLuma = vec3(0.2126, 0.7152, 0.0722);
// Floor for luma values that are divided by or fed into inversesqrt().
const float kMinLuma = 1e-5;
// Sharpening only applies to fully opaque pixels: on soft edges the taps
// would mix premultiplied neighbours of different alpha.
const float kOpaqueAlphaThreshold = 0.9999;
// Below this alpha the pixel is not un-premultiplied for the colour matrix.
const float kMinUnpremultiplyAlpha = 0.0001;
// CAS strengths below this do nothing (ColorAdjustments epsilon).
const float kSharpenEpsilon = 0.001;

// Downscale taps: inner and outer tap distance in device pixels and the LOD
// bias towards the next sharper mip level.
const float kDownscaleInnerTap = 1.2;
const float kDownscaleOuterTap = 2.8;
const float kDownscaleLodBias = -0.7;

// CAS: peak = kCasPeakContrastSlope * contrast + kCasPeakBase.
const float kCasPeakContrastSlope = -3.0;
const float kCasPeakBase = 8.0;
const float kCasWindowTaps = 4.0;
// The min/max sums of the cross and the full window span [0, 2].
const float kCasRangeSum = 2.0;

// Smart sharpening, downscale taps: 9-tap blur weights and unsharp amount.
const float kSmartBlurCentre = 0.17;
const float kSmartBlurInner = 0.14;
const float kSmartBlurOuter = 0.0675;
const float kSmartDownscaleAmount = 0.18;
// Smart sharpening, plain taps: Laplacian weight and amount.
const float kSmartLaplacianCentre = 4.0;
const float kSmartLaplacianAmount = 0.0625;

vec3 tap(vec2 uv, float bias)
{
    return texture(imageTexture, uv, bias).rgb;
}

vec3 casFromWindow(vec3 a, vec3 b, vec3 c, vec3 d, vec3 e, vec3 f, vec3 g,
                   vec3 h, vec3 i)
{
    vec3 mnRGB = min(min(min(d, e), min(f, b)), h);
    vec3 mnRGB2 = min(mnRGB, min(min(a, c), min(g, i)));
    mnRGB += mnRGB2;

    vec3 mxRGB = max(max(max(d, e), max(f, b)), h);
    vec3 mxRGB2 = max(mxRGB, max(max(a, c), max(g, i)));
    mxRGB += mxRGB2;

    float mnL = dot(mnRGB, kLuma);
    float mxL = dot(mxRGB, kLuma);
    float rcpM = 1.0 / max(mxL, kMinLuma);
    float amp = clamp(min(mnL, kCasRangeSum - mxL) * rcpM, 0.0, 1.0);
    amp = inversesqrt(max(amp, kMinLuma));

    float peak = kCasPeakContrastSlope * casContrast + kCasPeakBase;
    float w = -1.0 / (amp * peak);
    float rcpWeight = 1.0 / (kCasWindowTaps * w + 1.0);

    vec3 window = (b + d) + (f + h);
    vec3 outColor = clamp((window * w + e) * rcpWeight, 0.0, 1.0);
    return mix(e, outColor, casSharpening);
}

vec3 applyCas(vec2 uv)
{
    vec2 offX = vec2(texelStep.x, 0.0);
    vec2 offY = vec2(0.0, texelStep.y);
    if (downscaleTaps != 0) {
        float bias = kDownscaleLodBias;
        vec2 inX = kDownscaleInnerTap * offX;
        vec2 inY = kDownscaleInnerTap * offY;
        vec2 outX = kDownscaleOuterTap * offX;
        vec2 outY = kDownscaleOuterTap * offY;
        return casFromWindow(tap(uv - outX - outY, bias), tap(uv - inY, bias),
                             tap(uv + outX - outY, bias), tap(uv - inX, bias),
                             tap(uv, bias), tap(uv + inX, bias),
                             tap(uv - outX + outY, bias), tap(uv + inY, bias),
                             tap(uv + outX + outY, bias));
    }
    return casFromWindow(tap(uv - offX - offY, 0.0), tap(uv - offY, 0.0),
                         tap(uv + offX - offY, 0.0), tap(uv - offX, 0.0),
                         tap(uv, 0.0), tap(uv + offX, 0.0),
                         tap(uv - offX + offY, 0.0), tap(uv + offY, 0.0),
                         tap(uv + offX + offY, 0.0));
}

vec3 applySmart(vec2 uv)
{
    vec2 offX = vec2(texelStep.x, 0.0);
    vec2 offY = vec2(0.0, texelStep.y);
    if (downscaleTaps != 0) {
        float bias = kDownscaleLodBias;
        vec3 center = tap(uv, bias);
        float lc = dot(center, kLuma);
        float inner = dot(tap(uv - kDownscaleInnerTap * offY, bias), kLuma)
                    + dot(tap(uv + kDownscaleInnerTap * offY, bias), kLuma)
                    + dot(tap(uv - kDownscaleInnerTap * offX, bias), kLuma)
                    + dot(tap(uv + kDownscaleInnerTap * offX, bias), kLuma);
        float outer = dot(tap(uv - kDownscaleOuterTap * offY, bias), kLuma)
                    + dot(tap(uv + kDownscaleOuterTap * offY, bias), kLuma)
                    + dot(tap(uv - kDownscaleOuterTap * offX, bias), kLuma)
                    + dot(tap(uv + kDownscaleOuterTap * offX, bias), kLuma);
        float blurredL = lc * kSmartBlurCentre + inner * kSmartBlurInner
                       + outer * kSmartBlurOuter;
        float deltaL = kSmartDownscaleAmount * (lc - blurredL);
        return clamp(center + deltaL, 0.0, 1.0);
    }
    vec3 c = tap(uv, 0.0);
    float lap = kSmartLaplacianCentre * dot(c, kLuma)
              - dot(tap(uv - offY, 0.0), kLuma)
              - dot(tap(uv + offY, 0.0), kLuma)
              - dot(tap(uv - offX, 0.0), kLuma)
              - dot(tap(uv + offX, 0.0), kLuma);
    return clamp(c + lap * kSmartLaplacianAmount, 0.0, 1.0);
}

vec3 overlayBlend(vec3 src, vec3 blend)
{
    return mix(2.0 * src * blend,
               vec3(1.0) - 2.0 * (vec3(1.0) - src) * (vec3(1.0) - blend),
               step(vec3(0.5), src));
}

vec3 sampleLocalBlur(vec2 uv, vec3 fallbackColor)
{
    vec2 off = texelStep * 12.0;
    vec4 acc = texture(imageTexture, uv) * 0.28;
    acc += (texture(imageTexture, uv + vec2(-off.x, 0.0)) +
            texture(imageTexture, uv + vec2( off.x, 0.0)) +
            texture(imageTexture, uv + vec2(0.0, -off.y)) +
            texture(imageTexture, uv + vec2(0.0,  off.y))) * 0.12;
    acc += (texture(imageTexture, uv + vec2(-off.x, -off.y)) +
            texture(imageTexture, uv + vec2( off.x, -off.y)) +
            texture(imageTexture, uv + vec2(-off.x,  off.y)) +
            texture(imageTexture, uv + vec2( off.x,  off.y))) * 0.06;
    return (acc.a > kMinUnpremultiplyAlpha) ? clamp(acc.rgb / acc.a, 0.0, 1.0) : fallbackColor;
}

vec3 applySmartEnhance(vec3 color, vec2 uv)
{
    // 1. Auto White Balance: per-channel histogram stretch (0.05% cutoff).
    vec3 wbColor = clamp(color * wbScale.xyz + wbBias.xyz, 0.0, 1.0);

    // 2. Spatial luminance blur approximation (wide radius Gaussian blur).
    vec3 blurColor = sampleLocalBlur(uv, color);
    vec3 wbBlurColor = clamp(blurColor * wbScale.xyz + wbBias.xyz, 0.0, 1.0);
    float blurLum = dot(wbBlurColor, kLuma);

    // 3. Shadows recovery: power curve when blurLum < 0.392.
    const float kShadowCutoff = 0.392;
    float shadowWeight = 0.0;
    if (blurLum < kShadowCutoff) {
        float t = blurLum / kShadowCutoff;
        shadowWeight = 1.0 - t * t;
    }
    vec3 shadowOverlay = overlayBlend(wbColor, vec3(1.0) - wbBlurColor);

    // 4. Highlights recovery: power curve when blurLum > 0.608.
    const float kHighlightCutoff = 0.608;
    float highlightWeight = 0.0;
    if (blurLum > kHighlightCutoff) {
        float t = (1.0 - blurLum) / (1.0 - kHighlightCutoff);
        highlightWeight = 1.0 - t * t;
    }
    vec3 highlightOverlay = overlayBlend(wbColor, wbBlurColor);

    // Adaptive amounts for shadow lift and highlight compression.
    const float kShadowAmount = 0.25;
    const float kHighlightAmount = 0.25;

    vec3 adjusted = wbColor;
    adjusted = mix(adjusted, shadowOverlay, kShadowAmount * shadowWeight);
    adjusted = mix(adjusted, highlightOverlay, kHighlightAmount * highlightWeight);

    // 5. Midtone clarity and S-curve boost.
    vec3 midDist = adjusted - vec3(0.5);
    vec3 midtoneBoost = adjusted + midDist * (vec3(1.0) - 4.0 * midDist * midDist) * 0.10;
    return clamp(midtoneBoost, 0.0, 1.0);
}

void main()
{
    vec4 color = texture(imageTexture, texCoord);
    vec3 rgb = color.rgb;
    float a = color.a;

    // The sharpened colour is computed in uniform control flow (sharpenMode
    // is uniform) so that the implicit derivatives of its taps are defined;
    // only the per-pixel opacity test selects the result.
    vec3 sharpened = rgb;
    if (sharpenMode == kSharpenCas && casSharpening > kSharpenEpsilon)
        sharpened = applyCas(texCoord);
    else if (sharpenMode == kSharpenSmart)
        sharpened = applySmart(texCoord);
    if (a >= kOpaqueAlphaThreshold)
        rgb = sharpened;

    // Smart Enhance adaptive local tone mapping on straight colour.
    if (smartEnhanceEnabled != 0 && smartEnhance > kSharpenEpsilon) {
        vec3 straight = (a > kMinUnpremultiplyAlpha) ? rgb / a : rgb;
        vec3 enhanced = applySmartEnhance(straight, texCoord);
        straight = mix(straight, enhanced, clamp(smartEnhance, 0.0, 1.0));
        rgb = straight * a;
    }

    // The adjustments are defined on straight colour: un-premultiply, apply,
    // premultiply again.
    if (colorEnabled != 0) {
        vec3 straight = (a > kMinUnpremultiplyAlpha) ? rgb / a : rgb;
        straight = clamp(vec3(dot(colorRow0.xyz, straight),
                              dot(colorRow1.xyz, straight),
                              dot(colorRow2.xyz, straight)) + vec3(colorOffset),
                         0.0, 1.0);
        rgb = straight * a;
    }
    color = vec4(rgb, a);

    if (checkerEnabled != 0) {
        // Same pattern as the widget viewer: square tiles of checkerTile
        // device pixels starting at the image's top-left corner, light in the
        // top-left and bottom-right cells.
        vec2 cell = mod(devicePos - checkerOrigin, vec2(checkerTile));
        bool light = (cell.x < checkerFirstCell) == (cell.y < checkerFirstCell);
        vec4 checker = light ? checkerLight : checkerDark;
        color += checker * (1.0 - color.a);
    }
    fragColor = color;
}
