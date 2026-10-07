#include "java_hooks.h"
#include "helper_dex.h"
#include "lsplant_init.h"
#include "log.h"
#include <lsplant.hpp>
#include <string>

static_assert(kHelperDexLen > 0, "helper.dex is empty!");

namespace unlockuwb {

static jobject LoadHelper(JNIEnv* env, jobject appCl) {
    jobject buf = env->NewDirectByteBuffer((void*)kHelperDex, (jlong)kHelperDexLen);
    jclass cl = env->FindClass("dalvik/system/InMemoryDexClassLoader");
    if (!cl) {
        LOGE("FindClass InMemoryDexClassLoader failed");
        return nullptr;
    }
    jmethodID ctor = env->GetMethodID(cl, "<init>",
        "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    jobject helperCl = env->NewObject(cl, ctor, buf, appCl);
    return env->NewGlobalRef(helperCl);
}

static jclass HelperClass(JNIEnv* env, jobject helperCl, const char* dotName) {
    jclass clcl = env->FindClass("java/lang/ClassLoader");
    jmethodID load = env->GetMethodID(clcl, "loadClass",
        "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring n = env->NewStringUTF(dotName);
    jclass c = (jclass)env->CallObjectMethod(helperCl, load, n);
    env->DeleteLocalRef(n);
    return c;
}

static jobject ResolveMethod(JNIEnv* env, jclass resolver,
                             jobject appCl, const char* cls, const char* name,
                             jobjectArray paramTypes) {
    jmethodID m = env->GetStaticMethodID(resolver, "method",
        "(Ljava/lang/ClassLoader;Ljava/lang/String;Ljava/lang/String;"
        "[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
    jstring jc = env->NewStringUTF(cls);
    jstring jn = env->NewStringUTF(name);
    jobject res = env->CallStaticObjectMethod(resolver, m, appCl, jc, jn, paramTypes);
    env->DeleteLocalRef(jc);
    env->DeleteLocalRef(jn);
    return res;
}

static jobject NewHooker(JNIEnv* env, jclass hookerCls, jint mode, jobject constant) {
    jmethodID ctor = env->GetMethodID(hookerCls, "<init>", "()V");
    jobject h = env->NewObject(hookerCls, ctor);
    env->SetIntField(h, env->GetFieldID(hookerCls, "mode", "I"), mode);
    if (constant) {
        env->SetObjectField(h, env->GetFieldID(hookerCls, "constant", "Ljava/lang/Object;"), constant);
    }
    return h;
}

static bool HookOne(JNIEnv* env, jclass resolver, jclass hookerCls,
                    jobject appCl, const char* cls, const char* name,
                    jobjectArray paramTypes, jint mode, jobject constant) {
    jobject target = ResolveMethod(env, resolver, appCl, cls, name, paramTypes);
    if (!target) {
        LOGD("Target method not found or not loaded: %s.%s", cls, name);
        return false;
    }
    jobject hooker = NewHooker(env, hookerCls, mode, constant);
    jmethodID cbId = env->GetMethodID(hookerCls, "callback",
        "([Ljava/lang/Object;)Ljava/lang/Object;");
    jobject cbMethod = env->ToReflectedMethod(hookerCls, cbId, JNI_FALSE);
    jobject backup = lsplant::Hook(env, target, hooker, cbMethod);
    if (!backup) {
        LOGE("lsplant::Hook failed: %s.%s", cls, name);
        return false;
    }
    env->SetObjectField(hooker, env->GetFieldID(hookerCls, "backup",
        "Ljava/lang/reflect/Method;"), backup);
    env->NewGlobalRef(hooker);
    LOGI("Successfully hooked %s.%s", cls, name);
    return true;
}

static jobjectArray Params0(JNIEnv* env) {
    return env->NewObjectArray(0, env->FindClass("java/lang/Class"), nullptr);
}

static jobjectArray Params1(JNIEnv* env, jclass resolver, jobject appCl, const char* t) {
    jmethodID tm = env->GetStaticMethodID(resolver, "type",
        "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;");
    jstring s = env->NewStringUTF(t);
    jclass c = (jclass)env->CallStaticObjectMethod(resolver, tm, appCl, s);
    jobjectArray a = env->NewObjectArray(1, env->FindClass("java/lang/Class"), c);
    env->DeleteLocalRef(s);
    return a;
}

static jobject BoxBool(JNIEnv* env, bool val) {
    jclass c = env->FindClass("java/lang/Boolean");
    jfieldID f = env->GetStaticFieldID(c, val ? "TRUE" : "FALSE", "Ljava/lang/Boolean;");
    return env->GetStaticObjectField(c, f);
}

static jobject BoxInt(JNIEnv* env, int val) {
    jclass c = env->FindClass("java/lang/Integer");
    jmethodID m = env->GetStaticMethodID(c, "valueOf", "(I)Ljava/lang/Integer;");
    return env->CallStaticObjectMethod(c, m, val);
}

void InstallAnchor(JNIEnv* env, void* callback) {
    jobject appCl = env->NewGlobalRef(
        env->CallObjectMethod(
            env->FindClass("android/app/ActivityThread"),
            env->GetStaticMethodID(env->FindClass("android/app/ActivityThread"),
                                   "currentApplication", "()Landroid/app/Application;")
        )
    );
    // Anchor via Instrumentation.callApplicationOnCreate
    jclass instrCls = env->FindClass("android/app/Instrumentation");
    jmethodID targetMid = env->GetMethodID(instrCls, "callApplicationOnCreate",
        "(Landroid/app/Application;)V");
    jobject target = env->ToReflectedMethod(instrCls, targetMid, JNI_FALSE);

    jobject helperCl = LoadHelper(env, nullptr);
    if (!helperCl) return;
    jclass anchorCls = HelperClass(env, helperCl, "io.github.skb8.unlockuwb.helper.Anchor");

    JNINativeMethod nm[] = {
        {(char*)"onCreate", (char*)"(Ljava/lang/Object;)V", callback}
    };
    env->RegisterNatives(anchorCls, nm, 1);

    jmethodID ctor = env->GetMethodID(anchorCls, "<init>", "()V");
    jobject anchor = env->NewObject(anchorCls, ctor);
    jmethodID cbMid = env->GetMethodID(anchorCls, "callback",
        "([Ljava/lang/Object;)Ljava/lang/Object;");
    jobject cb = env->ToReflectedMethod(anchorCls, cbMid, JNI_FALSE);

    jobject backup = lsplant::Hook(env, target, anchor, cb);
    if (backup) {
        env->SetObjectField(anchor,
            env->GetFieldID(anchorCls, "backup", "Ljava/lang/reflect/Method;"), backup);
        env->NewGlobalRef(anchor);
        LOGI("Anchor installed on Instrumentation.callApplicationOnCreate");
    }
}

void InstallSettingsHooks(JNIEnv* env, jobject classLoader) {
    LOGI("Installing hooks for com.android.settings...");
    jobject helperCl = LoadHelper(env, classLoader);
    if (!helperCl) {
        LOGE("Failed to load helper dex in settings");
        return;
    }
    jclass resolver = HelperClass(env, helperCl, "io.github.skb8.unlockuwb.helper.Resolver");
    jclass hookerCls = HelperClass(env, helperCl, "io.github.skb8.unlockuwb.helper.Hooker");
    if (!resolver || !hookerCls) {
        LOGE("Resolver or Hooker class not found");
        return;
    }

    jobject trueVal = BoxBool(env, true);
    jobject falseVal = BoxBool(env, false);
    jobject zeroVal = BoxInt(env, 0);

    const char* secUwbPrefCtrl = "com.samsung.android.settings.uwb.UwbPreferenceController";
    const char* secUwbPolicy = "com.samsung.android.settings.uwb.UwbSettingPolicy";
    const char* secUwbCaptionCtrl = "com.samsung.android.settings.uwb.UwbPreferenceCaptionController";
    const char* aospUwbPrefCtrl = "com.android.settings.uwb.UwbPreferenceController";

    // 1. Samsung UwbPreferenceController: isUwbSupportedOnDevice -> true
    HookOne(env, resolver, hookerCls, classLoader, secUwbPrefCtrl,
            "isUwbSupportedOnDevice", Params0(env), 1 /* RETURN_CONST */, trueVal);

    // 2. Samsung UwbPreferenceController: isMenuUnavailable -> false
    HookOne(env, resolver, hookerCls, classLoader, secUwbPrefCtrl,
            "isMenuUnavailable", Params0(env), 1 /* RETURN_CONST */, falseVal);

    // 3. Samsung UwbPreferenceController: getAvailabilityStatus -> 0 (AVAILABLE)
    HookOne(env, resolver, hookerCls, classLoader, secUwbPrefCtrl,
            "getAvailabilityStatus", Params0(env), 1 /* RETURN_CONST */, zeroVal);

    // 4. Samsung UwbPreferenceController: displayPreference -> automatically enable UWB Labs
    HookOne(env, resolver, hookerCls, classLoader, secUwbPrefCtrl,
            "displayPreference", Params1(env, resolver, classLoader, "androidx.preference.PreferenceScreen"),
            3 /* CALL_AND_ENABLE_LABS */, nullptr);

    // 5. Samsung UwbPreferenceController: updateState -> reset isRegulationMode
    HookOne(env, resolver, hookerCls, classLoader, secUwbPrefCtrl,
            "updateState", Params1(env, resolver, classLoader, "androidx.preference.Preference"),
            4 /* CALL_AND_RESET_REGULATION */, nullptr);

    // 6. Samsung UwbSettingPolicy: isRestrictionMode -> false
    HookOne(env, resolver, hookerCls, classLoader, secUwbPolicy,
            "isRestrictionMode", Params0(env), 1 /* RETURN_CONST */, falseVal);

    // 7. Samsung UwbPreferenceCaptionController
    HookOne(env, resolver, hookerCls, classLoader, secUwbCaptionCtrl,
            "isUwbSupportedOnDevice", Params0(env), 1 /* RETURN_CONST */, trueVal);
    HookOne(env, resolver, hookerCls, classLoader, secUwbCaptionCtrl,
            "getAvailabilityStatus", Params0(env), 1 /* RETURN_CONST */, zeroVal);

    // 8. AOSP UwbPreferenceController
    HookOne(env, resolver, hookerCls, classLoader, aospUwbPrefCtrl,
            "isUwbSupportedOnDevice", Params0(env), 1 /* RETURN_CONST */, trueVal);
    HookOne(env, resolver, hookerCls, classLoader, aospUwbPrefCtrl,
            "isUwbDisabledDueToRegulatory", Params0(env), 1 /* RETURN_CONST */, falseVal);
    HookOne(env, resolver, hookerCls, classLoader, aospUwbPrefCtrl,
            "getAvailabilityStatus", Params0(env), 1 /* RETURN_CONST */, zeroVal);

    LOGI("com.android.settings hooks installed.");
}

void InstallSystemServerHooks(JNIEnv* env, jobject classLoader) {
    LOGI("Installing hooks for system_server...");
    jobject helperCl = LoadHelper(env, classLoader);
    if (!helperCl) {
        LOGE("Failed to load helper dex in system_server");
        return;
    }
    jclass resolver = HelperClass(env, helperCl, "io.github.skb8.unlockuwb.helper.Resolver");
    jclass hookerCls = HelperClass(env, helperCl, "io.github.skb8.unlockuwb.helper.Hooker");
    if (!resolver || !hookerCls) return;

    jstring usCode = env->NewStringUTF("US");

    // 1. CountryDetectorService.detectCountry -> Return Country("US", 1)
    HookOne(env, resolver, hookerCls, classLoader,
            "com.android.server.CountryDetectorService",
            "detectCountry", Params0(env), 2 /* RETURN_COUNTRY_OBJ */, nullptr);

    // 2. ComprehensiveCountryDetector.detectCountry -> Return Country("US", 1)
    HookOne(env, resolver, hookerCls, classLoader,
            "com.android.server.location.countrydetector.ComprehensiveCountryDetector",
            "detectCountry", Params0(env), 2 /* RETURN_COUNTRY_OBJ */, nullptr);

    // 3. AOSP UwbCountryCode.getCountryCode -> "US"
    HookOne(env, resolver, hookerCls, classLoader,
            "com.android.server.uwb.UwbCountryCode",
            "getCountryCode", Params0(env), 1 /* RETURN_CONST */, usCode);

    // 4. Samsung UwbCountryCode.getCountryCode (if present) -> "US"
    HookOne(env, resolver, hookerCls, classLoader,
            "com.samsung.android.server.uwb.UwbCountryCode",
            "getCountryCode", Params0(env), 1 /* RETURN_CONST */, usCode);

    LOGI("system_server hooks installed.");
}

} // namespace unlockuwb
