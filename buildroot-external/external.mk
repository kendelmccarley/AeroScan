include $(sort $(wildcard $(BR2_EXTERNAL_AEROSCAN_PATH)/package/*/*.mk))

# Buildroot's dump1090 package forces CPUFEATURES=no, which drops the
# runtime-dispatched SIMD demodulator ("starch") paths and leaves the scalar
# fallback pinning a core at 2.4 MS/s. The v9.0 tarball bundles the
# cpu_features sources, so re-enable it. ARCH must name the target arch
# (aarch64 → NEON mix on rpi4, arm → ARMv7 NEON mix on rpi2) — dump1090's
# Makefile otherwise keys the DSP mix off the build host's `uname -m`.
# Appending here works: external.mk is included after package .mk files, and
# with duplicate command-line assignments the last one wins.
DUMP1090_MAKE_OPTS += CPUFEATURES=yes ARCH=$(ARCH)
