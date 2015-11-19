// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SYZYGY_REFINERY_ANALYZERS_THREAD_ANALYZER_H_
#define SYZYGY_REFINERY_ANALYZERS_THREAD_ANALYZER_H_

#include "base/macros.h"
#include "syzygy/refinery/analyzers/analyzer.h"

namespace refinery {

// The thread analyzer populates the Stack layer from thread information in the
// minidump.
class ThreadAnalyzer : public Analyzer {
 public:
  ThreadAnalyzer() {}
  const char* name() const override { return kThreadAnalyzerName; }

  AnalysisResult Analyze(const minidump::Minidump& minidump,
                         ProcessState* process_state) override;

 private:
  static const char kThreadAnalyzerName[];
  DISALLOW_COPY_AND_ASSIGN(ThreadAnalyzer);
};

}  // namespace refinery

#endif  // SYZYGY_REFINERY_ANALYZERS_THREAD_ANALYZER_H_
