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

#include "syzygy/refinery/types/pdb_crawler.h"

#include "syzygy/common/align.h"
#include "syzygy/pdb/pdb_reader.h"
#include "syzygy/pdb/pdb_type_info_stream_enum.h"
#include "syzygy/pdb/gen/pdb_type_info_records.h"
#include "syzygy/pe/cvinfo_ext.h"
#include "syzygy/refinery/types/type.h"
#include "syzygy/refinery/types/type_repository.h"

// Declaration of struct used for the unknown records. This is needed because
// the macro LEAF_CASE_TABLE in ParseType generates call to Reader<UnknownLeaf>
// in some cases. But the CVInfo.h header has no declaration of this struct
// which would prevent the compilation.
namespace Microsoft_Cci_Pdb {
struct UnknownLeaf;
}

namespace refinery {

namespace {

namespace cci = Microsoft_Cci_Pdb;

struct TempType {
  TempType(const base::string16& n,
           const base::string16& dn,
           TypeId t,
           Type::Flags f)
      : name(n), decorated_name(dn), type_id(t), flags(f) {}

  bool is_const() const { return (flags & Type::FLAG_CONST) != 0; }
  bool is_volatile() const { return (flags & Type::FLAG_VOLATILE) != 0; }

  // CV flags.
  Type::Flags flags;

  // Name of the type.
  base::string16 name;

  // Decorated name of the type.
  base::string16 decorated_name;

  // Type Id of the closest element on the way down in the type repository.
  TypeId type_id;
};

class TypeCreator {
 public:
  explicit TypeCreator(TypeRepository* repository);
  ~TypeCreator();

  // Crawls @p stream, creates all types and assigns names to pointers.
  bool CreateTypes(scoped_refptr<pdb::PdbStream> stream);

 private:
  // Parses current type info record given its type.
  // @param type_creator pointer to the TypeCreator object which called this
  //   function.
  // @returns true on success, false on failure.
  template <typename T>
  bool ReadType() {
    return true;
  }

  // Parses type given by a type from the PDB type info stream.
  // @returns true on success, false on failure.
  bool ParseType(uint16_t type);

  // Checks if type object referenced by @p type_index exists.
  // @returns pointer to the type object.
  TempType* FindOrCreateTempType(TypeId type_index);

  // Creates a basic type object given its @p type_index (when needed).
  // @returns pointer to the type object.
  TempType* CreateBasicType(TypeId type_index);

  // Adds temporary type to the temporary hash.
  TempType* AddTempType(TypeId type_index,
                        const base::string16& name,
                        const base::string16& decorated_name,
                        TypeId type_id,
                        Type::Flags flags);

  // @returns name for a basic type specified by its @ type.
  static base::string16 BasicTypeName(uint32 type);

  // @returns size for a basic type specified by its @ type.
  static size_t BasicTypeSize(uint32 type);

  // @returns size of a member field pointer given its @p ptrmode and @p
  // ptrtype.
  static size_t MemberPointerSize(cci::CV_pmtype ptrmode,
                                  cci::CV_ptrtype ptrtype);

  // @returns the string of CV modifiers.
  static base::string16 GetCVMod(Type::Flags flags);

  // @returns the CV_prmode of the given basic type index.
  static cci::CV_prmode TypeIndexToPrMode(TypeId type_index);

  // Pointer to a type info repository.
  TypeRepository* repository_;

  // Type info enumerator used to transverse the stream.
  pdb::TypeInfoEnumerator type_info_enum_;

  // Temporary types hash.
  base::hash_map<TypeId, TempType*> temp_stash_;
};

// Parses pointers from the type info stream.
template <>
bool TypeCreator::ReadType<cci::LeafPointer>() {
  pdb::LeafPointer type_info;
  scoped_refptr<pdb::PdbStream> stream = type_info_enum_.GetDataStream();
  if (!type_info.Initialize(stream.get())) {
    LOG(ERROR) << "Unable to read type info record.";
    return false;
  }

  const cci::CV_ptrtype ptrtype = (cci::CV_ptrtype)(type_info.attr().ptrtype);

  size_t size = 0;
  // Set the size of the pointer.
  switch (type_info.attr().ptrmode) {
    // The size of a regular pointer or reference can be deduced from its type.
    // TODO(mopler): Investigate references.
    case cci::CV_PTR_MODE_PTR:
    case cci::CV_PTR_MODE_REF: {
      if (ptrtype == cci::CV_PTR_NEAR32)
        size = 4;
      else if (ptrtype == cci::CV_PTR_64)
        size = 8;
      break;
    }
    // However in case of a member field pointer, its size depends on the
    // properties of the containing class. The pointer contains extra
    // information about the containing class.
    case cci::CV_PTR_MODE_PMFUNC:
    case cci::CV_PTR_MODE_PMEM: {
      cci::CV_pmtype ptrmode = cci::CV_PMTYPE_Undef;

      // Skip the type index of the containing class and read the pointer mode.
      if (!stream->Seek(stream->pos() + 4) || !stream->Read(&ptrmode, 1)) {
        LOG(ERROR) << "Unable to read the member field pointer mode.";
        return false;
      }
      size = MemberPointerSize(ptrmode, ptrtype);
      break;
    }
    case cci::CV_PTR_MODE_RESERVED: {
      // We shouldn't encounter reserved pointer mode.
      return false;
    }
  }

  // Try to find the object in the repository.
  TempType* child = FindOrCreateTempType(type_info.body().utype);
  if (child == nullptr)
    return false;

  // TODO(mopler): Different names for member data and member function pointers.
  base::string16 name = child->name + L"*";
  base::string16 decorated_name = child->decorated_name + L"*";
  TypeId type_id = type_info_enum_.type_id();

  Type::Flags flags = kNoTypeFlags;
  if (type_info.attr().isconst)
    flags |= Type::FLAG_CONST;
  if (type_info.attr().isvolatile)
    flags |= Type::FLAG_VOLATILE;

  name += GetCVMod(flags);
  decorated_name += GetCVMod(flags);

  PointerTypePtr created =
      new PointerType(child->name + L"*", child->decorated_name + L"*", size);

  // Setting the flags from the child node - this is needed because of
  // different semantics between PDB file and Type interface. In PDB pointer
  // has a const flag when it's const, while here pointer has a const flag if
  // it points to a const type.
  created->Finalize(child->flags, child->type_id);

  repository_->AddTypeWithId(created, type_id);

  // Add to temporary stash
  AddTempType(type_info_enum_.type_id(), name, decorated_name, type_id, flags);
  return true;
}

// Parses modifiers from the type info stream.
template <>
bool TypeCreator::ReadType<cci::LeafModifier>() {
  pdb::LeafModifier type_info;
  scoped_refptr<pdb::PdbStream> stream = type_info_enum_.GetDataStream();
  if (!type_info.Initialize(stream.get())) {
    LOG(ERROR) << "Unable to read type info record.";
    return false;
  }

  TempType* child = FindOrCreateTempType(type_info.body().type);
  if (child == nullptr)
    return false;

  Type::Flags flags = kNoTypeFlags;
  if (type_info.attr().mod_const)
    flags |= Type::FLAG_CONST;
  if (type_info.attr().mod_volatile)
    flags |= Type::FLAG_VOLATILE;

  AddTempType(type_info_enum_.type_id(), child->name + GetCVMod(flags),
              child->decorated_name + GetCVMod(flags), child->type_id, flags);
  return true;
}

// TODO(mopler): Add template specialization for more leaves.

TypeCreator::TypeCreator(TypeRepository* repository) : repository_(repository) {
  DCHECK(repository);
}

TypeCreator::~TypeCreator() {
  for (auto it = temp_stash_.begin(); it != temp_stash_.end(); ++it)
    delete it->second;
}

base::string16 TypeCreator::BasicTypeName(uint32 type) {
  switch (type) {
// Just return the name of the enum.
#define SPECIAL_TYPE_NAME(record_type, type_name, size) \
  case cci::record_type: return L#type_name;
    SPECIAL_TYPE_NAME_CASE_TABLE(SPECIAL_TYPE_NAME)
#undef SPECIAL_TYPE_NAME
  }
  return L"unknown_basic_type";
}

size_t TypeCreator::BasicTypeSize(uint32 type) {
  switch (type) {
    // Just return the name of the enum.
#define SPECIAL_TYPE_NAME(record_type, type_name, size) \
  case cci::record_type: return size;
    SPECIAL_TYPE_NAME_CASE_TABLE(SPECIAL_TYPE_NAME)
#undef SPECIAL_TYPE_NAME
  }
  return 0;
}

size_t TypeCreator::MemberPointerSize(cci::CV_pmtype ptrmode,
                                      cci::CV_ptrtype ptrtype) {
  DCHECK(ptrtype == cci::CV_PTR_NEAR32 || ptrtype == cci::CV_PTR_64);

  // The translation of modes to pointer sizes depends on the compiler. The
  // following values have been determined experimentally. For details see
  // https://github.com/google/syzygy/wiki/MemberPointersInPdbFiles.
  if (ptrtype == cci::CV_PTR_NEAR32) {
    switch (ptrmode) {
      case cci::CV_PMTYPE_Undef:
        return 0;
      case cci::CV_PMTYPE_D_Single:
        return 4;
      case cci::CV_PMTYPE_D_Multiple:
        return 4;
      case cci::CV_PMTYPE_D_Virtual:
        return 8;
      case cci::CV_PMTYPE_D_General:
        return 12;
      case cci::CV_PMTYPE_F_Single:
        return 4;
      case cci::CV_PMTYPE_F_Multiple:
        return 8;
      case cci::CV_PMTYPE_F_Virtual:
        return 12;
      case cci::CV_PMTYPE_F_General:
        return 16;
    }
  } else if (ptrtype == cci::CV_PTR_64) {
    switch (ptrmode) {
      case cci::CV_PMTYPE_Undef:
        return 0;
      case cci::CV_PMTYPE_D_Single:
        return 4;
      case cci::CV_PMTYPE_D_Multiple:
        return 4;
      case cci::CV_PMTYPE_D_Virtual:
        return 8;
      case cci::CV_PMTYPE_D_General:
        return 12;
      case cci::CV_PMTYPE_F_Single:
        return 8;
      case cci::CV_PMTYPE_F_Multiple:
        return 16;
      case cci::CV_PMTYPE_F_Virtual:
        return 16;
      case cci::CV_PMTYPE_F_General:
        return 24;
    }
  }
  // It seems that VS doesn't use the other pointer types in PDB files.
  return 0;
}

base::string16 TypeCreator::GetCVMod(Type::Flags flags) {
  base::string16 suffix;
  if (flags & Type::FLAG_CONST) {
    suffix += L" const";
  }
  if (flags & Type::FLAG_VOLATILE) {
    flags |= Type::FLAG_VOLATILE;
    suffix += L" volatile";
  }
  return suffix;
}

TempType* TypeCreator::AddTempType(TypeId type_index,
                                   const base::string16& name,
                                   const base::string16& decorated_name,
                                   TypeId type_id,
                                   Type::Flags flags) {
  TempType* temp_pointer = new TempType(name, decorated_name, type_id, flags);
  bool inserted =
      temp_stash_.insert(std::make_pair(type_index, temp_pointer)).second;
  DCHECK(inserted);

  return temp_pointer;
}

cci::CV_prmode TypeCreator::TypeIndexToPrMode(TypeId type_index) {
  return static_cast<cci::CV_prmode>(
      (type_index & cci::CV_PRIMITIVE_TYPE::CV_MMASK) >>
      cci::CV_PRIMITIVE_TYPE::CV_MSHIFT);
}

TempType* TypeCreator::CreateBasicType(TypeId type_index) {
  // Check if we are dealing with pointer.
  cci::CV_prmode prmode = TypeIndexToPrMode(type_index);
  if (prmode == cci::CV_TM_DIRECT) {
    // Create and add type to the repository.
    BasicTypePtr created =
        new BasicType(BasicTypeName(type_index), BasicTypeSize(type_index));
    bool inserted = repository_->AddTypeWithId(created, type_index);
    DCHECK(inserted);

    // Create temporary type to temporary stash.
    return AddTempType(type_index, BasicTypeName(type_index),
                       BasicTypeName(type_index), type_index,
                       BasicTypeSize(type_index));
  } else {
    TypeId basic_index = type_index & (cci::CV_PRIMITIVE_TYPE::CV_TMASK |
                                       cci::CV_PRIMITIVE_TYPE::CV_SMASK);
    TempType* child = FindOrCreateTempType(basic_index);
    if (child == nullptr)
      return false;

    // Get pointer size.
    size_t size = 0;
    switch (prmode) {
      case cci::CV_TM_NPTR32:
        size = 4;
        break;
      case cci::CV_TM_NPTR64:
        size = 8;
        break;
      case cci::CV_TM_NPTR128:
        size = 16;
        break;
      default:
        return nullptr;
    }

    // Create and add type to the repository.
    PointerTypePtr created = new PointerType(size);
    created->Finalize(0x0, basic_index);
    created->SetName(child->name + L"*");
    bool inserted = repository_->AddTypeWithId(created, type_index);
    DCHECK(inserted);

    // Create temporary type to temporary stash.
    return AddTempType(type_index, child->name + L"*",
                       child->decorated_name + L"*", type_index, kNoTypeFlags);
  }
}

TempType* TypeCreator::FindOrCreateTempType(TypeId type_index) {
  auto it = temp_stash_.find(type_index);
  if (it != temp_stash_.end())
    return it->second;

  // Check if it is a special type index.
  if (type_index >= type_info_enum_.type_info_header().type_min) {
    // For now returning wildcard dummy.
    // TODO(mopler): return nullptr here once we implement the other types.
    return AddTempType(type_index, L"dummy", L"dummy", type_index,
                       kNoTypeFlags);
  } else {
    // Construct the needed basic type.
    return CreateBasicType(type_index);
  }
}

bool TypeCreator::ParseType(uint16_t type) {
  switch (type) {
// Call the template reader to process the current record.
#define LEAF_TYPE_DUMP(type_value, struct_type) \
  case cci::type_value:                         \
    return ReadType<cci::##struct_type>();
    LEAF_CASE_TABLE(LEAF_TYPE_DUMP)
#undef LEAF_TYPE_DUMP
    default:
      return false;
  }
}

bool TypeCreator::CreateTypes(scoped_refptr<pdb::PdbStream> stream) {
  DCHECK(stream);

  if (!type_info_enum_.Init(stream.get())) {
    LOG(ERROR) << "Unable to initialize type info stream enumerator.";
    return false;
  }

  const TypeId kSmallestUnreservedIndex = 0x1000;
  if (type_info_enum_.type_info_header().type_min < kSmallestUnreservedIndex) {
    LOG(ERROR) << "Degenerate stream with type indices in the reserved range.";
    return false;
  }

  while (!type_info_enum_.EndOfStream()) {
    if (!type_info_enum_.NextTypeInfoRecord()) {
      LOG(ERROR) << "Unable to load next type info record.";
      return false;
    }

    if (!ParseType(type_info_enum_.type())) {
      LOG(ERROR) << "Unable to parse type info stream.";
      return false;
    }
  }

  return true;
}

}  // namespace

PdbCrawler::PdbCrawler() {
}

PdbCrawler::~PdbCrawler() {
}

bool PdbCrawler::InitializeForFile(const base::FilePath& path) {
  pdb::PdbReader reader;
  pdb::PdbFile pdb_file;

  if (!reader.Read(path, &pdb_file)) {
    LOG(ERROR) << "Failed to read PDB file " << path.value() << ".";
    return false;
  }

  stream_ = pdb_file.GetStream(pdb::kTpiStream);
  return true;
}

bool PdbCrawler::GetTypes(TypeRepository* types) {
  DCHECK(types);
  DCHECK(stream_);

  TypeCreator creator(types);

  return creator.CreateTypes(stream_);
}

}  // namespace refinery
