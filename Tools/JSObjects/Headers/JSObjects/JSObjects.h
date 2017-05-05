//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __JSObjects_h
#define __JSObjects_h

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

class JSObject {
public:
  typedef std::unique_ptr<JSObject> UniquePtr;

public:
  enum Type {
    kTypeInteger,
    kTypeReal,
    kTypeString,
    kTypeBoolean,
    kTypeNull,
    kTypeArray,
    kTypeDictionary
  };

protected:
  JSObject() {}
  JSObject(JSObject const &rhs) = delete;
  JSObject(JSObject &&rhs) = delete;
  JSObject &operator=(JSObject const &rhs) = delete;
  JSObject &operator=(JSObject &&rhs) = delete;

public:
  virtual ~JSObject() {}

public:
  virtual Type type() const = 0;

public:
  virtual bool equals(JSObject const *obj) const { return (obj == this); }

public:
  //
  // dict.key
  // array[index]
  //
  JSObject const *traverse(std::string const &path) const;

  template <typename T> inline T const *traverse(std::string const &path) const;

public:
  void dump(FILE *fp = stderr, size_t indent = 0) const;
  std::string toString() const;

protected:
  static inline void Indent(FILE *fp, size_t n);
  static inline std::string QuoteString(std::string const &s);

protected:
  friend class JSArray;
  friend class JSDictionary;
  virtual void dump1(FILE *fp, size_t indent, size_t cindent) const = 0;
  virtual std::string toString1() const = 0;
};

template <typename T> static inline T *JSCastTo(JSObject *obj) {
  return (obj != nullptr && obj->type() == T::JSType()) ? static_cast<T *>(obj)
                                                        : nullptr;
}

template <typename T> static inline T const *JSCastTo(JSObject const *obj) {
  return (obj != nullptr && obj->type() == T::JSType())
             ? static_cast<T const *>(obj)
             : nullptr;
}

template <typename T>
inline T const *JSObject::traverse(std::string const &path) const {
  return JSCastTo<T>(traverse(path));
}

class JSInteger : public JSObject {
private:
  int64_t _value;

public:
  JSInteger(int64_t value = 0) : _value(value) {}

  inline int64_t value() const { return _value; }

public:
  inline static JSInteger *New(int64_t value) { return new JSInteger(value); }

public:
  JSObject::Type type() const override { return JSInteger::JSType(); }

  static JSObject::Type JSType() { return JSObject::kTypeInteger; }

public:
  bool equals(JSObject const *obj) const override {
    if (JSObject::equals(obj))
      return true;

    JSInteger const *objt = JSCastTo<JSInteger>(obj);
    return (objt != nullptr && equals(objt));
  }

  virtual bool equals(JSInteger const *obj) const {
    return (obj != nullptr && (obj == this || value() == obj->value()));
  }

protected:
  void dump1(FILE *fp, size_t indent, size_t) const override;
  std::string toString1() const override;
};

class JSReal : public JSObject {
private:
  double _value;

public:
  JSReal(double value = 0.0) : _value(value) {}

  inline double value() const { return _value; }

public:
  inline static JSReal *New(double value) { return new JSReal(value); }

public:
  JSObject::Type type() const override { return JSReal::JSType(); }

  static JSObject::Type JSType() { return JSObject::kTypeReal; }

public:
  bool equals(JSObject const *obj) const override {
    if (JSObject::equals(obj))
      return true;

    JSReal const *objt = JSCastTo<JSReal>(obj);
    return (objt != nullptr && equals(objt));
  }

  virtual bool equals(JSReal const *obj) const {
    if (obj == nullptr) {
      return false;
    }

    return obj == this || std::fabs(value() - obj->value()) <=
                              std::numeric_limits<decltype(value())>::epsilon();
  }

protected:
  void dump1(FILE *fp, size_t indent, size_t) const override;
  std::string toString1() const override;
};

class JSString : public JSObject {
private:
  std::string _value;

public:
  JSString(std::string const &value = nullptr) : _value(value) {}

  inline std::string const &value() const { return _value; }

public:
  inline static JSString *New(std::string const &value) {
    return new JSString(value);
  }

public:
  JSObject::Type type() const override { return JSString::JSType(); }

  static JSObject::Type JSType() { return JSObject::kTypeString; }

public:
  bool equals(JSObject const *obj) const override {
    if (JSObject::equals(obj))
      return true;

    JSString const *objt = JSCastTo<JSString>(obj);
    return (objt != nullptr && equals(objt));
  }

  virtual bool equals(JSString const *obj) const {
    return (obj != nullptr && (obj == this || value() == obj->value()));
  }

protected:
  void dump1(FILE *fp, size_t indent, size_t) const override;
  std::string toString1() const override;
};

class JSBoolean : public JSObject {
private:
  bool _value;

private:
  JSBoolean(bool value) : _value(value) {}

private:
  static void *operator new(size_t) noexcept { return nullptr; }

  static void operator delete(void *) {}

public:
  inline static JSBoolean *New(bool value) {
    static JSBoolean const kTrue(true);
    static JSBoolean const kFalse(false);

    return const_cast<JSBoolean *>(value ? &kTrue : &kFalse);
  }

public:
  inline bool value() const { return _value; }

public:
  JSObject::Type type() const override { return JSBoolean::JSType(); }

  static JSObject::Type JSType() { return JSObject::kTypeBoolean; }

public:
  bool equals(JSObject const *obj) const override {
    if (JSObject::equals(obj))
      return true;

    JSBoolean const *objt = JSCastTo<JSBoolean>(obj);
    return (objt != nullptr && equals(objt));
  }

  virtual bool equals(JSBoolean const *obj) const {
    return (obj != nullptr && (obj == this || value() == obj->value()));
  }

protected:
  void dump1(FILE *fp, size_t indent, size_t) const override;
  std::string toString1() const override;
};

class JSNull : public JSObject {
private:
  JSNull() {}

private:
  static void *operator new(size_t) noexcept { return nullptr; }

  static void operator delete(void *) {}

public:
  inline static JSNull *New() {
    static JSNull const kNull;
    return const_cast<JSNull *>(&kNull);
  }

public:
  JSObject::Type type() const override { return JSNull::JSType(); }

  static JSObject::Type JSType() { return JSObject::kTypeNull; }

public:
  bool equals(JSObject const *obj) const override {
    if (JSObject::equals(obj))
      return true;

    JSNull const *objt = JSCastTo<JSNull>(obj);
    return (objt != nullptr && equals(objt));
  }

  virtual bool equals(JSNull const *obj) const {
    return (obj != nullptr && obj == this);
  }

protected:
  void dump1(FILE *fp, size_t indent, size_t) const override;
  std::string toString1() const override;
};

class JSArray : public JSObject {
public:
  typedef std::vector<JSObject::UniquePtr> Vector;

private:
  Vector _array;

public:
  JSArray() {}

public:
  inline static JSArray *New() { return new JSArray; }

public:
  inline bool empty() const { return _array.empty(); }

  inline size_t count() const { return _array.size(); }

  inline JSObject const *value(size_t index) const {
    return _array[index].get();
  }

  template <typename T> inline T const *value(size_t index) const {
    return JSCastTo<T>(value(index));
  }

public:
  inline void append(JSObject *obj) {
    _array.push_back(UniquePtr(obj));
  }

public:
  inline Vector::const_iterator begin() const { return _array.begin(); }

  inline Vector::const_iterator end() const { return _array.end(); }

public:
  JSObject::Type type() const override { return JSArray::JSType(); }

  static JSObject::Type JSType() { return JSObject::kTypeArray; }

public:
  bool equals(JSObject const *obj) const override {
    if (JSObject::equals(obj))
      return true;

    JSArray const *objt = JSCastTo<JSArray>(obj);
    return (objt != nullptr && equals(objt));
  }

  virtual bool equals(JSArray const *obj) const {
    if (obj == nullptr)
      return false;

    if (count() != obj->count())
      return false;

    for (size_t n = 0; n < count(); n++) {
      if (!value(n)->equals(obj->value(n)))
        return false;
    }

    return true;
  }

public:
  void dump1(FILE *fp, size_t, size_t cindent) const override;
  std::string toString1() const override;
};

class JSDictionary : public JSObject {
private:
  typedef std::vector<std::string> KeyVector;
  typedef std::multimap<std::string, JSObject::UniquePtr> Map;

private:
  KeyVector _keys;
  Map _map;

public:
  JSDictionary() {}

public:
  inline static JSDictionary *New() { return new JSDictionary; }

public:
  inline bool empty() const { return _map.empty(); }

  inline size_t count() const { return _map.size(); }

  inline JSObject const *value(size_t index) const {
    return (index < _keys.size()) ? value(_keys[index]) : nullptr;
  }

  template <typename T> inline T const *value(size_t index) const {
    return JSCastTo<T>(value(index));
  }

  inline JSObject const *value(std::string const &key) const {
    Map::const_iterator i = _map.find(key);
    return (i != _map.end()) ? i->second.get() : nullptr;
  }

  template <typename T> inline T const *value(std::string const &key) const {
    return JSCastTo<T>(value(key));
  }

public:
  inline void set(std::string const &key, JSObject *obj) {
    if (_map.find(key) == _map.end()) {
      _keys.push_back(key);
    }
    _map.insert(std::make_pair(key, UniquePtr(obj)));
  }

public:
  inline KeyVector::const_iterator begin() const { return _keys.begin(); }

  inline KeyVector::const_iterator end() const { return _keys.end(); }

public:
  JSObject::Type type() const override { return JSDictionary::JSType(); }

  static JSObject::Type JSType() { return JSObject::kTypeDictionary; }

public:
  bool equals(JSObject const *obj) const override {
    if (JSObject::equals(obj))
      return true;

    JSDictionary const *objt = JSCastTo<JSDictionary>(obj);
    return (objt != nullptr && equals(objt));
  }

  virtual bool equals(JSDictionary const *obj) const {
    if (obj == nullptr)
      return false;

    if (count() != obj->count())
      return false;

    for (Map::const_iterator i = _map.begin(); i != _map.end(); ++i) {
      if (!value(i->first)->equals(obj->value(i->first)))
        return false;
    }

    return true;
  }

protected:
  void dump1(FILE *fp, size_t, size_t cindent) const override;
  std::string toString1() const override;

public:
  static JSDictionary *Parse(std::string const &path);
  static JSDictionary *
  Parse(std::string const &path,
        std::function<bool(unsigned, unsigned, std::string const &)> const
            &error);

  static JSDictionary *Parse(FILE *fp);
  static JSDictionary *
  Parse(FILE *fp, std::function<bool(unsigned, unsigned,
                                     std::string const &)> const &error);
};

#endif // !__JSObjects_h
