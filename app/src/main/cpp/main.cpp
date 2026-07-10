#include <jni.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <game-activity/GameActivity.h>
#include "AndroidOut.h"
#include "Renderer.h"

extern "C" {
void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            pApp->userData = new Renderer(pApp);
            break;
        case APP_CMD_TERM_WINDOW:
            if (pApp->userData) {
                auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
                pApp->userData = nullptr;
                delete pRenderer;
            }
            break;
        default:
            break;
    }
}

bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

void android_main(struct android_app *pApp) {
    pApp->onAppCmd = handle_cmd;
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    do {
        bool done = false;
        while (!done) {
            int timeout = 0, events;
            android_poll_source *pSource;
            int result = ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&pSource));
            switch (result) {
                case ALOOPER_POLL_TIMEOUT: [[clang::fallthrough]];
                case ALOOPER_POLL_WAKE: done = true; break;
                case ALOOPER_EVENT_ERROR: break;
                default: if (pSource) pSource->process(pApp, pSource);
            }
        }
        if (pApp->userData) {
            auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
            pRenderer->handleInput();
            pRenderer->render();
        }
    } while (!pApp->destroyRequested);
}

JNIEXPORT jint JNICALL
Java_com_mohamedalaa_khtot_MainActivity_getNativeLevel(JNIEnv *env, jobject thiz, jlong rendererPtr) {
    if (rendererPtr == 0) return 1;
    return reinterpret_cast<Renderer *>(rendererPtr)->getGame().getLevel();
}
}
