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

// This file is generated by generate_type_info_records.py, DO NOT MODIFY.

#include "syzygy/pdb/gen/pdb_type_info_records.h"
#include "syzygy/pdb/pdb_stream.h"

namespace pdb {

LeafClass::LeafClass() {
  ::memset(&body_, 0, sizeof(body_));
}

bool LeafClass::Initialize(PdbStream* stream) {
  size_t to_read = offsetof(Microsoft_Cci_Pdb::LeafClass, data);
  size_t bytes_read = 0;
  if (!stream->ReadBytes(&body_, to_read, &bytes_read) ||
      bytes_read != to_read) {
    return false;
  }
  if (!ReadUnsignedNumeric(stream, &size_))
    return false;
  if (!ReadWideString(stream, &name_))
    return false;
  if (property().decorated_name_present != 0 &&
      !ReadWideString(stream, &decorated_name_)) {
    return false;
  }

  return true;
}

LeafModifier::LeafModifier() {
  ::memset(&body_, 0, sizeof(body_));
}

bool LeafModifier::Initialize(PdbStream* stream) {
  size_t to_read = offsetof(Microsoft_Cci_Pdb::LeafModifier, attr);
  size_t bytes_read = 0;
  if (!stream->ReadBytes(&body_, to_read, &bytes_read) ||
      bytes_read != to_read) {
    return false;
  }
  if (!ReadBasicType(stream, &attr_))
    return false;

  return true;
}

LeafPointer::LeafPointer() {
  ::memset(&body_, 0, sizeof(body_));
}

bool LeafPointer::Initialize(PdbStream* stream) {
  size_t to_read = offsetof(Microsoft_Cci_Pdb::LeafPointer::LeafPointerBody, attr);
  size_t bytes_read = 0;
  if (!stream->ReadBytes(&body_, to_read, &bytes_read) ||
      bytes_read != to_read) {
    return false;
  }
  if (!ReadBasicType(stream, &attr_))
    return false;

  return true;
}

}  // namespace pdb
