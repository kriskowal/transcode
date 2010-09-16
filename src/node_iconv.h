// -- kriskowal Kris Kowal Copyright (C) 2010 MIT License
#ifndef SRC_NODE_ICONV_H_
#define SRC_NODE_ICONV_H_

#include <node/node.h>
#include <node/node_object_wrap.h>
#include <v8.h>
#include <iconv.h>

namespace node {

class Transcoder : public ObjectWrap {
 public:
  static void Initialize(v8::Handle<v8::Object> target);
  static inline bool HasInstance(v8::Handle<v8::Value> val) {
    if (!val->IsObject()) return false;
    v8::Local<v8::Object> obj = val->ToObject();
    return constructor_template->HasInstance(obj);
  }

  iconv_t descriptor() const { return descriptor_; }

 protected:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
  static v8::Handle<v8::Value> New(const v8::Arguments &args); // source, target
  static v8::Handle<v8::Value> Transcode(const v8::Arguments &args); // source, target, start, stop, start, stop
  //static v8::Handle<v8::Value> Reset(const v8::Arguments &args); // source, target, start, stop, start, stop
  static v8::Handle<v8::Value> Close(const v8::Arguments &args);

  iconv_t descriptor_;
};

}

#endif // SRC_NODE_ICONV_H_
