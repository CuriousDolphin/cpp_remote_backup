#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <iostream>
#include <map>

const std::string PARAM_DELIMITER = "##";
const std::string REQUEST_DELIMITER = "\r\n";
const int LEN_BUFF = 10000;

// Define available file changes
enum class Method
{
    SNAPSHOT,
    GET,
    PUT,
    PATCH,
    DELETE,
};
// Define available file changes
enum class FileStatus
{
    created,   // file created on local fs
    modified,  // file modified on local fs
    erased,    // file deleted from local fs
    missing,   // remote file not exist on local fs
    untracked, // local file not exist on remote fs
};
enum class Server_error
{
    WRONG_CREDENTIALS,
    UNKNOWN_COMMAND,
    WRONG_N_ARGS,
    FILE_EXIST,
    UNKNOWN_ERROR,
    FILE_CREATE_ERROR
};

const std::map<Server_error, int> ERROR_COD = {{Server_error::WRONG_CREDENTIALS, 1}, // "LOGIN USER PWD"
                                               {Server_error::UNKNOWN_COMMAND, 2},   //  "GET FILPATH"
                                               {Server_error::WRONG_N_ARGS, 3},
                                               {Server_error::FILE_EXIST, 4},
                                               {Server_error::UNKNOWN_ERROR, 5},
                                               {Server_error::FILE_CREATE_ERROR, 5}

};

#endif
