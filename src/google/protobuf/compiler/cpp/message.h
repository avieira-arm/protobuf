// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "google/protobuf/compiler/cpp/enum.h"
#include "google/protobuf/compiler/cpp/extension.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/message_layout_helper.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/cpp/parse_function_generator.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
class MessageGenerator {
 public:
  MessageGenerator(
      const Descriptor* descriptor,
      const absl::flat_hash_map<absl::string_view, std::string>& ignored,
      int index_in_file_messages, const Options& options,
      MessageSCCAnalyzer* scc_analyzer);

  MessageGenerator(const MessageGenerator&) = delete;
  MessageGenerator& operator=(const MessageGenerator&) = delete;

  ~MessageGenerator() = default;

  // Append the two types of nested generators to the corresponding vector.
  void AddGenerators(
      std::vector<std::unique_ptr<EnumGenerator>>* enum_generators,
      std::vector<std::unique_ptr<ExtensionGenerator>>* extension_generators);

  // Generate definitions for this class and all its nested types.
  void GenerateClassDefinition(io::Printer* p);

  // Generate definitions of inline methods (placed at the end of the header
  // file).
  void GenerateInlineMethods(io::Printer* p);

  // Generate all non-inline methods for this class.
  void GenerateClassMethods(io::Printer* p);

  // Generate source file code that should go outside any namespace.
  void GenerateSourceInProto2Namespace(io::Printer* p);


  void GenerateInitDefaultSplitInstance(io::Printer* p);

  // Generate the constexpr constructor for constant initialization of the
  // default instance.
  void GenerateConstexprConstructor(io::Printer* p);

  void GenerateSchema(io::Printer* p, int offset, int has_offset);

  // Generate the field offsets array.  Returns the a pair of the total number
  // of entries generated and the index of the first has_bit entry.
  std::pair<size_t, size_t> GenerateOffsets(io::Printer* p);

  const Descriptor* descriptor() const { return descriptor_; }

 private:
  // Generate declarations and definitions of accessors for fields.
  void GenerateFieldAccessorDeclarations(io::Printer* p);
  void GenerateFieldAccessorDefinitions(io::Printer* p);

  // Generate constructors and destructor.
  void GenerateStructors(io::Printer* p);

  // The compiler typically generates multiple copies of each constructor and
  // destructor: http://gcc.gnu.org/bugs.html#nonbugs_cxx
  // Placing common code in a separate method reduces the generated code size.
  //
  // Generate the shared constructor code.
  void GenerateSharedConstructorCode(io::Printer* p);
  // Generate the shared destructor code.
  void GenerateSharedDestructorCode(io::Printer* p);
  // Generate the arena-specific destructor code.
  void GenerateArenaDestructorCode(io::Printer* p);

  // Generate standard Message methods.
  void GenerateClear(io::Printer* p);
  void GenerateOneofClear(io::Printer* p);
  void GenerateVerify(io::Printer* p);
  void GenerateSerializeWithCachedSizes(io::Printer* p);
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p);
  void GenerateSerializeWithCachedSizesBody(io::Printer* p);
  void GenerateSerializeWithCachedSizesBodyShuffled(io::Printer* p);
  void GenerateByteSize(io::Printer* p);
  void GenerateMergeFrom(io::Printer* p);
  void GenerateClassSpecificMergeImpl(io::Printer* p);
  void GenerateCopyFrom(io::Printer* p);
  void GenerateSwap(io::Printer* p);
  void GenerateIsInitialized(io::Printer* p);

  // Helpers for GenerateSerializeWithCachedSizes().
  //
  // cached_has_bit_index maintains that:
  //   cached_has_bits = _has_bits_[cached_has_bit_index]
  // for cached_has_bit_index >= 0
  void GenerateSerializeOneField(io::Printer* p, const FieldDescriptor* field,
                                 int cached_has_bits_index);
  // Generate a switch statement to serialize 2+ fields from the same oneof.
  // Or, if fields.size() == 1, just call GenerateSerializeOneField().
  void GenerateSerializeOneofFields(
      io::Printer* p, const std::vector<const FieldDescriptor*>& fields);
  void GenerateSerializeOneExtensionRange(
      io::Printer* p, const Descriptor::ExtensionRange* range);

  // Generates has_foo() functions and variables for singular field has-bits.
  void GenerateSingularFieldHasBits(const FieldDescriptor* field,
                                    io::Printer* p);
  // Generates has_foo() functions and variables for oneof field has-bits.
  void GenerateOneofHasBits(io::Printer* p);
  // Generates has_foo_bar() functions for oneof members.
  void GenerateOneofMemberHasBits(const FieldDescriptor* field, io::Printer* p);
  // Generates the clear_foo() method for a field.
  void GenerateFieldClear(const FieldDescriptor* field, bool is_inline,
                          io::Printer* p);

  // Returns whether impl_ has a copy ctor.
  bool ImplHasCopyCtor() const;

  // Generates the body of the message's copy constructor.
  void GenerateCopyConstructorBody(io::Printer* p) const;
  void GenerateCopyConstructorBodyImpl(io::Printer* p) const;
  void GenerateCopyConstructorBodyOneofs(io::Printer* p) const;

  // Returns the level that this message needs ArenaDtor. If the message has
  // a field that is not arena-exclusive, it needs an ArenaDtor
  // (go/proto-destructor).
  //
  // - Returning kNone means we don't need to generate ArenaDtor.
  // - Returning kOnDemand means we need to generate ArenaDtor, but don't need
  //   to register ArenaDtor at construction. Such as when the message's
  //   ArenaDtor code is only for destructing inlined string.
  // - Returning kRequired means we meed to generate ArenaDtor and register it
  //   at construction.
  ArenaDtorNeeds NeedsArenaDestructor() const;

  size_t HasBitsSize() const;
  size_t InlinedStringDonatedSize() const;
  absl::flat_hash_map<absl::string_view, std::string> HasBitVars(
      const FieldDescriptor* field) const;
  int HasBitIndex(const FieldDescriptor* field) const;
  int HasByteIndex(const FieldDescriptor* field) const;
  int HasWordIndex(const FieldDescriptor* field) const;
  std::vector<uint32_t> RequiredFieldsBitMask() const;

  const Descriptor* descriptor_;
  int index_in_file_messages_;
  Options options_;
  FieldGeneratorTable field_generators_;
  // optimized_order_ is the order we layout the message's fields in the
  // class. This is reused to initialize the fields in-order for cache
  // efficiency.
  //
  // optimized_order_ excludes oneof fields and weak fields.
  std::vector<const FieldDescriptor*> optimized_order_;
  std::vector<int> has_bit_indices_;
  int max_has_bit_index_ = 0;

  // A map from field index to inlined_string index. For non-inlined-string
  // fields, the element is -1. If there is no inlined string in the message,
  // this is empty.
  std::vector<int> inlined_string_indices_;
  // The count of inlined_string fields in the message.
  int max_inlined_string_index_ = 0;

  std::vector<const EnumGenerator*> enum_generators_;
  std::vector<const ExtensionGenerator*> extension_generators_;
  int num_required_fields_ = 0;
  int num_weak_fields_ = 0;

  std::unique_ptr<MessageLayoutHelper> message_layout_helper_;
  std::unique_ptr<ParseFunctionGenerator> parse_function_generator_;

  MessageSCCAnalyzer* scc_analyzer_;

  absl::flat_hash_map<absl::string_view, std::string> variables_;

};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__
