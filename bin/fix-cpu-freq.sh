#!/bin/bash
help () {
  echo >&2 'Usage: fix-cpu-freq <commands>'
  exit 1
}
cmd=$@
if [[ -z $cmd ]]; then help; fi

# Disable CPU frequency variability (Turbo Boost).
cpudir=/sys/devices/system/cpu
if [[ -e $cpudir/intel_pstate ]]; then
  min_perf_pct="$(cat $cpudir/intel_pstate/min_perf_pct)"
  max_perf_pct="$(cat $cpudir/intel_pstate/max_perf_pct)"
  sudo bash -c "echo $max_perf_pct >$cpudir/intel_pstate/min_perf_pct"
  freq="$(cat $cpudir/cpu0/cpufreq/scaling_min_freq)"
elif [[ -e $cpudir/cpu0/cpufreq ]]; then
  governor="$(cat $cpudir/cpu0/cpufreq/scaling_governor)"
  freq="$(cat $cpudir/cpu0/cpufreq/scaling_min_freq)"
  for cpu in $cpudir/cpu*; do
    sudo bash -c "echo userspace >$cpu/cpufreq/scaling_governor"
    sudo bash -c "echo $freq >$cpu/cpufreq/scaling_setspeed"
  done
fi

$cmd

# Re-enable CPU variability.
if [[ -e $cpudir/intel_pstate ]]; then
  sudo bash -c "echo $min_perf_pct >$cpudir/intel_pstate/min_perf_pct"
elif [[ -e $cpudir/cpu0/cpufreq ]]; then
  for cpu in $cpudir/cpu*; do
    sudo bash -c "echo $governor >$cpu/cpufreq/scaling_governor"
  done
fi
