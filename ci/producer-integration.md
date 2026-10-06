# MIRO producer integration candidate

The application C/Lua/raylib code is preserved. `build-miro.ysh` uses pinned
NDK r27c direct compile/link stages and verifies the existing build-toolchain
contract with the NDK-built host kernel before build effects. Lua archive bytes
and raylib commit are pinned; each attempt uses private scratch. All three
variants retain strip sizes and native digests. `package-miro.ysh` invokes the
single canonical Android-NDK packager; it does not implement a second one.

The refreshed base b8fb0cbe18b37ad38411e70e47c105a1b76e58fb had replaced the
older no-op signer step with AICI composite calls. Those calls still reached
the insufficient v1 receipt/ancestry gate. The new workflow has no alternate
build/bootstrap/policy path: it requires the independent qualified deployment.
On ordinary GitHub runners it remains explicitly BLOCKED. No accepted artifact
upload bypass remains. Application and emulator diagnostic source are retained;
no installation or device reconfiguration is performed by this change.

Two fresh native builds and canonical package sets (versionCodes 15 and 16)
were inspected locally. See `evidence/fp3-candidate-pair.tsv`; they are candidates,
not accepted A/B builds. Distinct APK hashes and a matching registered signer do
not prove replacement installation. Original key recovery is UNKNOWN. The
approved stable lane, independent producer deployment, emulator/physical
replacement, launch and visual behavior are not established here.

Registration of crystal-miro-a1-apk remains blocked. Fresh FP2 materialization
is isomorphisms/flexible-pipes PR #31, “Preserve blocked FP2 runtime qualification
without language substitution” (https://github.com/isomorphisms/flexible-pipes/pull/31),
head 74d8ac804b0bba983b3448c9f474241e2d283042. It preserves failing runtime
prerequisites and explicitly supplies no qualified registered engine. No operation
registration or demonstration through either dispatch route is claimed.

Owners: [Crystal #8](https://github.com/functorial-games/crystal/issues/8),
[Android-NDK #14](https://github.com/isomorphisms/android-NDK/issues/14),
[AICI #207](https://github.com/isomorphisms/ai-ci/issues/207),
[Cat Food #109](https://github.com/isomorphisms/catfood/issues/109),
[FP #28](https://github.com/isomorphisms/flexible-pipes/issues/28) and
[FP #16](https://github.com/isomorphisms/flexible-pipes/issues/16).
