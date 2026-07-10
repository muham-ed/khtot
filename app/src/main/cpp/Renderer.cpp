#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <chrono>

#include "AndroidOut.h"
#include "Shader.h"
#include "Utility.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// لون الخلفية (داكن جداً للفخامة)
#define DARK_BACKGROUND 18 / 255.f, 18 / 255.f, 24 / 255.f, 1

static const char *vertex = R"vertex(#version 300 es
in vec3 inPosition;
uniform mat4 uProjection;
uniform mat4 uModel;
void main() {
    gl_Position = uProjection * uModel * vec4(inPosition, 1.0);
}
)vertex";

static const char *fragment = R"fragment(#version 300 es
precision mediump float;
out vec4 outColor;
uniform vec4 uColor;
void main() {
    outColor = uColor;
}
)fragment";

static constexpr float kProjectionHalfHeight = 2.f;
static constexpr float kProjectionNearPlane = -1.f;
static constexpr float kProjectionFarPlane = 1.f;

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::render() {
    updateRenderArea();

    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    float deltaTime = (lastTime_ > 0) ? (float)(currentTime - (long long)lastTime_) / 1000.0f : 0.0f;
    lastTime_ = (double)currentTime;

    game_.update(deltaTime);

    if (shaderNeedsNewProjectionMatrix_) {
        float projectionMatrix[16] = {0};
        Utility::buildOrthographicMatrix(
                projectionMatrix,
                kProjectionHalfHeight,
                float(width_) / height_,
                kProjectionNearPlane,
                kProjectionFarPlane);
        shader_->setProjectionMatrix(projectionMatrix);
        shaderNeedsNewProjectionMatrix_ = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    if (!models_.empty()) {
        float modelMatrix[16];
        float cellSize = 0.6f;
        int currentGridWidth = 3 + (game_.getLevel() / 5);
        if (currentGridWidth > 6) currentGridWidth = 6;
        float gridOffsetX = (currentGridWidth * cellSize) / 2.0f - (cellSize / 2.0f);
        float gridOffsetY = (currentGridWidth * cellSize) / 2.0f - (cellSize / 2.0f);

        GLint colorLoc = glGetUniformLocation(shader_->getProgram(), "uColor");

        for (const auto &arrow : game_.getArrows()) {
            if (arrow.gone) continue;

            float angle = 0;
            if (arrow.dir == Direction::RIGHT) angle = 0;
            else if (arrow.dir == Direction::UP) angle = M_PI / 2.0f;
            else if (arrow.dir == Direction::LEFT) angle = M_PI;
            else if (arrow.dir == Direction::DOWN) angle = -M_PI / 2.0f;

            float worldX = (arrow.currentX * cellSize) - gridOffsetX;
            float worldY = (arrow.currentY * cellSize) - gridOffsetY;

            Utility::buildTransformMatrix(modelMatrix, worldX, worldY, angle, 0.4f);
            shader_->setModelMatrix(modelMatrix);

            // لون السهم (أبيض مائل للزرقة)
            glUniform4f(colorLoc, 0.9f, 0.95f, 1.0f, 1.0f);
            shader_->drawModel(models_[0]);
        }
    }

    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::initRenderer() {
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    eglMakeCurrent(display, surface, surface, context);
    display_ = display;
    surface_ = surface;
    context_ = context;

    width_ = -1;
    height_ = -1;

    shader_ = std::unique_ptr<Shader>(
            Shader::loadShader(vertex, fragment, "inPosition", "", "uProjection", "uModel"));
    assert(shader_);
    shader_->activate();

    glClearColor(DARK_BACKGROUND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    createModels();
}

void Renderer::updateRenderArea() {
    EGLint width, height;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);
        shaderNeedsNewProjectionMatrix_ = true;
    }
}

void Renderer::createModels() {
    // تصميم سهم "خطوط" (Khtot) احترافي
    // يتكون من مثلث للرأس ومستطيل للجسم
    std::vector<Vertex> vertices = {
            Vertex(Vector3{0.5f, 0, 0}, Vector2{0,0}),     // 0: قمة الرأس
            Vertex(Vector3{0.1f, 0.3f, 0}, Vector2{0,0}),   // 1: زاوية الرأس علوية
            Vertex(Vector3{0.1f, -0.3f, 0}, Vector2{0,0}),  // 2: زاوية الرأس سفلية
            Vertex(Vector3{0.1f, 0.08f, 0}, Vector2{0,0}),  // 3: بداية الجسم علوي
            Vertex(Vector3{-0.5f, 0.08f, 0}, Vector2{0,0}), // 4: نهاية الجسم علوي
            Vertex(Vector3{-0.5f, -0.08f, 0}, Vector2{0,0}),// 5: نهاية الجسم سفلي
            Vertex(Vector3{0.1f, -0.08f, 0}, Vector2{0,0})  // 6: بداية الجسم سفلي
    };
    std::vector<Index> indices = {
            0, 1, 2,     // مثلث الرأس
            3, 4, 5,     // مستطيل الجسم (المثلث الأول)
            3, 5, 6      // مستطيل الجسم (المثلث الثاني)
    };

    models_.emplace_back(vertices, indices, nullptr);
}

void Renderer::handleInput() {
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) return;

    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        if ((action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_DOWN) {
            game_.handleTap(x, y, width_, height_);
            if (game_.isLevelCleared()) {
                game_.nextLevel();
            }
        }
    }
    android_app_clear_motion_events(inputBuffer);
}
