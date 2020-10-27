//
// Created by isnob on 27/10/2020.
//

#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H
#include "../shared/const.h"
#include "node.h"

class Request{

public:
    Request(Method method,Node node):method(method),node(node){};

    Method method;
    Node node;
};




#endif //CLIENT_REQUEST_H
