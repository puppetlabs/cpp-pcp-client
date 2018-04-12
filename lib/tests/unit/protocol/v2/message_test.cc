#include "tests/test.hpp"

#include <cpp-pcp-client/protocol/v2/message.hpp>
#include <cpp-pcp-client/protocol/v2/schemas.hpp>

#include <iostream>
#include <vector>
#include <stdint.h>
#include <chrono>

using namespace PCPClient;
using namespace v2;

TEST_CASE("v2::Message::Message - parsing valid messages", "[message]") {
    SECTION("can instantiate by passing a basic valid message") {
        REQUIRE_NOTHROW(Message(R"({"id":"f0e71a48-969c-4377-b953-35f0fc55c388","message_type":"foo"})"));
    }

    SECTION("stringify a message") {
        auto text = R"({"id":"f0e71a48-969c-4377-b953-35f0fc55c388","message_type":"foo"})";
        REQUIRE(Message(text).toString() == text);
    }
}

TEST_CASE("v2::Message::Message - failure cases", "[message]") {
    SECTION("throws a data_parse_error in case the message is invalid json") {
        REQUIRE_THROWS_AS(Message(R"({"id":"f0e71a48-969c-4377-b953-35f0fc55c388",})"), lth_jc::data_parse_error);
    }
}

TEST_CASE("v2::Message::getParsedChunks - validating parsed messages", "[message]") {
    Validator validator;
    validator.registerSchema(Protocol::EnvelopeSchema());

    SECTION("validates a message of unknown schema with no data") {
        auto text = R"({"id":"f0e71a48-969c-4377-b953-35f0fc55c388","message_type":"http://puppetlabs.com/error_message"})";
        auto json = lth_jc::JsonContainer(text);

        auto chunks = Message(json).getParsedChunks(validator);
        REQUIRE(chunks.envelope.toString() == json.toString());
        REQUIRE_FALSE(chunks.has_data);
        REQUIRE_FALSE(chunks.invalid_data);
        REQUIRE(chunks.data.empty());
        REQUIRE(chunks.debug.empty());
        REQUIRE(chunks.num_invalid_debug == 0);
    }

    SECTION("validates a message with optional properties") {
        auto text = "{"
            R"("id":"f0e71a48-969c-4377-b953-35f0fc55c388",)"
            R"("message_type":"http://puppetlabs.com/error_message",)"
            R"("sender":"pcp://foo/bar",)"
            R"("target":"pcp://foo/bar")"
            "}";
        auto json = lth_jc::JsonContainer(text);

        auto chunks = Message(json).getParsedChunks(validator);
        REQUIRE(chunks.envelope.toString() == json.toString());
        REQUIRE_FALSE(chunks.has_data);
        REQUIRE_FALSE(chunks.invalid_data);
        REQUIRE(chunks.data.empty());
        REQUIRE(chunks.debug.empty());
        REQUIRE(chunks.num_invalid_debug == 0);
    }

    SECTION("validates a message that has data") {
        auto text = "{"
            R"("id":"f0e71a48-969c-4377-b953-35f0fc55c388",)"
            R"("message_type":"http://puppetlabs.com/error_message",)"
            R"("data":"some error message")"
            "}";
        auto json = lth_jc::JsonContainer(text);

        validator.registerSchema(Protocol::ErrorMessageSchema());
        auto chunks = Message(json).getParsedChunks(validator);
        REQUIRE(chunks.envelope.toString() == json.toString());
        REQUIRE(chunks.has_data);
        REQUIRE_FALSE(chunks.invalid_data);
        REQUIRE(chunks.data.get<std::string>() == json.get<std::string>("data"));
        REQUIRE(chunks.debug.empty());
        REQUIRE(chunks.num_invalid_debug == 0);
    }

    SECTION("validates an inventory request") {
        auto text = "{"
            R"("id":"f0e71a48-969c-4377-b953-35f0fc55c388",)"
            R"("message_type":"http://puppetlabs.com/inventory_request",)"
            R"("data":{"query":["pcp://*/*"]})"
            "}";
        auto json = lth_jc::JsonContainer(text);

        validator.registerSchema(Protocol::InventoryRequestSchema());
        auto chunks = Message(json).getParsedChunks(validator);
        REQUIRE(chunks.envelope.toString() == json.toString());
        REQUIRE(chunks.has_data);
        REQUIRE_FALSE(chunks.invalid_data);
        REQUIRE(chunks.data.toString() == json.get<lth_jc::JsonContainer>("data").toString());
        REQUIRE(chunks.debug.empty());
        REQUIRE(chunks.num_invalid_debug == 0);
    }

    SECTION("validates an inventory response") {
        auto text = "{"
            R"("id":"f0e71a48-969c-4377-b953-35f0fc55c388",)"
            R"("message_type":"http://puppetlabs.com/inventory_response",)"
            R"("data":{"uris":["pcp://foo/bar"]})"
            "}";
        auto json = lth_jc::JsonContainer(text);

        validator.registerSchema(Protocol::InventoryResponseSchema());
        auto chunks = Message(json).getParsedChunks(validator);
        REQUIRE(chunks.envelope.toString() == json.toString());
        REQUIRE(chunks.has_data);
        REQUIRE_FALSE(chunks.invalid_data);
        REQUIRE(chunks.data.toString() == json.get<lth_jc::JsonContainer>("data").toString());
        REQUIRE(chunks.debug.empty());
        REQUIRE(chunks.num_invalid_debug == 0);
    }
}

TEST_CASE("v2::Message::getParsedChunks - failure cases", "[message]") {
    Validator validator;
    validator.registerSchema(Protocol::EnvelopeSchema());

    SECTION("throws a validation_error in case the schema does not match") {
        Message msg(R"({"id":"f0e71a48-969c-4377-b953-35f0fc55c388"})");
        REQUIRE_THROWS_AS(msg.getParsedChunks(validator), validation_error);
    }

    SECTION("marks invalid_data in case the data schema does not match") {
        validator.registerSchema(Protocol::ErrorMessageSchema());
        Message msg("{"
            R"("id":"f0e71a48-969c-4377-b953-35f0fc55c388",)"
            R"("message_type":"http://puppetlabs.com/error_message",)"
            R"("data":{"description":"some error message"})"
            "}");
        auto chunks = msg.getParsedChunks(validator);
        REQUIRE(chunks.has_data);
        REQUIRE(chunks.invalid_data);
    }

    SECTION("marks invalid_data in case the schema is not registered") {
        Message msg("{"
            R"("id":"f0e71a48-969c-4377-b953-35f0fc55c388",)"
            R"("message_type":"http://puppetlabs.com/error_message",)"
            R"("data":"some error message")"
            "}");
        auto chunks = msg.getParsedChunks(validator);
        REQUIRE(chunks.has_data);
        REQUIRE(chunks.invalid_data);
    }
}

//
// Performance
//

TEST_CASE("v2::Message serialization and parsing performance", "[message]") {
    Validator validator;
    validator.registerSchema(Protocol::EnvelopeSchema());
    validator.registerSchema(Protocol::ErrorMessageSchema());

    std::string text = R"({"id":"f0e71a48-969c-4377-b953-35f0fc55c388","message_type":"http://puppetlabs.com/error_message","sender":"pcp://client01.example.com/puppet-api-test","target":"pcp://client01.example.com/puppet-api-test","data":"a 1019 bytes txt:\nLorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam vitae laoreet est. Mauris eget imperdiet turpis. Duis dignissim sagittis tortor eu molestie. Donec luctus non urna in fringilla. Curabitur eu ullamcorper diam. Nulla nec tincidunt sem, quis aliquam purus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vel arcu venenatis mauris porttitor sollicitudin. Pellentesque sit amet purus a leo euismod varius. Nulla consectetur dui sit amet augue tempus, ac interdum dui tincidunt.\nVestibulum nec enim varius, sagittis nisi vel, convallis dolor. Donec ut sodales magna. Vivamus mi purus, porttitor at magna vel, ornare faucibus ipsum. Proin augue sapien, ornare nec facilisis a, pretium vel leo. Ut quam velit, hendrerit sed facilisis id, euismod eget lorem. Nam congue vitae diam vehicula bibendum. Aenean pulvinar odio ipsum, quis malesuada dui feugiat vitae. Vivamus leo ligula, luctus interdum quam quis, tempor pellentesque arcu. Nullam iaculis pulvinar odio, eget cras amet.\n"})";
    Message msg(text);

    static std::string current_test {};
    static auto num_msg = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    SECTION("serialize " + std::to_string(num_msg) + " messages of ~1 Kbyte") {
        current_test = "serialize";
        for (auto idx = 0; idx < num_msg; ++idx) {
            auto s_m = msg.toString();
            if (s_m.size() != text.size()) {
                FAIL("serialization failure");
            }
        }
    }

    SECTION("parse and validate " + std::to_string(num_msg) + " messages of ~1 Kbyte") {
        current_test = "parse";
        for (auto idx = 0; idx < num_msg; idx++) {
            Message parsed_msg { text };
            if (!parsed_msg.getParsedChunks(validator).has_data) {
                FAIL("parsing failure");
            }
        }
    }

    auto execution_time =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start)
                    .count());

    std::cout << "  time to " << current_test << " " << num_msg
              << " messages of " << text.size() << " bytes: "
              << execution_time / (1000 * 1000) << " s ("
              << static_cast<int>((num_msg / execution_time) * (1000 * 1000))
              << " msg/s)\n";
}
