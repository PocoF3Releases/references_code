/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#define I(_i, _j) ((_j)+ 4*(_i))

namespace android {
    class Math {
    public:
        static void setIdentityM(float *sm, int smOffset) {
            for (int i=0 ; i<16 ; i++) {
                sm[smOffset + i] = 0;
            }
            for(int i = 0; i < 16; i += 5) {
                sm[smOffset + i] = 1.0f;
            }
        }

        static void scaleM(float *m, int mOffset, float x, float y, float z) {
            for (int i=0 ; i<4 ; i++) {
                int mi = mOffset + i;
                m[     mi] *= x;
                m[ 4 + mi] *= y;
                m[ 8 + mi] *= z;
            }
        }

        static void orthoM(float *m, int mOffset,
                                      float left, float right, float bottom, float top,
                                      float near, float far) {

            float r_width  = 1.0f / (right - left);
            float r_height = 1.0f / (top - bottom);
            float r_depth  = 1.0f / (far - near);
            float x =  2.0f * (r_width);
            float y =  2.0f * (r_height);
            float z = -2.0f * (r_depth);
            float tx = -(right + left) * r_width;
            float ty = -(top + bottom) * r_height;
            float tz = -(far + near) * r_depth;
            m[mOffset + 0] = x;
            m[mOffset + 5] = y;
            m[mOffset +10] = z;
            m[mOffset +12] = tx;
            m[mOffset +13] = ty;
            m[mOffset +14] = tz;
            m[mOffset +15] = 1.0f;
            m[mOffset + 1] = 0.0f;
            m[mOffset + 2] = 0.0f;
            m[mOffset + 3] = 0.0f;
            m[mOffset + 4] = 0.0f;
            m[mOffset + 6] = 0.0f;
            m[mOffset + 7] = 0.0f;
            m[mOffset + 8] = 0.0f;
            m[mOffset + 9] = 0.0f;
            m[mOffset + 11] = 0.0f;
        }

        static void translateM(float* m, int mOffset, float x, float y, float z) {
            for (int i=0 ; i<4 ; i++) {
                int mi = mOffset + i;
                m[12 + mi] += m[mi] * x + m[4 + mi] * y + m[8 + mi] * z;
            }
        }

        static void multiplyMM(float* r, const float* lhs, const float* rhs) {
            for (int i=0 ; i<4 ; i++) {
                const float rhs_i0 = rhs[ I(i,0) ];
                float ri0 = lhs[ I(0,0) ] * rhs_i0;
                float ri1 = lhs[ I(0,1) ] * rhs_i0;
                float ri2 = lhs[ I(0,2) ] * rhs_i0;
                float ri3 = lhs[ I(0,3) ] * rhs_i0;
                for (int j=1 ; j<4 ; j++) {
                    const float rhs_ij = rhs[ I(i,j) ];
                    ri0 += lhs[ I(j,0) ] * rhs_ij;
                    ri1 += lhs[ I(j,1) ] * rhs_ij;
                    ri2 += lhs[ I(j,2) ] * rhs_ij;
                    ri3 += lhs[ I(j,3) ] * rhs_ij;
                }
                r[ I(i,0) ] = ri0;
                r[ I(i,1) ] = ri1;
                r[ I(i,2) ] = ri2;
                r[ I(i,3) ] = ri3;
            }
        }
    };
}  // namespace android
