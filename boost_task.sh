#!/bin/bash
# Usage: sudo ./boost_task.sh <program> [args...]

# Check if program is provided
if [ $# -lt 1 ]; then
    # sudo ./boost_task.sh ./sim
    echo "Usage: sudo $0 <program> [args...]"
    exit 1
fi

PROGRAM="$1"
shift
ARGS="$@"

# ---- CONFIGURATION ----
# Number of CPU cores to use (all cores by default)
CORES=$(nproc)

# CPU governor: performance
echo "Setting CPU governor to performance..."
for cpu in /sys/devices/system/cpu/cpu[0-9]*; do
    echo performance | sudo tee $cpu/cpufreq/scaling_governor > /dev/null
done

# Nice value (higher priority)
NICE_VALUE=-10

# I/O priority (real-time class, highest)
IONICE_CLASS=1
IONICE_PRIORITY=0

# ------------------------

echo "Running $PROGRAM with optimizations..."
sudo nice -n $NICE_VALUE ionice -c $IONICE_CLASS -n $IONICE_PRIORITY taskset -c 0-$((CORES-1)) "$PROGRAM" $ARGS
