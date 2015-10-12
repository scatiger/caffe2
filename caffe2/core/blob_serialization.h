#ifndef CAFFE2_CORE_BLOB_SERIALIZATION_H_
#define CAFFE2_CORE_BLOB_SERIALIZATION_H_

#include "caffe2/core/blob.h"
#include "caffe2/core/typeid.h"

namespace caffe2 {

// BlobSerializerBase is a class that serializes a blob to a string. This class
// exists purely for the purpose of registering type-specific serialization
// code.
class BlobSerializerBase {
 public:
  virtual string Serialize(const Blob& blob, const string& name) = 0;
};

// THe Blob serialization registry and serializer creator functions.
DECLARE_TYPED_REGISTRY(BlobSerializerRegistry, CaffeTypeId,
                       BlobSerializerBase);
#define REGISTER_BLOB_SERIALIZER(name, id, ...) \
  REGISTER_TYPED_CLASS(BlobSerializerRegistry, name, id, __VA_ARGS__)
// Creates an operator with the given operator definition.
inline BlobSerializerBase* CreateSerializer(CaffeTypeId id) {
  return BlobSerializerRegistry()->Create(id);
}

// TensorSerializer is the serializer for Tensors: it puts the tensor object
// into a TensorProto protocol buffer.
template <class Context>
class TensorSerializer : public BlobSerializerBase {
 public:
  TensorSerializer() : device_context_() {}
  ~TensorSerializer() {}
  string Serialize(const Blob& blob, const string& name);

 private:
  Context device_context_;
};

////////////////////////////////////////////////////////////////////////////////
// Implementations
////////////////////////////////////////////////////////////////////////////////
template <class Context>
string TensorSerializer<Context>::Serialize(
    const Blob& blob, const string& name) {
  CAFFE_CHECK(blob.IsType<Tensor<Context> >());
  auto& input = blob.Get<Tensor<Context> >();
  TensorProto proto;
  proto.set_name(name);
  if (input.template IsType<float>()) {
    proto.set_data_type(TensorProto::FLOAT);
    for (int dim : input.dims()) {
      proto.add_dims(dim);
    }
    proto.mutable_float_data()->Reserve(input.size());
    for (int i = 0; i < input.size(); ++i) {
      proto.add_float_data(0);
    }
    this->device_context_.template Copy<float, Context, CPUContext>(
        input.size(), input.template data<float>(),
        proto.mutable_float_data()->mutable_data());
  } else if (input.template IsType<int>()) {
    static_assert(sizeof(int) == 4,
        "int in this compiler does not equal to 4 bytes.");
    proto.set_data_type(TensorProto::INT32);
    for (int dim : input.dims()) {
      proto.add_dims(dim);
    }
    proto.mutable_int32_data()->Reserve(input.size());
    for (int i = 0; i < input.size(); ++i) {
      proto.add_int32_data(0);
    }
    this->device_context_.template Copy<int, Context, CPUContext>(
        input.size(), input.template data<int>(),
        proto.mutable_int32_data()->mutable_data());
  } else {
    CAFFE_LOG_FATAL << "TensorSerializer does not have a serialization "
                  "implementation for " << input.meta().name();
  }
  return proto.SerializeAsString();
}

}  // namespace caffe2

#endif  // CAFFE2_CORE_BLOB_SERIALIZATION_H_
