#!/bin/bash

# Copyright 2022 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set +x

zmake build --clobber test-drivers || exit 1

echo "Searching for '${1}'..."
found_errors=0
loop_count=100
start_time=$(date +%Y-%m-%d_%H.%M.%S)
log_dir="/tmp"
EXECUTABLE=./build/zephyr/test-drivers/build-singleimage/zephyr/zephyr.exe
while [ "${loop_count}" -gt 0 ]; do
  seed=${RANDOM}
  log_file_prefix="${log_dir}"/shuffle_"${start_time}"_"${seed}"

  echo "[$((100 - loop_count))] Using seed=${seed}"
  error_count=$(timeout 150s "${EXECUTABLE}" -seed="${seed}" 2>&1 |
    tee "${log_file_prefix}".log |
    grep -c "${1}")
  status=$?

  result="0-matches"
  if [ "${status}" -eq 124 ]; then
    echo "    Timeout"
    result="timed-out"
  elif [ "${status}" -ne 0 ]; then
    echo "    Error/timeout"
    result="exit-code-${status}"
  fi
  if [ "${error_count}" -gt 0 ]; then
    echo "    Found ${error_count} errors matching '${1}'"
    result="${error_count}-matches"
  fi

  # Rename the log file to include the outcome
  mv \
    "${log_file_prefix}".log \
    "${log_file_prefix}"_"${result}".log

  found_errors=$((found_errors + error_count))
  loop_count=$((loop_count - 1))
done

if [ "${found_errors}" -ne 0 ]; then
  exit 1
fi
