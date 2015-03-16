#include "test/test.h"

#include <cthun-client/data_container/data_container.h>

#include <iostream>

static const std::string JSON = "{\"foo\" : {\"bar\" : 2},"
                                " \"goo\" : 1,"
                                " \"bool\" : true,"
                                " \"string\" : \"a string\","
                                " \"null\" : null,"
                                " \"real\" : 3.1415,"
                                " \"vec\" : [1, 2], "
                                " \"nested\" : {"
                                "                  \"foo\" : \"bar\""
                                "               }"
                                "}";

namespace CthunClient {

TEST_CASE("DataContainer::get", "[data]") {
    DataContainer msg { JSON };

    SECTION("it can get a root value") {
        REQUIRE(msg.get<int>("goo") == 1);
    }

    SECTION("it can get a nested value") {
        REQUIRE(msg.get<int>("foo", "bar") == 2);
    }

    SECTION("it can get a bool value") {
        REQUIRE(msg.get<bool>("bool") == true);
    }

    SECTION("it can get a string value") {
        REQUIRE(msg.get<std::string>("string") == "a string");
    }

    SECTION("it can get a double value") {
        REQUIRE(msg.get<double>("real") == 3.1415);
    }

    SECTION("it can get a vector") {
        std::vector<int> tmp { 1, 2 };
        std::vector<int> result { msg.get<std::vector<int>>("vec") };
        REQUIRE(tmp[0] == result[0]);
        REQUIRE(tmp[1] == result[1]);
    }

    SECTION("it should behave correctly given a null value") {
        REQUIRE(msg.get<std::string>("null") == "");
        REQUIRE(msg.get<int>("null") == 0);
        REQUIRE(msg.get<bool>("null") == false);
    }

    SECTION("it returns a null like value when indexing something that "
            "doesn't exist") {
        REQUIRE(msg.get<std::string>("invalid") == "");
        REQUIRE(msg.get<int>("goo", "1") == 0);
        REQUIRE(msg.get<bool>("foo", "baz") == false);
    }
}

TEST_CASE("DataContainer::includes", "[data]") {
    SECTION("Document/object lookups") {
        DataContainer msg { JSON };
        REQUIRE(msg.includes("foo") == true);
        REQUIRE(msg.includes("foo", "bar") == true);
        REQUIRE(msg.includes("foo", "baz") == false);
    }

    SECTION("Non object/document lookups") {
        DataContainer msg { "\"foo\"" };
        REQUIRE(msg.includes("bar", "bar") == false);
        REQUIRE(msg.includes("foo") == false);
    }
}

TEST_CASE("DataContainer::set", "[data]") {
    DataContainer msg {};

    SECTION("it should add a new pair to the root") {
        msg.set<int>("foo", 4);
        REQUIRE(msg.get<int>("foo") == 4);
    }

    SECTION("it allows the creation of a nested structure") {
        msg.set<int>("level1", "level21", 0);
        msg.set<bool>("bool1", true);
        msg.set<std::string>("level1", "level22", "a string");
        msg.set<std::string>("level11", "different string");
        REQUIRE(msg.get<int>("level1", "level21") == 0);
        REQUIRE(msg.get<bool>("bool1") == true);
        REQUIRE(msg.get<std::string>("level1", "level22") == "a string");
        REQUIRE(msg.get<std::string>("level11") == "different string");
    }

    SECTION("it allows resetting an integer value") {
        msg.set<int>("i entry", 0);
        REQUIRE(msg.includes("i entry"));
        REQUIRE(msg.get<int>("i entry") == 0);

        msg.set<int>("i entry", 5);
        REQUIRE(msg.get<int>("i entry") == 5);
    }

    SECTION("it allows resetting a double value") {
        msg.set<double>("d entry", 3.14159);
        REQUIRE(msg.includes("d entry"));
        REQUIRE(msg.get<double>("d entry") == 3.14159);

        msg.set<double>("d entry", 2.71828);
        REQUIRE(msg.get<double>("d entry") == 2.71828);
    }

    SECTION("it allows resetting a boolean value") {
        msg.set<bool>("b entry", true);
        REQUIRE(msg.includes("b entry"));
        REQUIRE(msg.get<bool>("b entry") == true);

        msg.set<bool>("b entry", false);
        REQUIRE(msg.get<bool>("b entry") == false);
    }

    SECTION("it allows resetting a string value") {
        msg.set<std::string>("s entry", "bar");
        REQUIRE(msg.includes("s entry"));
        REQUIRE(msg.get<std::string>("s entry") == "bar");

        msg.set<std::string>("s entry", "spam");
        REQUIRE(msg.get<std::string>("s entry") == "spam");
    }

    SECTION("it allows resetting a string vector value") {
        std::vector<std::string> s_v { "foo", "bar" };
        msg.set<std::vector<std::string>>("s_v entry", s_v);
        REQUIRE(msg.includes("s_v entry"));
        REQUIRE(msg.get<std::vector<std::string>>("s_v entry") == s_v);

        std::vector<std::string> s_v_other { "spam", "eggs" };
        msg.set<std::vector<std::string>>("s_v entry", s_v_other);
        REQUIRE(msg.get<std::vector<std::string>>("s_v entry") == s_v_other);
    }

    SECTION("it allows resetting a DataContainer value") {
        DataContainer d {};
        d.set<int>("i", 1);
        msg.set<DataContainer>("d_c entry", d);
        auto i_entry = msg.get<DataContainer>("d_c entry");
        REQUIRE(msg.includes("d_c entry"));
        REQUIRE(i_entry.get<int>("i") == 1);

        DataContainer d_other {};
        d_other.set<bool>("b", false);
        auto b_entry = msg.get<DataContainer>("d_c entry");
        REQUIRE_FALSE(b_entry.get<bool>("b"));
    }

    SECTION("it can set a key to a vector") {
        std::vector<std::string> strings { "foo", "bar" };
        msg.set<std::vector<std::string>>("sv", strings);

        std::vector<int> ints { 4, 2 };
        msg.set<std::vector<int>>("iv", ints);

        std::vector<bool> bools { true, false };
        msg.set<std::vector<bool>>("bv", bools);

        std::vector<double> doubles { 0.00, 9.99 };
        msg.set<std::vector<double>>("dv", doubles);

        REQUIRE(msg.get<std::vector<std::string>>("sv")[0] == "foo");
        REQUIRE(msg.get<std::vector<std::string>>("sv")[1] == "bar");

        REQUIRE(msg.get<std::vector<int>>("iv")[0] == 4);
        REQUIRE(msg.get<std::vector<int>>("iv")[1] == 2);

        REQUIRE(msg.get<std::vector<bool>>("bv")[0] == true);
        REQUIRE(msg.get<std::vector<bool>>("bv")[1] == false);

        REQUIRE(msg.get<std::vector<double>>("dv")[0] == 0.00);
        REQUIRE(msg.get<std::vector<double>>("dv")[1] == 9.99);
    }
}

TEST_CASE("DataContainer::keys", "[data]") {
    SECTION("It returns a vector of keys") {
        DataContainer data { "{ \"a\" : 1, "
                              " \"b\" : 2}"};
        REQUIRE(data.keys().size() == 2);
    }

    SECTION("It returns an empty vector when the DataContainer is empty") {
        DataContainer data {};
        REQUIRE(data.keys().size() == 0);
    }
}

}  // namespace CthunClient
