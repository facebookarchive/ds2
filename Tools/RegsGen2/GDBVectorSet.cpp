//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "GDBVectorSet.h"
#include "ParseConstants.h"

GDBVectorSet::GDBVectorSet() : _size(0) {}

//
// Parse the gdb-vector-set dictionary, the parsing is done in two steps;
// first collect all the vectors inside the dictionary, then for each vector
// defined check that if a "union" is used, to validate the fields.
//
bool GDBVectorSet::parse(JSDictionary const *d) {
  //
  // Parse the defaults dictionary; the only key present in the
  // defaults dictionary is 'bit-size' that specifies the size
  // of the vector.
  //
  if (auto dflts = d->value<JSDictionary>("*")) {
    if (auto bitsize = dflts->value<JSInteger>("bit-size")) {
      _size = bitsize->value();

      if (_size < 8) {
        fprintf(stderr, "error: vector set specifies a size that is "
                        "less than eight\n");
        return false;
      }

      if (_size & (_size - 1)) {
        fprintf(stderr, "error: vector set specifies a size that is "
                        "not a power of two\n");
        return false;
      }
    }
  }

  //
  // Parse all the fields
  //
  for (auto vname : *d) {
    //
    // Skip defaults dictionary
    //
    if (vname == "*")
      continue;

    if (_map.find(vname) != _map.end()) {
      fprintf(stderr, "error: vector '%s' is already defined\n", vname.c_str());
      return false;
    }

    if (auto vd = d->value<JSDictionary>(vname)) {
      auto encoding = vd->value<JSString>("encoding");
      auto elsize = vd->value<JSInteger>("element-bit-size");

      if (encoding == nullptr) {
        fprintf(stderr, "error: vector '%s' has no encoding type\n",
                vname.c_str());
        return false;
      }

      auto vec = std::make_shared<GDBVector>();

      //
      // Copy defaults
      //
      vec->Name = vname;
      vec->BitSize = _size;

      //
      // Parse and check encoding
      //
      std::string EncodingName;
      if (!ParseGDBEncodingName(encoding->value(), vec->Encoding,
                                EncodingName)) {
        fprintf(stderr, "error: vector '%s' specifies invalid "
                        "GDB encoding '%s'\n",
                vname.c_str(), encoding->value().c_str());
        return false;
      }

      switch (vec->Encoding) {
      case kGDBEncodingInt:
      case kGDBEncodingIEEESingle:
      case kGDBEncodingIEEEDouble:
      case kGDBEncodingUInt128:
      case kGDBEncodingUnion:
        break;

      default:
        fprintf(stderr, "error: vector '%s' specifies an "
                        "unsupported GDB encoding '%s'\n",
                vname.c_str(), encoding->value().c_str());
        return false;
      }

      //
      // IEEE single and double ignores the element size.
      //
      if (vec->Encoding == kGDBEncodingIEEESingle ||
          vec->Encoding == kGDBEncodingIEEEDouble) {
        vec->ElementSize = (vec->Encoding == kGDBEncodingIEEESingle) ? 32 : 64;
        if (elsize != nullptr && elsize->value() != vec->ElementSize) {
          fprintf(stderr, "error: vector '%s' specifies a size of "
                          "%zu bits, while the encoding '%s' is %zu bits "
                          "long\n",
                  vname.c_str(), static_cast<size_t>(elsize->value()),
                  encoding->value().c_str(), vec->ElementSize);
          return false;
        } else {
          //
          // Reset the element size, because the GDB encoding may
          // need to avoid transmitting the size of the element.
          //
          vec->ElementSize = -1;
        }
      } else if (vec->Encoding != kGDBEncodingUnion) {
        if (elsize == nullptr) {
          fprintf(stderr, "error: vector '%s' does not specify the "
                          "element size",
                  vname.c_str());
          return false;
        } else {
          vec->ElementSize = elsize->value();
        }
      } else {
        //
        // This is a union, we need to parse all the fields,
        // the fields are organized by the tuple (union-field-name,
        // vector-name)
        //
        auto uflds = vd->value<JSDictionary>("union-fields");
        if (uflds != nullptr) {
          for (auto fname : *uflds) {
            GDBUnion::Field field;

            field.Name = fname;
            ParseGDBEncodingName(uflds->value<JSString>(fname)->value().c_str(),
                                 field.Encoding, field.EncodingName);

            vec->Union.FieldNames.push_back(field);
          }
        } else {
          fprintf(stderr, "warning: vector '%s' is declared as an "
                          "union but it has no fields\n",
                  vname.c_str());
        }
      }

      _vectors.push_back(vec);
      _map[vec->Name] = vec;
    }
  }

  //
  // TODO Once we have parsed all the vectors, validate element size,
  // we need to this after we parsed all the vector types so that
  // any union size can be computed.
  //

  return true;
}
