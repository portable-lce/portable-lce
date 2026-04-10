// #if defined(_WIN32)
// #include <xhash>
// #else
#include <iomanip>
#include <sstream>
// #endif  // _WIN32
// #include <openssl/evp.h>
// #include <openssl/types.h>

#include "Hasher.h"

Hasher::Hasher(std::string& salt) { this->salt = salt; }

std::string Hasher::getHash(std::string& name) {
    // 4jcraft: stubbed for portability reasons.
    // 
    // The OpenSSL meson is broken on windows and expects winsock to
    // be statically linked (which is dumb since we only want libcrypto
    // for MD5 hashing), and avoiding a dependency on libcrypto here is
    // probably a good thing either way for some platforms.
    //
    // Given that this class isn't used and im lazy, we're just gonna do
    // this until it becomes a problem. This class isnt actually used
    // anywhere at the moment anyways so it's whatever.
    return "";
    
#if 0
#if defined(_WIN32)
    // 4J Stu - Removed try/catch
    // try {
    std::string s = std::string(salt).append(name);
    // MessageDigest m;
    // m = MessageDigest.getInstance("MD5");
    // m.update(s.getBytes(), 0, s.length());
    // return new BigInteger(1, m.digest()).toString(16);

    // TODO 4J Stu - Will this hash us with the same distribution as the MD5?
    return toString(hash_value(s));
    //}
    // catch (NoSuchAlgorithmException e)
    //{
    //	throw new RuntimeException(e);
    //}
#else
    // adapted from a SSL example
    std::string combined = salt + name;
    std::string combined_str(combined.begin(), combined.end());
    unsigned char result[EVP_MAX_MD_SIZE];
    EVP_MD_CTX* md5_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(md5_ctx, EVP_md5(), nullptr);
    EVP_DigestUpdate(md5_ctx, combined_str.c_str(), combined_str.size());
    unsigned int result_len;
    EVP_DigestFinal_ex(md5_ctx, result, &result_len);
    EVP_MD_CTX_free(md5_ctx);
    std::stringstream ss;
    for (unsigned int i = 0; i < result_len; i++) {
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)result[i];
    }
    std::string hash_str = ss.str();
    return std::string(hash_str.begin(), hash_str.end());
#endif
#endif
}
