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

#define DARK_BG 15/255.f, 15/255.f, 20/255.f, 1

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(display_);
    }
}

void Renderer::render() {
    updateRenderArea();
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    float deltaTime = (lastTime_ > 0) ? (float)(currentTime - (long long)lastTime_) / 1000.0f : 0.0f;
    lastTime_ = (double)currentTime;
    game_.update(deltaTime);

    if (shaderNeedsNewProjectionMatrix_) {
        float projectionMatrix[16];
        Utility::buildOrthographicMatrix(projectionMatrix, 2.f, float(width_) / height_, -1.f, 1.f);
        shader_->setProjectionMatrix(projectionMatrix);
        shaderNeedsNewProjectionMatrix_ = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    if (!models_.empty()) {
        float modelMatrix[16];
        float cellSize = 0.6f;
        int size = 3 + (game_.getLevel() / 5); if (size > 6) size = 6;
        float gridOffsetX = (size * cellSize) / 2.0f - (cellSize / 2.0f);
        float gridOffsetY = (size * cellSize) / 2.0f - (cellSize / 2.0f);
        GLint colorLoc = glGetUniformLocation(shader_->getProgram(), "uColor");

        for (const auto &arrow : game_.getArrows()) {
            if (arrow.gone) continue;
            float angle = 0;
            if (arrow.dir == Direction::RIGHT) angle = 0;
            else if (arrow.dir == Direction::UP) angle = M_PI / 2.0f;
            else if (arrow.dir == Direction::LEFT) angle = M_PI;
            else if (arrow.dir == Direction::DOWN) angle = -M_PI / 2.0f;

            Utility::buildTransformMatrix(modelMatrix, (arrow.currentX * cellSize) - gridOffsetX, (arrow.currentY * cellSize) - gridOffsetY, angle, 0.4f);
            shader_->setModelMatrix(modelMatrix);

            if (arrow.isHinted) glUniform4f(colorLoc, 1.0f, 0.8f, 0.0f, 1.0f); // ذهبي للتلميح
            else glUniform4f(colorLoc, 0.9f, 0.95f, 1.0f, 1.0f);

            shader_->drawModel(models_[0]);
        }
    }
    eglSwapBuffers(display_, surface_);
}

void Renderer::initRenderer() {
    constexpr EGLint attribs[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_NONE };
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);
    EGLint numConfigs;
    EGLConfig config;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);
    EGLContext context = eglCreateContext(display, config, nullptr, new EGLint[3]{EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE});
    eglMakeCurrent(display, surface, surface, context);
    display_ = display; surface_ = surface; context_ = context;
    width_ = -1; height_ = -1;
    shader_ = std::unique_ptr<Shader>(Shader::loadShader(R"(#version 300 es
in vec3 inPosition; uniform mat4 uProjection; uniform mat4 uModel; void main() { gl_Position = uProjection * uModel * vec4(inPosition, 1.0); })",
R"(#version 300 es
precision mediump float; out vec4 outColor; uniform vec4 uColor; void main() { outColor = uColor; })", "inPosition", "", "uProjection", "uModel"));
    shader_->activate();
    glClearColor(DARK_BG);
    createModels();
}

void Renderer::updateRenderArea() {
    EGLint w, h; eglQuerySurface(display_, surface_, EGL_WIDTH, &w); eglQuerySurface(display_, surface_, EGL_HEIGHT, &h);
    if (w != width_ || h != height_) { width_ = w; height_ = h; glViewport(0, 0, w, h); shaderNeedsNewProjectionMatrix_ = true; }
}

void Renderer::createModels() {
    std::vector<Vertex> vertices = { Vertex({0.5f, 0, 0}, {0,0}), Vertex({0.1f, 0.3f, 0}, {0,0}), Vertex({0.1f, -0.3f, 0}, {0,0}), Vertex({0.1f, 0.08f, 0}, {0,0}), Vertex({-0.5f, 0.08f, 0}, {0,0}), Vertex({-0.5f, -0.08f, 0}, {0,0}), Vertex({0.1f, -0.08f, 0}, {0,0}) };
    std::vector<Index> indices = { 0, 1, 2, 3, 4, 5, 3, 5, 6 };
    models_.emplace_back(vertices, indices, nullptr);
}

void Renderer::handleInput() {
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) return;
    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &ev = inputBuffer->motionEvents[i];
        if ((ev.action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_DOWN) {
            float x = GameActivityPointerAxes_getX(&ev.pointers[0]);
            float y = GameActivityPointerAxes_getY(&ev.pointers[0]);
            if (!game_.handleTap(x, y, width_, height_)) {
                // هزة بسيطة عند الخطأ
            }
            if (game_.isLevelCleared()) game_.nextLevel();
        }
    }
    android_app_clear_motion_events(inputBuffer);
}
