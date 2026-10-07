#include <string>
#include <unistd.h>
#include "log.h"
#include "java_hooks.h"
#include "lsplant_init.h"
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

static std::string g_process_name;

static void Anchor_onCreate(JNIEnv* env, jclass, jobject application) {
    static bool done = false;
    if (done) return;
    done = true;

    LOGI("Settings application onCreate triggered, preparing hooks...");
    jclass appClass = env->GetObjectClass(application);
    jmethodID getCl = env->GetMethodID(appClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject appCl = env->NewGlobalRef(env->CallObjectMethod(application, getCl));

    unlockuwb::InstallSettingsHooks(env, appCl);
}

class UnlockUwbModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char* nice = env->GetStringUTFChars(args->nice_name, nullptr);
        g_process_name = nice ? nice : "";
        env->ReleaseStringUTFChars(args->nice_name, nice);

        // We only care about com.android.settings
        if (g_process_name != "com.android.settings") {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (g_process_name != "com.android.settings") return;

        LOGI("postAppSpecialize in com.android.settings (pid=%d)", getpid());
        if (!unlockuwb::InitLSPlant(env)) {
            LOGE("Failed to init LSPlant in Settings");
            return;
        }
        unlockuwb::InstallAnchor(env, (void*)Anchor_onCreate);
    }

    void preServerSpecialize(ServerSpecializeArgs *) override {
        // Keep module loaded in system_server
    }

    void postServerSpecialize(const ServerSpecializeArgs *) override {
        LOGI("postServerSpecialize in system_server (pid=%d)", getpid());
        if (!unlockuwb::InitLSPlant(env)) {
            LOGE("Failed to init LSPlant in system_server");
            return;
        }

        jclass clClass = env->FindClass("java/lang/ClassLoader");
        jmethodID getSysCl = env->GetStaticMethodID(clClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
        jobject sysCl = env->CallStaticObjectMethod(clClass, getSysCl);

        unlockuwb::InstallSystemServerHooks(env, sysCl);
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(UnlockUwbModule)
