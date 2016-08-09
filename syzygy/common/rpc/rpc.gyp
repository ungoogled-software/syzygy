# Copyright 2014 Google Inc. All Rights Reserved.
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
      'target_name': 'common_rpc_lib',
      'type': 'static_library',
      'sources': [
        'helpers.cc',
        'helpers.h',
        'mem.cc',
      ],
    },
    {
      'target_name': 'common_rpc_unittests',
      'type': 'executable',
      'sources': [
        'helpers_unittest.cc',
        '<(src)/syzygy/testing/run_all_unittests.cc',
      ],
      'dependencies': [
        'common_rpc_lib',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:test_support_base',
        #'<(src)/testing/gtest.gyp:gtest',
      ],
    },
  ],
}
