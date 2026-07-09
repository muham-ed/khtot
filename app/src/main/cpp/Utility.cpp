#include "Utility.h"
#include "AndroidOut.h"
#include <cmath>
#include <GLES3/gl3.h>

#define CHECK_ERROR(e) case e: aout << "GL Error: "#e << std::endl; break;

bool Utility::checkAndLogGlError(bool alwaysLog) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        if (alwaysLog) {
            aout << "No GL error" << std::endl;
        }
        return true;
    } else {
        switch (error) {
            CHECK_ERROR(GL_INVALID_ENUM);
            CHECK_ERROR(GL_INVALID_VALUE);
            CHECK_ERROR(GL_INVALID_OPERATION);
            CHECK_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
            CHECK_ERROR(GL_OUT_OF_MEMORY);
            default:
                aout << "Unknown GL error: " << error << std::endl;
        }
        return false;
    }
}

float *
Utility::buildOrthographicMatrix(float *outMatrix, float halfHeight, float aspect, float near,
                                 float far) {
    float halfWidth = halfHeight * aspect;

    outMatrix[0] = 1.f / halfWidth;
    outMatrix[1] = 0.f;
    outMatrix[2] = 0.f;
    outMatrix[3] = 0.f;

    outMatrix[4] = 0.f;
    outMatrix[5] = 1.f / halfHeight;
    outMatrix[6] = 0.f;
    outMatrix[7] = 0.f;

    outMatrix[8] = 0.f;
    outMatrix[9] = 0.f;
    outMatrix[10] = -2.f / (far - near);
    outMatrix[11] = 0.f;

    outMatrix[12] = 0.f;
    outMatrix[13] = 0.f;
    outMatrix[14] = -(far + near) / (far - near);
    outMatrix[15] = 1.f;

    return outMatrix;
}

float *Utility::buildIdentityMatrix(float *outMatrix) {
    for (int i = 0; i < 16; i++) outMatrix[i] = 0.f;
    outMatrix[0] = 1.f;
    outMatrix[5] = 1.f;
    outMatrix[10] = 1.f;
    outMatrix[15] = 1.f;
    return outMatrix;
}

float *Utility::buildTransformMatrix(float *outMatrix, float x, float y, float rotationZ, float scale) {
    float c = cosf(rotationZ);
    float s = sinf(rotationZ);

    buildIdentityMatrix(outMatrix);

    outMatrix[0] = c * scale;
    outMatrix[1] = s * scale;
    outMatrix[4] = -s * scale;
    outMatrix[5] = c * scale;
    outMatrix[12] = x;
    outMatrix[13] = y;

    return outMatrix;
}
