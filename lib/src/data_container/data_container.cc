#include <cthun-client/data_container/data_container.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/allocators.h>
#include <rapidjson/rapidjson.h>  // rapidjson::Type

namespace CthunClient {

DataContainer::DataContainer() : document_root_ { new rapidjson::Document() } {
    document_root_->SetObject();
}

DataContainer::DataContainer(const std::string& json_text) : DataContainer() {
    document_root_->Parse(json_text.data());

    if (document_root_->HasParseError()) {
        throw data_parse_error { "invalid json" };
    }
}

DataContainer::DataContainer(const rapidjson::Value& value) : DataContainer() {
    // Because rapidjson disallows the use of copy constructors we pass
    // the json by const reference and recreate it by explicitly copying
    document_root_->CopyFrom(value, document_root_->GetAllocator());
}

DataContainer::DataContainer(const DataContainer& data) : DataContainer(){
    document_root_->CopyFrom(*data.document_root_, document_root_->GetAllocator());
}

DataContainer::DataContainer(const DataContainer&& data) : DataContainer() {
    document_root_->CopyFrom(*data.document_root_, document_root_->GetAllocator());
}

DataContainer& DataContainer::operator=(DataContainer other) {
    std::swap(document_root_, other.document_root_);
    return *this;
}

// unique_ptr requires a complete type at time of destruction. this forces us to
// either have an empty destructor or use a shared_ptr instead.
DataContainer::~DataContainer() {}

std::vector<std::string> DataContainer::keys() const {
    std::vector<std::string> k;
    rapidjson::Value* v = reinterpret_cast<rapidjson::Value*>(document_root_.get());

    if (v->IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = v->MemberBegin();
             itr != v->MemberEnd(); ++itr) {
                k.push_back(itr->name.GetString());
        }
    }

    // Return an empty vector if the document type isn't an object
    return k;
}

rapidjson::Document DataContainer::getRaw() const {
    rapidjson::Document tmp;
    auto& a_t = document_root_->GetAllocator();
    tmp.CopyFrom(*document_root_, a_t);
    return tmp;
}

std::string DataContainer::toString() const {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document_root_->Accept(writer);
    return buffer.GetString();
}

bool DataContainer::empty() const {
    rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());
    auto data_type = getValueType(*jval);

    if (data_type == DataType::Object) {
        return jval->ObjectEmpty();
    } else if (data_type == DataType::Array) {
        return jval->Empty();
    } else {
        return false;
    }
}

bool DataContainer::includes(const DataContainerKey& key) const {
    rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());

    if (hasKey(*jval, key.data())) {
        return true;
    } else {
        return false;
    }
}

bool DataContainer::includes(std::vector<DataContainerKey> keys) const {
    rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());

    for (const auto& key : keys) {
        if (!hasKey(*jval, key.data())) {
            return false;
        }
        jval = getValueInJson(*jval, key.data());
    }

    return true;
}

DataType DataContainer::type() const {
    rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());
    return getValueType(*jval);
}

DataType DataContainer::type(const DataContainerKey& key) const {
    rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());

    if (!hasKey(*jval, key.data())) {
        throw data_key_error { "unknown key: " + key };
    }

    jval = getValueInJson(*jval, key.data());

    return getValueType(*jval);
}

DataType DataContainer::type(std::vector<DataContainerKey> keys) const {
    rapidjson::Value* jval = reinterpret_cast<rapidjson::Value*>(document_root_.get());

    for (const auto& key : keys) {
        if (!hasKey(*jval, key.data())) {
            throw data_key_error { "unknown key: " + key };
        }
        jval = getValueInJson(*jval, key.data());
    }

    return getValueType(*jval);
}

// Private functions

DataType DataContainer::getValueType(const rapidjson::Value& jval) const {
    switch (jval.GetType()) {
        case rapidjson::Type::kNullType:
            return DataType::Null;
        case rapidjson::Type::kFalseType:
            return DataType::Bool;
        case rapidjson::Type::kTrueType:
            return DataType::Bool;
        case rapidjson::Type::kObjectType:
            return DataType::Object;
        case rapidjson::Type::kArrayType:
            return DataType::Array;
        case rapidjson::Type::kStringType:
            return DataType::String;
        case rapidjson::Type::kNumberType:
            if (jval.IsDouble()) {
                return DataType::Double;
            } else {
                return DataType::Int;
            }
        default:
            // This is unexpected as for rapidjson docs
            return DataType::Null;
    }
}

bool DataContainer::hasKey(const rapidjson::Value& jval, const char* key) const {
    return (jval.IsObject() && jval.HasMember(key));
}

bool DataContainer::isObject(const rapidjson::Value& jval) const {
    return jval.IsObject();
}

rapidjson::Value* DataContainer::getValueInJson(const rapidjson::Value& jval,
                                                const char* key) const {
    return const_cast<rapidjson::Value*>(&jval[key]);
}

void DataContainer::createKeyInJson(const char* key,
                                    rapidjson::Value& jval) {
    jval.AddMember(rapidjson::Value(key, document_root_->GetAllocator()).Move(),
                   rapidjson::Value(rapidjson::kObjectType).Move(),
                   document_root_->GetAllocator());
}

// getValue specialisations

template<>
int DataContainer::getValue<>() const {
    return 0;
}

template<>
int DataContainer::getValue<>(const rapidjson::Value& value) const {
    if (value.IsNull()) {
        return 0;
    }
    return value.GetInt();
}

template<>
bool DataContainer::getValue<>() const {
    return false;
}

template<>
bool DataContainer::getValue<>(const rapidjson::Value& value) const {
    if (value.IsNull()) {
        return false;
    }
    return value.GetBool();
}

template<>
std::string DataContainer::getValue() const {
    return "";
}

template<>
std::string DataContainer::getValue<>(const rapidjson::Value& value) const {
    if (value.IsNull()) {
        return "";
    }
    return std::string(value.GetString());
}

template<>
double DataContainer::getValue<>() const {
    return 0.0;
}

template<>
double DataContainer::getValue<>(const rapidjson::Value& value) const {
    if (value.IsNull()) {
        return 0.0;
    }
    return value.GetDouble();
}

template<>
DataContainer DataContainer::getValue<>() const {
    DataContainer container {};
    return container;
}

template<>
DataContainer DataContainer::getValue<>(const rapidjson::Value& value) const {
    if (value.IsNull()) {
        DataContainer container {};
        return container;
    }
    // rvalue return
    DataContainer containter { value };
    return containter;
}

template<>
rapidjson::Value DataContainer::getValue<>(const rapidjson::Value& value) const {
    DataContainer* tmp_this = const_cast<DataContainer*>(this);
    rapidjson::Value v { value, tmp_this->document_root_->GetAllocator() };
    return v;
}

template<>
std::vector<std::string> DataContainer::getValue<>() const {
    std::vector<std::string> tmp {};
    return tmp;
}

template<>
std::vector<std::string> DataContainer::getValue<>(const rapidjson::Value& value) const {
    std::vector<std::string> tmp {};

    if (value.IsNull()) {
        return tmp;
    }

    for (rapidjson::Value::ConstValueIterator itr = value.Begin();
         itr != value.End();
         itr++) {
        tmp.push_back(itr->GetString());
    }

    return tmp;
}

template<>
std::vector<bool> DataContainer::getValue<>() const {
    std::vector<bool> tmp {};
    return tmp;
}

template<>
std::vector<bool> DataContainer::getValue<>(const rapidjson::Value& value) const {
    std::vector<bool> tmp {};

    if (value.IsNull()) {
        return tmp;
    }

    for (rapidjson::Value::ConstValueIterator itr = value.Begin();
         itr != value.End();
         itr++) {
        tmp.push_back(itr->GetBool());
    }

    return tmp;
}

template<>
std::vector<int> DataContainer::getValue<>() const {
    std::vector<int> tmp {};
    return tmp;
}

template<>
std::vector<int> DataContainer::getValue<>(const rapidjson::Value& value) const {
    std::vector<int> tmp {};

    if (value.IsNull()) {
        return tmp;
    }

    for (rapidjson::Value::ConstValueIterator itr = value.Begin();
         itr != value.End();
         itr++) {
        tmp.push_back(itr->GetInt());
    }

    return tmp;
}

template<>
std::vector<double> DataContainer::getValue<>() const {
    std::vector<double> tmp {};
    return tmp;
}

template<>
std::vector<double> DataContainer::getValue<>(const rapidjson::Value& value) const {
    std::vector<double> tmp {};

    if (value.IsNull()) {
        return tmp;
    }

    for (rapidjson::Value::ConstValueIterator itr = value.Begin();
         itr != value.End();
         itr++) {
        tmp.push_back(itr->GetDouble());
    }

    return tmp;
}

template<>
std::vector<DataContainer> DataContainer::getValue<>() const {
    std::vector<DataContainer> tmp {};
    return tmp;
}

template<>
std::vector<DataContainer> DataContainer::getValue<>(const rapidjson::Value& value) const {
    std::vector<DataContainer> tmp {};

    if (value.IsNull()) {
        return tmp;
    }

    for (rapidjson::Value::ConstValueIterator itr = value.Begin();
         itr != value.End();
         itr++) {
        DataContainer* tmp_this = const_cast<DataContainer*>(this);
        const rapidjson::Value tmpvalue(*itr, tmp_this->document_root_->GetAllocator());
        DataContainer tmp_data { tmpvalue };
        tmp.push_back(tmp_data);
    }

    return tmp;
}

// setValue specialisations

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, bool new_value) {
    jval.SetBool(new_value);
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, int new_value) {
    jval.SetInt(new_value);
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, const std::string new_value) {
    jval.SetString(new_value.data(), new_value.size(), document_root_->GetAllocator());
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, const char * new_value) {
    jval.SetString(new_value, std::string(new_value).size(), document_root_->GetAllocator());
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, double new_value) {
    jval.SetDouble(new_value);
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<std::string> new_value ) {
    jval.SetArray();

    for (const auto& value : new_value) {
        // rapidjson doesn't like std::string...
        rapidjson::Value s;
        s.SetString(value.data(), value.size(), document_root_->GetAllocator());
        jval.PushBack(s, document_root_->GetAllocator());
    }
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<bool> new_value ) {
    jval.SetArray();

    for (const auto& value : new_value) {
        rapidjson::Value tmp_val;
        tmp_val.SetBool(value);
        jval.PushBack(tmp_val, document_root_->GetAllocator());
    }
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<int> new_value ) {
    jval.SetArray();

    for (const auto& value : new_value) {
        rapidjson::Value tmp_val;
        tmp_val.SetInt(value);
        jval.PushBack(tmp_val, document_root_->GetAllocator());
    }
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<double> new_value ) {
    jval.SetArray();

    for (const auto& value : new_value) {
        rapidjson::Value tmp_val;
        tmp_val.SetDouble(value);
        jval.PushBack(tmp_val, document_root_->GetAllocator());
    }
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<DataContainer> new_value ) {
    jval.SetArray();

    for (auto value : new_value) {
        rapidjson::Document tmp_value;
        tmp_value.CopyFrom(*value.document_root_, document_root_->GetAllocator());
        jval.PushBack(tmp_value, document_root_->GetAllocator());
    }
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, DataContainer new_value ) {
    jval.CopyFrom(new_value.getRaw(), document_root_->GetAllocator());
}

}  // namespace CthunClient
