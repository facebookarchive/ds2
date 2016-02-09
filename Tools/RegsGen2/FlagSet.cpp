//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "FlagSet.h"
#include "ParseConstants.h"

FlagSet::FlagSet() : _size(0) {}

bool FlagSet::parse(std::string const &name, JSDictionary const *d) {
  //
  // Obtain the set defaults (*)
  //
  auto dflts = d->value<JSDictionary>("*");
  if (dflts == nullptr) {
    fprintf(stderr, "error: flag set '%s' does not specify mandatory "
                    "default entry (*)\n",
            name.c_str());
    return false;
  }

  auto bitsize = dflts->value<JSInteger>("bit-size");
  if (bitsize == nullptr) {
    fprintf(stderr, "error: flag set '%s' does not have mandatory "
                    "bit size in default dictionary\n",
            name.c_str());
    return false;
  }

  if (bitsize->value() == 0 ||
      (bitsize->value() & (bitsize->value() - 1)) != 0) {
    fprintf(stderr, "error: flag set '%s' does specify an invalid "
                    "bit size\n",
            name.c_str());
    return false;
  }

  auto gdbname = dflts->value<JSString>("gdb-name");

  _name = name;
  if (gdbname != nullptr) {
    _GDBName = gdbname->value();
  }
  _size = bitsize->value();

  std::vector<bool> bits(_size);

  //
  // Parse all flags, every entry is a 2 values array
  // containing start and length.
  //
  for (auto e : *d) {
    //
    // Skip over the default dictionary.
    //
    if (e == "*")
      continue;

    auto values = d->value<JSArray>(e);
    if (values == nullptr) {
      fprintf(stderr, "error: flag '%s' in flag set '%s' does not "
                      "specify an array of values\n",
              e.c_str(), name.c_str());
      return false;
    }

    if (values->count() != 2) {
      fprintf(stderr, "error: flag '%s' in flag set '%s' specifies "
                      "an array of %zu values, when expecting 2\n",
              e.c_str(), name.c_str(), values->count());
      return false;
    }

    auto start = values->value<JSInteger>(0);
    auto length = values->value<JSInteger>(1);

    if (start == nullptr || length == nullptr) {
      fprintf(stderr, "error: flag '%s' in flag set '%s' specifies "
                      "an invalid start or length entry, they must be "
                      "integers\n",
              e.c_str(), name.c_str());
      return false;
    }

    if (start->value() >= _size) {
      fprintf(stderr, "error: flag '%s' in flag set '%s' specifies "
                      "an invalid start of %zu, when the maximum size is %zu\n",
              e.c_str(), name.c_str(), static_cast<size_t>(start->value()),
              _size);
      return false;
    }

    if (length->value() == 0) {
      fprintf(stderr, "error: flag '%s' in flag set '%s' specifies "
                      "an invalid length of zero\n",
              e.c_str(), name.c_str());
      return false;
    }

    if (start->value() + length->value() > _size) {
      fprintf(stderr, "error: flag '%s' in flag set '%s' specifies "
                      "an invalid combination of start of %zu and length %zu, "
                      "when the maximum size is %zu\n",
              e.c_str(), name.c_str(), static_cast<size_t>(start->value()),
              static_cast<size_t>(length->value()), _size);
      return false;
    }

    for (ssize_t n = 0; n < length->value(); n++) {
      if (bits[n + start->value()]) {
        fprintf(stderr, "error: flag '%s' in flag set '%s' is "
                        "overlapping with other flags\n",
                e.c_str(), name.c_str());
        return false;
      }
      bits[n + start->value()] = true;
    }

    _flags.push_back(
        std::make_shared<Flag>(e, start->value(), length->value()));
  }

  for (size_t n = 0; n < bits.size(); n++) {
    if (!bits[n]) {
      fprintf(stderr, "warning: flag set '%s' does not specify any flag "
                      "for bit %zu\n",
              name.c_str(), n);
    }
  }

  return true;
}
