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

#ifndef SYZYGY_PDB_GEN_PDB_TYPE_INFO_RECORDS_H_
#define SYZYGY_PDB_GEN_PDB_TYPE_INFO_RECORDS_H_

#include "base/strings/string16.h"
#include "syzygy/pdb/pdb_stream_record.h"
#include "syzygy/pe/cvinfo_ext.h"

namespace pdb {

// Forward declaration.
class PdbStream;

class LeafClass {
 public:
  LeafClass();

  // @name Accessors.
  // @{
  const Microsoft_Cci_Pdb::LeafClass& body() { return body_; }
  uint64_t size() const { return size_; }
  const base::string16& name() const { return name_; }
  const base::string16& decorated_name() const { return decorated_name_; }
  LeafPropertyField property() const { return {body_.property}; }
  // @}

  bool has_decorated_name() const {
    return property().decorated_name_present != 0;
  }

  // Initializes the class from the given pdb stream.
  // @param stream pointer to the pdb stream.
  // @returns true on success, false on failure.
  bool Initialize(PdbStream* stream);

 private:
  // The struct from CVInfo.h which represents this record.
  Microsoft_Cci_Pdb::LeafClass body_;

  // Additional fields parsed from the pdb stream.
  uint64_t size_;
  base::string16 name_;
  base::string16 decorated_name_;
};

class LeafModifier {
 public:
  LeafModifier();

  // @name Accessors.
  // @{
  const Microsoft_Cci_Pdb::LeafModifier& body() { return body_; }
  LeafModifierAttribute attr() const { return attr_; }
  // @}

  // Initializes the class from the given pdb stream.
  // @param stream pointer to the pdb stream.
  // @returns true on success, false on failure.
  bool Initialize(PdbStream* stream);

 private:
  // The struct from CVInfo.h which represents this record.
  Microsoft_Cci_Pdb::LeafModifier body_;

  // Additional fields parsed from the pdb stream.
  LeafModifierAttribute attr_;
};

class LeafPointer {
 public:
  LeafPointer();

  // @name Accessors.
  // @{
  const Microsoft_Cci_Pdb::LeafPointer::LeafPointerBody& body() { return body_; }
  LeafPointerAttribute attr() const { return attr_; }
  // @}

  // Initializes the class from the given pdb stream.
  // @param stream pointer to the pdb stream.
  // @returns true on success, false on failure.
  bool Initialize(PdbStream* stream);

 private:
  // The struct from CVInfo.h which represents this record.
  Microsoft_Cci_Pdb::LeafPointer::LeafPointerBody body_;

  // Additional fields parsed from the pdb stream.
  LeafPointerAttribute attr_;
};

}  // namespace pdb

#endif  // SYZYGY_PDB_GEN_PDB_TYPE_INFO_RECORDS_H_
