// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/ui/ui_test_suite.h"

#include <string>

#include "base/environment.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/string_number_conversions.h"
#include "chrome/common/env_vars.h"

// Timeout for the test in milliseconds.  UI tests only.
const char UITestSuite::kTestTimeout[] = "test-timeout";

// Parameters to run test in parallel.  UI tests only.
const char UITestSuite::kBatchCount[] = "batch-count";
const char UITestSuite::kBatchIndex[] = "batch-index";
const char UITestSuite::kGTestTotalShards[] = "GTEST_TOTAL_SHARDS=";
const char UITestSuite::kGTestShardIndex[] = "GTEST_SHARD_INDEX=";


UITestSuite::UITestSuite(int argc, char** argv)
    : ChromeTestSuite(argc, argv) {
#if defined(OS_WIN)
  crash_service_ = NULL;
#endif
}

void UITestSuite::Initialize() {
  ChromeTestSuite::Initialize();

  const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();
  UITest::set_in_process_renderer(
      parsed_command_line.HasSwitch(switches::kSingleProcess));
  UITest::set_no_sandbox(
      parsed_command_line.HasSwitch(switches::kNoSandbox));
  UITest::set_full_memory_dump(
      parsed_command_line.HasSwitch(switches::kFullMemoryCrashReport));
  UITest::set_safe_plugins(
      parsed_command_line.HasSwitch(switches::kSafePlugins));
  UITest::set_dump_histograms_on_exit(
      parsed_command_line.HasSwitch(switches::kDumpHistogramsOnExit));
  UITest::set_enable_dcheck(
      parsed_command_line.HasSwitch(switches::kEnableDCHECK));
  UITest::set_silent_dump_on_dcheck(
      parsed_command_line.HasSwitch(switches::kSilentDumpOnDCHECK));
  UITest::set_disable_breakpad(
      parsed_command_line.HasSwitch(switches::kDisableBreakpad));
  std::wstring test_timeout =
      parsed_command_line.GetSwitchValue(UITestSuite::kTestTimeout);
  if (!test_timeout.empty()) {
    int timeout;
    base::StringToInt(WideToUTF8(test_timeout), &timeout);
    UITest::set_test_timeout_ms(timeout);
  }

#if defined(OS_WIN)
  int batch_count = 0;
  int batch_index = 0;
  std::wstring batch_count_str =
      parsed_command_line.GetSwitchValue(UITestSuite::kBatchCount);
  if (!batch_count_str.empty()) {
    base::StringToInt(WideToUTF16Hack(batch_count_str), &batch_count);
  }
  std::wstring batch_index_str =
      parsed_command_line.GetSwitchValue(UITestSuite::kBatchIndex);
  if (!batch_index_str.empty()) {
    base::StringToInt(WideToUTF16Hack(batch_index_str), &batch_index);
  }
  if (batch_count > 0 && batch_index >= 0 && batch_index < batch_count) {
    // Running UI test in parallel. Gtest supports running tests in shards,
    // and every UI test instance is running with different user data dir.
    // Thus all we need to do is to set up environment variables for gtest.
    // See http://code.google.com/p/googletest/wiki/GoogleTestAdvancedGuide.
    std::string batch_count_env(UITestSuite::kGTestTotalShards);
    batch_count_env.append(WideToASCII(batch_count_str));
    std::string batch_index_env(UITestSuite::kGTestShardIndex);
    batch_index_env.append(WideToASCII(batch_index_str));
    _putenv(batch_count_env.c_str());
    _putenv(batch_index_env.c_str());
  }
#endif

  std::string js_flags =
    parsed_command_line.GetSwitchValueASCII(switches::kJavaScriptFlags);
  if (!js_flags.empty())
    UITest::set_js_flags(js_flags);
  std::string log_level =
    parsed_command_line.GetSwitchValueASCII(switches::kLoggingLevel);
  if (!log_level.empty())
    UITest::set_log_level(log_level);

#if defined(OS_WIN)
  LoadCrashService();
#endif
}

void UITestSuite::Shutdown() {
#if defined(OS_WIN)
  if (crash_service_)
    base::KillProcess(crash_service_, 0, false);
#endif

  ChromeTestSuite::Shutdown();
}

void UITestSuite::SuppressErrorDialogs() {
  TestSuite::SuppressErrorDialogs();
  UITest::set_show_error_dialogs(false);
}

#if defined(OS_WIN)
void UITestSuite::LoadCrashService() {
  scoped_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar(env_vars::kHeadless))
    return;

  if (base::GetProcessCount(L"crash_service.exe", NULL))
    return;

  FilePath exe_dir;
  if (!PathService::Get(base::DIR_EXE, &exe_dir)) {
    DCHECK(false);
    return;
  }

  FilePath crash_service = exe_dir.Append(L"crash_service.exe");
  if (!base::LaunchApp(crash_service.ToWStringHack(), false, false,
                       &crash_service_)) {
    printf("Couldn't start crash_service.exe, so this ui_test run won't tell " \
           "you if any test crashes!\n");
    return;
  }

  printf("Started crash_service.exe so you know if a test crashes!\n");
}
#endif
