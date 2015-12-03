#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>

#include <leatherman/util/scope_exit.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".client_metadata"

#include <leatherman/logging/logging.hpp>

#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#include <stdio.h>  // std::fopen

namespace PCPClient {

// Get rid of OSX 10.7 and greater deprecation warnings.
#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

static const std::string PCP_URI_SCHEME { "pcp://" };

// TODO(ale): consider moving the SSL functions elsewhere

int pwdCallback(char *buf, int size, int rwflag, void *password) {
    throw connection_config_error { "key is protected by password" };
    return EXIT_FAILURE;
}

void validatePrivateKeyCertPair(const std::string& key, const std::string& crt) {
    LOG_TRACE("About to validate private key / certificate pair: '%1%' / '%2%'",
              key, crt);
    auto ctx = SSL_CTX_new(SSLv23_method());
    leatherman::util::scope_exit ctx_cleaner {
        [ctx]() { SSL_CTX_free(ctx); }
    };
    if (ctx == nullptr) {
        throw connection_config_error { "failed to create SSL context" };
    }
    SSL_CTX_set_default_passwd_cb(ctx, &pwdCallback);
    LOG_TRACE("Created SSL context");
    if (SSL_CTX_use_certificate_file(ctx, crt.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw connection_config_error { "failed to open cert" };
    }
    LOG_TRACE("Certificate loaded");
    if (SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw connection_config_error { "failed to load private key" };
    }
    LOG_TRACE("Private key loaded");
    if (!SSL_CTX_check_private_key(ctx)) {
        throw connection_config_error { "mismatch between private key and cert " };
    }
    LOG_TRACE("Private key / certificate pair has been successfully validated");
}

std::string getCommonNameFromCert(const std::string& crt) {
    LOG_TRACE("Retrieving client name from certificate '%1%'", crt);
    std::unique_ptr<std::FILE, int(*)(std::FILE*)> fp {
        std::fopen(crt.data(), "r"), std::fclose };

    if (fp == nullptr) {
        throw connection_config_error { "certificate file '" + crt
                                        + "' does not exist" };
    }

    std::unique_ptr<X509, void(*)(X509*)> cert {
        PEM_read_X509(fp.get(), NULL, NULL, NULL), X509_free };

    if (cert == nullptr) {
        throw connection_config_error { "certificate file '" + crt
                                        + "' is invalid" };
    }

    auto subj = X509_get_subject_name(cert.get());
    auto name_entry = X509_NAME_get_entry(subj, 0);

    if (name_entry == nullptr) {
        throw connection_config_error { "failed to retrieve the client common "
                                        "name from " + crt };
    }

    ASN1_STRING* asn1_name = X509_NAME_ENTRY_get_data(name_entry);
    unsigned char* name_ptr = ASN1_STRING_data(asn1_name);
    int name_size = ASN1_STRING_length(asn1_name);

    return std::string { name_ptr, name_ptr + name_size };
}

#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic pop
#endif

ClientMetadata::ClientMetadata(const std::string& _client_type,
                               const std::string& _ca,
                               const std::string& _crt,
                               const std::string& _key,
                               const long& _connection_timeout)
        : ca { _ca },
          crt { _crt },
          key { _key },
          client_type { _client_type },
          common_name { getCommonNameFromCert(crt) },
          uri { PCP_URI_SCHEME + common_name + "/" + client_type },
          connection_timeout { _connection_timeout } {
    LOG_INFO("Retrieved common name from the certificate and determined "
             "the client URI: %1%", uri);
    validatePrivateKeyCertPair(key, crt);
    LOG_DEBUG("Validated the private key / certificate pair");
}

}  // namespace PCPClient
