#include "CurtainAnim.h"
#include <log/log.h>
#include <android-base/properties.h>

namespace android {
namespace renderengine {
namespace skia {

CurtainAnim::CurtainAnim() {
    SkString sksl(R"(
        uniform shader in_image;
        uniform float2 in_maxSizeXY;
        uniform float in_pushDeep;
        uniform float in_foldDeep;
        uniform float in_screenRadius;
        uniform float in_progress;

        const float PI = 3.14159265;
        const float SMOOTH = 0.005;

        const float mScreenWidth = 2.0;
        const float mScreenHeight = 2.0;
        const float mCameraDegree = 30;
        const float cameraDistance = (mScreenWidth / 2.0) / tan(mCameraDegree * 0.5 * PI / 180.0);

        float2 to2D(float3 point3){
            point3.z += cameraDistance;
            float zW = mScreenWidth/2.0 * point3.z / cameraDistance;
            float zH = mScreenHeight/2.0 * point3.z / cameraDistance;
            float2 point2 = float2(mScreenWidth / 2.0 * point3.x / zW,mScreenHeight / 2.0 * point3.y / zH);
            return point2;
        }

        float3 getImgColor(float2 st){
            float2 uv = st * 2.-1.;
            uv = abs(uv);

            float sx = smoothstep(1.,1.-SMOOTH,uv.x);
            float sy = smoothstep(1.,1.-SMOOTH,uv.y);

            float a = 1.;
            if(uv.x > 1. - in_screenRadius && uv.y > 1. - in_screenRadius){
                uv -= 1. - in_screenRadius;
                a = smoothstep(in_screenRadius, in_screenRadius-SMOOTH, length(uv));
            }
            float2 coord = float2(st.x*in_maxSizeXY.x, st.y*in_maxSizeXY.y);
            return in_image.eval(coord).rgb * a * sx * sy;
        }

        float per(float from,float to,float val){
            return (val-from)/(to-from);
        }

        float lumin(float3 c){
            return 0.2126*c.r + 0.7152*c.g + 0.0722*c.b;
        }

        half4 main(float2 coord) {
            float2 st = float2(coord.x/in_maxSizeXY.x, coord.y/in_maxSizeXY.y);
            float2 uv = st*2.0 - 1.0;

            float aPer = smoothstep(0.,1.,in_progress);
            float d = mix(in_pushDeep,0.,aPer);
            float dis = mix(in_foldDeep,0.,aPer);

            float2 ct = to2D(float3(0.,1.,d + dis));
            float2 cb = to2D(float3(0.,-1.,d + dis));
            float2 rt = to2D(float3(1.,1.,d));
            float2 rb = to2D(float3(1.,-1.,d));

            float2 p = float2(st.x,st.y);
            float3 color = float3(0.);

            float2 tmpUV = uv;
            tmpUV.x = abs(tmpUV.x);
            float x = per(cb.x,rb.x,tmpUV.x);
            float z = mix(dis,0.,x);
            float y = per(mix(cb.y,rb.y,x), mix(ct.y,rt.y,x),tmpUV.y);
            float len = length(float2(x,dis - z));
            p.x = step(0.,uv.x) * mix(0.5,1.,len) + (1.-step(0.,uv.x)) * mix(0.5,0.,len);
            p.y = y;

            float per = smoothstep(0.,1.,in_progress);
            float zPer = smoothstep(0.,1.,1.-z);

            float ang = - PI/2. * clamp((1.-per)*(1.-per),0.,1.);
            float c = cos(ang);
            float s = sin(ang);
            float2 lightUV = float2(uv.x,uv.y+ct.y)*mat2(c, s, -s, c);
            float tx = clamp(smoothstep(-0.5,0.5,lightUV.x),0.,1.);
            tx *= smoothstep(-0.2,0.2,uv.x);

            per = mix(tx,1.,zPer * per);

            float3 imgColor = getImgColor(p);
            float lumin = lumin(imgColor);
            float maxLight = mix(1.,0.,per);
            float minLight = max(maxLight-0.3,0.);
            float imgAlpha = clamp(smoothstep(minLight,maxLight,lumin),0.,1.) * per;

            color = mix(float3(0.), getImgColor(p),zPer * imgAlpha);
            return half4(color, 1);
        }
    )");

    // Init shader.
    auto [curtainAnimEffect, error] = SkRuntimeEffect::MakeForShader(sksl);
    if (!curtainAnimEffect) {
        LOG_ALWAYS_FATAL("RuntimeShader error: %s", error.c_str());
    }
    mCurtainAnimEffect = std::move(curtainAnimEffect);

    // Init uniform properties.
    mPushDeep = std::atof(
            android::base::GetProperty("ro.sf.curtain_anim.push_deep", "1.0").c_str());
    mFoldDeep = std::atof(
            android::base::GetProperty("ro.sf.curtain_anim.fold_deep", "0.5").c_str());
    mScreenRadius = std::atof(
            android::base::GetProperty("ro.sf.curtain_anim.screen_radius", "0.025").c_str());
    mDebug = android::base::GetBoolProperty("ro.sf.curtain_anim.debug", false);
}

sk_sp<SkShader> CurtainAnim::makeShader(SkCanvas* canvas, const SkRect& effectRegion,
                                          sk_sp<SkImage> input, float progress) {
    SkMatrix inputMatrix;
    if (!canvas->getTotalMatrix().invert(&inputMatrix)) {
        ALOGE("matrix was unable to be inverted");
    }
    SkRuntimeShaderBuilder curtainAnimBuilder(mCurtainAnimEffect);
    SkSamplingOptions linearSampling(SkFilterMode::kLinear, SkMipmapMode::kNone);
    curtainAnimBuilder.child("in_image") = input->makeShader(SkTileMode::kClamp,
            SkTileMode::kClamp, linearSampling, inputMatrix);
    curtainAnimBuilder.uniform("in_maxSizeXY") =
            SkV2{(float)effectRegion.width(), (float)effectRegion.height()};
    curtainAnimBuilder.uniform("in_pushDeep") = mPushDeep;
    curtainAnimBuilder.uniform("in_foldDeep") = mFoldDeep;
    curtainAnimBuilder.uniform("in_screenRadius") = mScreenRadius;
    curtainAnimBuilder.uniform("in_progress") = progress;
    if (mDebug) {
        ALOGD("Curtain-Anim: progress=%f pushDeep=%f foldDeep=%f screenRadius=%f",
                progress, mPushDeep, mFoldDeep, mScreenRadius);
    }
    return curtainAnimBuilder.makeShader();
}

void CurtainAnim::drawCurtainAnim(SkCanvas* canvas, float rate, const sk_sp<SkSurface> surface){
    if (rate >= 0.f && rate < 1.0f) {
        sk_sp<SkImage> image = surface->makeImageSnapshot();
        SkPaint paint;
        const SkRect bounds = canvas->getLocalClipBounds();
        auto curtainAnimShader =
                makeShader(canvas, bounds, image, rate);
        paint.setShader(curtainAnimShader);
        canvas->clear(SK_ColorTRANSPARENT);
        canvas->drawRect(bounds, paint);
    }
}

} // namespace skia
} // namespace renderengine
} // namespace android