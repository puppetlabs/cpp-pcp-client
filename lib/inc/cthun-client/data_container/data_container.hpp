#ifndef CTHUN_CLIENT_SRC_DATA_CONTAINER_DATA_CONTAINER_H_
#define CTHUN_CLIENT_SRC_DATA_CONTAINER_DATA_CONTAINER_H_

#include <vector>
#include <cstdarg>
#include <iostream>
#include <tuple>
#include <typeinfo>
#include <memory>

// TODO(ale): build this on Linux

// TODO(ale): do we need forward declarations for rapidjson::Type?

// Forward declarations for rapidjson
namespace rapidjson {
    class CrtAllocator;
    template <typename BaseAllocator> class MemoryPoolAllocator;
    template <typename Encoding, typename Allocator> class GenericValue;
    template<typename CharType> struct UTF8;
    using Value = GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator>>;
    using Allocator = MemoryPoolAllocator<CrtAllocator>;
    template <typename Encoding,
              typename Allocator,
              typename StackAllocator> class GenericDocument;
    using Document = GenericDocument<UTF8<char>,
                                     MemoryPoolAllocator<CrtAllocator>,
                                     CrtAllocator>;
 }  // namespace rapidjson

namespace CthunClient {

// Errors

/// Error thrown when trying to parse an invalid JSON string.
class data_parse_error : public std::runtime_error  {
  public:
    explicit data_parse_error(std::string const& msg) : std::runtime_error(msg) {}
};

/// Error due to a key operation failure.
class data_key_error : public std::runtime_error  {
  public:
    explicit data_key_error(std::string const& msg) : std::runtime_error(msg) {}
};

// Types

enum DataType { Object, Array, String, Int, Bool, Double, Null };

// TODO(ale): check: we don't support float nor nullptr scalars...
// Should we?

// TODO(ale): check: replacing 'index' with 'key'; having both terms
// is confusing - the users specify keys; they don't care about
// internal trees

//
// DataContainer - data abstraction overlaying the raw JSON objects
//
// Usage:
//
// NOTE SUPPORTED SCALARS
// int, double, bool, std::string
//
// == set
//
// To set a key to a scalar value in object x
//    x.set<int>("foo", 1);
//    x.set<std::string>(foo", "bar");
//
// To set a nested key to a scalar value in object x
//    x.set<bool>({ "foo", "bar", "baz" }, true);
//
// To set a key to a vector value in object x
//    std::vector<int> tmp { 1, 2, 3 };
//    x.set<std::vector<int>>("foo", tmp);
//
// == get
//
// To get a scalar value from a key in object x
//    x.get<std::string>("foo");
//    x.get<int>("bar");
//
// To get a vector from a key in object x
//    x.get<std::vector<float>>("foo");
//
// To get a result object (json object) from object x
//    x.get<DataContainer>("foo");
//
// To get a null value from a key in object x
//    x.get<std::string>("foo") == "";
//    x.get<int>("foo") == 0;
//    x.get<bool>("foo") == false;
//    x.get<float>("foo") == 0.0f;
//    x.get<double>("foo") == 0.0;
//
// To get a json string representation of object x
//    x.toString();
//
// == includes
//
// To check if a key is set in object x
//    x.includes("foo");
//    x.includes({ "foo", "bar", "baz" });
//
// == keys
//
//  TODO
//
// == type
//
//  TODO
//
// == empty
//
//  TODO
//

struct DataContainerKey : public std::string {
    DataContainerKey(const std::string& value) : std::string(value) {}
    DataContainerKey(const char* value) : std::string(value) {}
    DataContainerKey(std::initializer_list<char> il) = delete;
};

class DataContainer {
  public:
    DataContainer();
    /// Throw a data_parse_error in case of invalid JSON text.
    explicit DataContainer(const std::string& json_txt);
    explicit DataContainer(const rapidjson::Value& value);
    DataContainer(const DataContainer& data);
    DataContainer(const DataContainer&& data);
    DataContainer& operator=(DataContainer other);

    ~DataContainer();

    std::vector<std::string> keys();

    rapidjson::Document getRaw() const;

    std::string toString() const;

    // TODO(ale): see if it's possible to use const for all
    // 'std::vector<DataContainerKey> keys' parameters

    bool includes(const DataContainerKey& key) const;
    bool includes(std::vector<DataContainerKey> keys) const;

    /// Throw a data_key_error in case the specified key is unknown.
    DataType type(const DataContainerKey& key) const;

    /// Throw a data_key_error if any one of the specified keys is
    /// unknown.
    DataType type(std::vector<DataContainerKey> keys) const;

    template <typename T>
    T get() const {
        return getValue<T>(*reinterpret_cast<rapidjson::Value*>(document_root_.get()));
    }

    template <typename T>
    T get(const DataContainerKey& key) const {
        rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());

        if (hasKey(*jval, key.data())) {
            return getValue<T>(*getValueInJson(*jval, key.data()));
        }

        return getValue<T>();
    }

    template <typename T>
    T get(std::vector<DataContainerKey> keys) const {
        rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());

        for (const auto& key : keys) {
            if (!hasKey(*jval, key.data())) {
                return getValue<T>();
            }

            jval = getValueInJson(*jval, key.data());
        }

        return getValue<T>(*jval);
    }

    /// Throw a data_key_error in case the root is not a valid JSON
    /// object, so that is not possible to set the entry.
    template <typename T>
    void set(const DataContainerKey& key, T value) {
        rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());
        const char* key_data = key.data();

        if (!isObject(*jval)) {
            // TODO(ale): check this error message; we can have any JSON value
            // in the root; in case it's not an object, we can't set a key/value
            // pair, but the problem is not the provided key... Also, replace
            // index with key (see above todo) and chek the method docstring

            throw data_key_error { "root is not a valid JSON object" };
        }

        if (!hasKey(*jval, key_data)) {
            createKeyInJson(key_data, *jval);
        }

        setValue<T>(*getValueInJson(*jval, key_data), value);
    }

    /// Throw a data_key_error if any nested key is not associated
    /// with a valid JSON object, so that it is not  possible to
    /// iterate the remaining keys.
    template <typename T>
    void set(std::vector<DataContainerKey> keys, T value) {
        rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());

        for (const auto& key : keys) {
            const char* key_data = key.data();
            if (!isObject(*jval)) {
                throw data_key_error { "invalid key supplied; cannot "
                                       "navigate the provided path" };
            }
            if (!hasKey(*jval, key_data)) {
                createKeyInJson(key_data, *jval);
            }
            jval = getValueInJson(*jval, key_data);
        }

        setValue<T>(*jval, value);
    }

  private:
    std::unique_ptr<rapidjson::Document> document_root_;

    DataType getValueType(const rapidjson::Value& jval) const;
    bool hasKey(const rapidjson::Value& jval, const char* key) const;
    bool isObject(const rapidjson::Value& jval) const;
    rapidjson::Value* getValueInJson(const rapidjson::Value& jval,
                                     const char* key) const;
    void createKeyInJson(const char* key, rapidjson::Value& jval);

    template <typename T>
    T get_(const rapidjson::Value& jval, const char * first_key) const {
        if (hasKey(jval, first_key)) {
            return getValue<T>(*getValueInJson(jval, first_key));
        }

        return getValue<T>();
    }

    template <typename T, typename ... Args>
    T get_(const rapidjson::Value& jval,
           const char * first_key,
           Args ... nested_keys_and_val) const {
        if (hasKey(jval, first_key)) {
            return get_<T>(*getValueInJson(jval, first_key),
                           nested_keys_and_val...);
        }

        return getValue<T>();
    }

    template<typename T>
    T getValue(const rapidjson::Value& value) const;

    template<typename T>
    T getValue() const;

    template<typename T>
    void setValue(rapidjson::Value& jval, T new_value);
};

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, const std::string& new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, const char* new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, bool new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, int new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, double new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<std::string> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<bool> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<int> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<double> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<DataContainer> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, DataContainer new_value);


}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_DATA_CONTAINER_DATA_CONTAINER_H_
