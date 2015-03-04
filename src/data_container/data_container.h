#ifndef CTHUN_CLIENT_SRC_DATA_CONTAINER_DATA_CONTAINER_H_
#define CTHUN_CLIENT_SRC_DATA_CONTAINER_DATA_CONTAINER_H_

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/allocators.h>

#include <vector>
#include <cstdarg>
#include <iostream>
#include <tuple>
#include <typeinfo>

namespace CthunClient {

// Data abstraction overlaying the raw JSON objects

// Errors

/// Error thrown when a message string is invalid.
class parse_error : public std::runtime_error  {
  public:
    explicit parse_error(std::string const& msg) : std::runtime_error(msg) {}
};

/// Error thrown when a nested message index is invalid.
class index_error : public std::runtime_error  {
  public:
    explicit index_error(std::string const& msg) : std::runtime_error(msg) {}
};

/// Error thrown when trying to set or get invalid type.
class data_type_error : public std::runtime_error  {
  public:
    explicit data_type_error(std::string const& msg) : std::runtime_error(msg) {}
};

// Usage:
// NOTE SUPPORTED SCALARS
// int, float, double, bool, std::string, nullptr
//
// To set a key to a scalar value in object x
//    x.set<int>("foo", 1);
//    x.set<std::string>(foo", "bar");
//
// To set a nested key to a scalar value in object x
//    x.set<bool>("foo", "bar", "baz", true);
//
// NOTE THAT LOCATION IN STUCTURE IS A VARAIDIC ARGUMENT
//
// To set a key to a vector value in object x
//    std::vector<int> tmp { 1, 2, 3 };
//    x.set<std::vector<int>>("foo", tmp);
//
// To get a scalar value from a key in object x
//    x.get<std::string>("foo");
//    x.get<int>("bar");
//
// To get a vector from a key in object x
//    x.get<std::vector<float>>("foo");
//
// To get a result object (json object) from object x
//    x.get<Message>("foo");
//    x.get<Data>("foo");
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
// To check if a key is set in object x
//    x.includes("foo");
//    x.includes("foo", "bar", "baz");

class DataContainer {
  public:
    DataContainer();
    explicit DataContainer(std::string json_txt);
    explicit DataContainer(const rapidjson::Value& value);
    DataContainer(const DataContainer& data);
    DataContainer(const DataContainer&& data);
    DataContainer& operator=(DataContainer other);

    // TODO(ale): add an empty() method

    std::string toString() const;

    bool includes(std::string first) const {
        return includes_(document_root_, first.data());
    }

    template <typename ... Args>
    bool includes(std::string first, Args... rest) const {
        return includes_(document_root_, first.data(), rest...);
    }

    rapidjson::Document getRaw() {
        rapidjson::Document tmp;
        rapidjson::Document::AllocatorType& a = document_root_.GetAllocator();
        tmp.CopyFrom(document_root_, a);
        return std::move(document_root_);
    }

    template <typename T>
    T get(std::string first) const {
        const rapidjson::Value& v = document_root_;
        return get_<T>(v, first.data());
    }

    template <typename T, typename ... Args>
    T get(std::string first, Args ... rest) const {
        const rapidjson::Value& v = document_root_;
        return get_<T>(v, first.data(), rest...);
    }

    template <typename T>
    void set(std::string first, T new_value) {
        set_<T>(document_root_, first.data(), new_value);
    }

    template <typename T, typename ... Args>
    void set(std::string first, std::string second, Args ... rest) {
        set_<T>(document_root_, first.data(), second.data(), rest...);
    }

  private:
    rapidjson::Document document_root_;

    template <typename ... Args>
    bool includes_(const rapidjson::Value& jval, const char * first) const {
        if (jval.IsObject() && jval.HasMember(first)) {
            return true;
        } else {
            return false;
        }
    }

    template <typename ... Args>
    bool includes_(const rapidjson::Value& jval, const char * first, Args ... rest) const {
        if (jval.IsObject() && jval.HasMember(first)) {
            return includes_(jval[first], rest...);
        } else {
            return false;
        }
    }

    template <typename T>
    T get_(const rapidjson::Value& jval, const char * first) const {
        if (jval.IsObject() && jval.HasMember(first)) {
            return getValue<T>(jval[first]);
        }

        return getValue<T>(rapidjson::Value(rapidjson::kNullType));
    }

    template <typename T, typename ... Args>
    T get_(const rapidjson::Value& jval, const char * first, Args ... rest) const {
        if (jval.HasMember(first) && jval[first].IsObject()) {
            return get_<T>(jval[first], rest...);
        }

        return getValue<T>(rapidjson::Value(rapidjson::kNullType));
    }

    // Last recursive call from the variadic template ends here
    template<typename T>
    void set_(rapidjson::Value& jval) {
        return;
    }

    template <typename T>
    void set_(rapidjson::Value& jval, const char* first, T new_val) {
        if (!jval.HasMember(first)) {
            jval.AddMember(rapidjson::Value(first, document_root_.GetAllocator()).Move(),
                           rapidjson::Value(rapidjson::kObjectType).Move(),
                           document_root_.GetAllocator());
        }
        setValue<T>(jval[first], new_val);
    }

    // deal with the case where parameter pack is a tuple of only strings
    // we need this because varaidic templates are greedy and won't call the
    // correct set_
    template <typename T>
    void set_string_(rapidjson::Value& jval, const char* new_val) {
        setValue<T>(jval, new_val);
    }

    template <typename T, typename ... Args>
    void set_(rapidjson::Value& jval, const char* first, Args ... rest) {
        if (sizeof...(rest) > 0) {
            if (jval.HasMember(first) && !jval[first].IsObject()) {
                throw index_error { "invalid message index supplied" };
            }

            if (!jval.HasMember(first)) {
                jval.AddMember(rapidjson::Value(first, document_root_.GetAllocator()).Move(),
                               rapidjson::Value(rapidjson::kObjectType).Move(),
                               document_root_.GetAllocator());
            }

            set_<T>(jval[first], rest...);
        } else {
            set_string_<std::string>(jval, first);
            return;
        }
    }

    template<typename T>
    T getValue(const rapidjson::Value& value) const;


    template<typename T>
    void setValue(rapidjson::Value& jval, T new_value);
};


template<typename T>
T DataContainer::getValue(const rapidjson::Value& Value) const {
    throw data_type_error { "invalid type for DataContainer" };
}

template<typename T>
void DataContainer::setValue(rapidjson::Value& jval, T new_value) {
    throw data_type_error { "invalid type for DataContainer" };
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::string new_value);

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
