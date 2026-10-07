#pragma once
#include <jni.h>

namespace unlockuwb {

void InstallSettingsHooks(JNIEnv* env, jobject classLoader);
void InstallSystemServerHooks(JNIEnv* env, jobject classLoader);
void InstallAnchor(JNIEnv* env, void* callback);

} // namespace unlockuwb
