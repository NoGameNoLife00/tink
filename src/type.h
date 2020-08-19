//
// Created by yx1502004 on 2020/5/29.
//

#ifndef TINK_TYPE_H
#define TINK_TYPE_H
#include <sys/types.h>
#include <memory>

typedef char byte;
//typedef std::shared_ptr<byte> BytePtr;
typedef std::unique_ptr<byte[]> BytePtr;

typedef std::shared_ptr<std::string> StringPtr;
typedef std::shared_ptr<struct sockaddr> RemoteAddrPtr;
#endif //TINK_TYPE_H
