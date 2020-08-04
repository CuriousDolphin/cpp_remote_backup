#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/crc.hpp>
#include <openssl/md5.h>
//using boost::uuids::detail::md5;

/* std::size_t fileSize(std::ifstream &file)
{
    std::streampos current = file.tellg();
    file.seekg(0, std::ios::end);
    std::size_t result = file.tellg();
    file.seekg(current, std::ios::beg);
    return result;
}

std::uint32_t crc32(const std::string &fp)
{
    std::ifstream is{fp, std::ios::in | std::ios::binary};
    std::size_t file_size = fileSize(is);

    std::vector<char> buf(file_size);
    is.read(&buf[0], file_size);

    boost::crc_32_type result;
    result.process_bytes(&buf[0], file_size);

    return result.checksum();
} */

// Print the MD5 sum as hex-digits.

std::string md5ToString(unsigned char *md)
{
    std::ostringstream buffer;
    for (unsigned i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        buffer << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md[i]);
    }
    return buffer.str();
}

std::string getMD5(const std::string &fp)
{
    MD5_CTX ctx;
    MD5_Init(&ctx);
    std::ifstream ifs(fp, std::ios::binary);

    char file_buffer[4096];
    while (ifs.read(file_buffer, sizeof(file_buffer)) || ifs.gcount())
    {
        MD5_Update(&ctx, file_buffer, ifs.gcount());
    }

    unsigned char digest[MD5_DIGEST_LENGTH] = {};
    MD5_Final(digest, &ctx);

    return md5ToString(digest);
}
