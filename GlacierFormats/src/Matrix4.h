#pragma once

namespace GlacierFormats {

    class Transform {
    public:
        float data[16];

        Transform() {};

        Transform(const float* position, const float* quaternion) {
            const float* q = quaternion;
            const float x = q[0];
            const float y = q[1];
            const float z = q[2];
            const float w = q[3];

            data[0] = 1 - 2 * (y * y - z * z);
            data[1] = 2 * (x * y - z * w);
            data[2] = 2 * (x * z + y * w);
            data[3] = 0.0f;
            data[4] = 2 * (x * y + z * w);
            data[5] = 1 - 2 * (x * x - z * z);
            data[6] = 2 * (y * z - x * w);
            data[7] = 0.0f;
            data[8] = 2 * (x * z - y * w);
            data[9] = 2 * (y * z + x * w);
            data[10] = 1 - 2 * (x * x - y * y);
            data[11] = 0.0f;

            data[12] = position[0];
            data[13] = position[1];
            data[14] = position[2];
            data[15] = 1.0f;
        }

        const float& operator[](int i) const { return data[i]; }
        float& operator[](int i) { return data[i]; }

        Transform inverse() const {
            float inv[16], det;
            int i;

            const float* m = data;

            inv[0] = m[5] * m[10] * m[15] -
                m[5] * m[11] * m[14] -
                m[9] * m[6] * m[15] +
                m[9] * m[7] * m[14] +
                m[13] * m[6] * m[11] -
                m[13] * m[7] * m[10];

            inv[4] = -m[4] * m[10] * m[15] +
                m[4] * m[11] * m[14] +
                m[8] * m[6] * m[15] -
                m[8] * m[7] * m[14] -
                m[12] * m[6] * m[11] +
                m[12] * m[7] * m[10];

            inv[8] = m[4] * m[9] * m[15] -
                m[4] * m[11] * m[13] -
                m[8] * m[5] * m[15] +
                m[8] * m[7] * m[13] +
                m[12] * m[5] * m[11] -
                m[12] * m[7] * m[9];

            inv[12] = -m[4] * m[9] * m[14] +
                m[4] * m[10] * m[13] +
                m[8] * m[5] * m[14] -
                m[8] * m[6] * m[13] -
                m[12] * m[5] * m[10] +
                m[12] * m[6] * m[9];

            inv[1] = -m[1] * m[10] * m[15] +
                m[1] * m[11] * m[14] +
                m[9] * m[2] * m[15] -
                m[9] * m[3] * m[14] -
                m[13] * m[2] * m[11] +
                m[13] * m[3] * m[10];

            inv[5] = m[0] * m[10] * m[15] -
                m[0] * m[11] * m[14] -
                m[8] * m[2] * m[15] +
                m[8] * m[3] * m[14] +
                m[12] * m[2] * m[11] -
                m[12] * m[3] * m[10];

            inv[9] = -m[0] * m[9] * m[15] +
                m[0] * m[11] * m[13] +
                m[8] * m[1] * m[15] -
                m[8] * m[3] * m[13] -
                m[12] * m[1] * m[11] +
                m[12] * m[3] * m[9];

            inv[13] = m[0] * m[9] * m[14] -
                m[0] * m[10] * m[13] -
                m[8] * m[1] * m[14] +
                m[8] * m[2] * m[13] +
                m[12] * m[1] * m[10] -
                m[12] * m[2] * m[9];

            inv[2] = m[1] * m[6] * m[15] -
                m[1] * m[7] * m[14] -
                m[5] * m[2] * m[15] +
                m[5] * m[3] * m[14] +
                m[13] * m[2] * m[7] -
                m[13] * m[3] * m[6];

            inv[6] = -m[0] * m[6] * m[15] +
                m[0] * m[7] * m[14] +
                m[4] * m[2] * m[15] -
                m[4] * m[3] * m[14] -
                m[12] * m[2] * m[7] +
                m[12] * m[3] * m[6];

            inv[10] = m[0] * m[5] * m[15] -
                m[0] * m[7] * m[13] -
                m[4] * m[1] * m[15] +
                m[4] * m[3] * m[13] +
                m[12] * m[1] * m[7] -
                m[12] * m[3] * m[5];

            inv[14] = -m[0] * m[5] * m[14] +
                m[0] * m[6] * m[13] +
                m[4] * m[1] * m[14] -
                m[4] * m[2] * m[13] -
                m[12] * m[1] * m[6] +
                m[12] * m[2] * m[5];

            inv[3] = -m[1] * m[6] * m[11] +
                m[1] * m[7] * m[10] +
                m[5] * m[2] * m[11] -
                m[5] * m[3] * m[10] -
                m[9] * m[2] * m[7] +
                m[9] * m[3] * m[6];

            inv[7] = m[0] * m[6] * m[11] -
                m[0] * m[7] * m[10] -
                m[4] * m[2] * m[11] +
                m[4] * m[3] * m[10] +
                m[8] * m[2] * m[7] -
                m[8] * m[3] * m[6];

            inv[11] = -m[0] * m[5] * m[11] +
                m[0] * m[7] * m[9] +
                m[4] * m[1] * m[11] -
                m[4] * m[3] * m[9] -
                m[8] * m[1] * m[7] +
                m[8] * m[3] * m[5];

            inv[15] = m[0] * m[5] * m[10] -
                m[0] * m[6] * m[9] -
                m[4] * m[1] * m[10] +
                m[4] * m[2] * m[9] +
                m[8] * m[1] * m[6] -
                m[8] * m[2] * m[5];

            det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

            if (det == 0)
                throw std::runtime_error("Matrix isn't invertible");

            det = static_cast<float>(1.0 / det);

            Transform ret;
            for (i = 0; i < 16; i++)
                ret.data[i] = inv[i] * det;
            return ret;
        }

        static Transform Identity() {
            Transform mat;
            std::fill(std::begin(mat.data), std::end(mat.data), 0.f);
            mat.data[0] = 1.0f;
            mat.data[5] = 1.0f;
            mat.data[10] = 1.0f;
            mat.data[15] = 1.0f;
            return mat;
        }

        Transform multiply(const Transform& m1) const {
            Transform r;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    float v = 0.f;
                    for (int k = 0; k < 4; ++k) {
                        v += this->data[i * 4 + k] * m1[k * 4 + j];
                    }
                    r[i * 4 + j] = v;
                }
            }
            return r;
        }


    };
}
