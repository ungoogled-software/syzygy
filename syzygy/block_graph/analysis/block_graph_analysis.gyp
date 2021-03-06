# Copyright 2013 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    '../../syzygy.gypi',
  ],
  'targets': [
    {
      'target_name': 'block_graph_analysis_lib',
      'type': 'static_library',
      'sources': [
        'control_flow_analysis.cc',
        'control_flow_analysis.h',
        'liveness_analysis.cc',
        'liveness_analysis.h',
        'liveness_analysis_internal.h',
        'memory_access_analysis.cc',
        'memory_access_analysis.h',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(src)/syzygy/block_graph/block_graph.gyp:block_graph_lib',
        '<(src)/syzygy/common/common.gyp:common_lib',
        '<(src)/syzygy/core/core.gyp:core_lib',
      ],
    },
  ],
}
