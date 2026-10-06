# Agent entry point

For Android APK work in this repository:

1. use the shared AICI build-toolchain and Android update/signing identity rules;
2. use the maintained direct NativeActivity packager from `isomorphisms/android-NDK`;
3. do not introduce Gradle, Java, Kotlin, d8, or another APK build path for the maintained MIRO NativeActivity application;
4. keep the canonical package IDs `org.isomorphisms.crystal.halite`, `.quartz`, and `.bismuth` stable;
5. never generate or silently select a signing key;
6. distinguish package/build acceptance from physical-device visual/interaction acceptance.

Current packaging repair is tracked in issue #8.
