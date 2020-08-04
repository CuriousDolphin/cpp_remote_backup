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

//using boost::uuids::detail::md5;

std::size_t fileSize(std::ifstream& file)
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
}

/* std::string toString(const md5::digest_type &digest)
{
    const auto charDigest = reinterpret_cast<const char *>(&digest);
    std::string result;
    boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type), std::back_inserter(result));
    return result;
} */

/*std::string getMD5(const std::string &fp)
{
    hash.process_bytes(s.data(), s.size());
    hash.get_digest(digest);
}*/

