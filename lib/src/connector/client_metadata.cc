#include <cpp-pcp-client/connector/client_metadata.hpp>
#include <cpp-pcp-client/connector/errors.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".client_metadata"

#include <leatherman/logging/logging.hpp>

#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#include <stdio.h>  // std::fopen

namespace CthunClient {

// Get rid of OSX 10.7 and greater deprecation warnings.
#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

static const std::string CTHUN_URI_SCHEME { "cth://" };

std::string getCommonNameFromCert(const std::string& client_crt_path) {
    LOG_INFO("Retrieving the common name from certificate %1%", client_crt_path);

    std::unique_ptr<std::FILE, int(*)(std::FILE*)> fp {
        std::fopen(client_crt_path.data(), "r"), std::fclose };

    if (fp == nullptr) {
        throw connection_config_error { "certificate file '" + client_crt_path
                                        + "' does not exist" };
    }

    std::unique_ptr<X509, void(*)(X509*)> cert {
        PEM_read_X509(fp.get(), NULL, NULL, NULL), X509_free };

    if (cert == nullptr) {
        throw connection_config_error { "certificate file '" + client_crt_path
                                        + "' is invalid" };
    }

    X509_NAME* subj = X509_get_subject_name(cert.get());
    X509_NAME_ENTRY* name_entry = X509_NAME_get_entry(subj, 0);

    if (name_entry == nullptr) {
        throw connection_config_error { "failed to retrieve the client common "
                                        "name from " + client_crt_path };
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
                               const std::string& _key)
        : ca { _ca },
          crt { _crt },
          key { _key },
          client_type { _client_type },
          common_name { getCommonNameFromCert(crt) },
          uri { CTHUN_URI_SCHEME + common_name + "/" + client_type } {
    LOG_INFO("Retrieved common name from the certificate and determined "
             "the client URI: %1%", uri);
}

}  // namespace CthunClient
