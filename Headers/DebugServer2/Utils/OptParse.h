//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

#include <map>
#include <string>
#include <vector>

namespace ds2 {

class OptParse {
public:
  enum OptionType {
    boolOption,
    stringOption,
    vectorOption,
  };

public:
  void addOption(OptionType type, std::string const &name, char shortName,
                 std::string const &help = std::string(), bool hidden = false);

public:
  int parse(int argc, char **argv);

public:
  bool getBool(std::string const &name) const;
  std::string const &getString(std::string const &name) const;
  std::vector<std::string> const &getVector(std::string const &name) const;

public:
  std::string const &getHost() const;
  std::string const &getPort() const;

public:
  void usageDie(std::string const &message = std::string());

private:
  struct OptionStorage {
    char shortName;
    OptionType type;
    struct {
      bool boolValue;
      std::string stringValue;
      std::vector<std::string> vectorValue;
    } values;
    std::string help;
    bool hidden;
  };

  typedef std::map<std::string, OptionStorage> OptionCollection;

private:
  OptionCollection _options;
  std::string _host;
  std::string _port;

private:
  OptionCollection::iterator findShortOpt(char shortOption);
  OptionStorage const &get(std::string const &name, OptionType type) const;
};
} // namespace ds2
