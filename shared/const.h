#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <iostream>

const std::string PARAM_DELIMITER = "##";
const std::string REQUEST_DELIMITER = "\r\n";
const int LEN_BUFF = 10000;

// Define available file changes
 enum class Method {
    SNAPSHOT,
    GET,
    PUT,
    PATCH,
    DELETE,
};

#endif
