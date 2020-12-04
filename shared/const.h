#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <iostream>
#include <map>

const std::string PARAM_DELIMITER = "##";
const std::string REQUEST_DELIMITER = "\r\n";
const int LEN_BUFF = 4096;

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
// Define available server error types
enum class Server_error
{
    WRONG_CREDENTIALS,
    UNKNOWN_COMMAND,
    WRONG_N_ARGS,
    FILE_EXIST,
    UNKNOWN_ERROR,
    FILE_CREATE_ERROR,
    FILE_HASH_MISMATCH,
    FILE_ALREADY_EXISTS,
    FILE_DELETE_ERROR,
    FILE_NOT_FOUND,
    FILE_OPEN_ERROR
};

const std::map<Server_error, int> ERROR_COD = {
        {Server_error::WRONG_CREDENTIALS, 1}, // "LOGIN USER PWD"
        {Server_error::UNKNOWN_COMMAND, 2},   //  "GET FILPATH"
        {Server_error::WRONG_N_ARGS, 3},
        {Server_error::FILE_EXIST, 4},
        {Server_error::UNKNOWN_ERROR, 5},
        {Server_error::FILE_CREATE_ERROR, 6},
        {Server_error::FILE_HASH_MISMATCH, 7},
        {Server_error::FILE_ALREADY_EXISTS, 8},
        {Server_error::FILE_DELETE_ERROR, 9},
        {Server_error::FILE_NOT_FOUND, 10},
        {Server_error::FILE_OPEN_ERROR, 11}
};

#endif
