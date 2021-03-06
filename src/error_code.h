
#ifndef TINK_ERROR_CODE_H
#define TINK_ERROR_CODE_H

namespace tink {


#define E_OK 0
#define E_FAILED -1
#define E_PACKET_SIZE 1 // 包数据过长
#define E_CONN_CLOSED 2 // 链接已经关闭
#define E_PACK_FAILED 3 // 序列化失败
#define E_UNPACK_FAILED 4 // 反序列化失败
#define E_MSG_REPEAT_ROUTER 5 // 消息id已被添加handle
#define E_CONN_NOT_FIND 6 // 找不到链接
#define E_BUFF_OVERSIZE 7 // 连接缓存已满
#define E_QUERY_MODULE 8 // 加载模块出错


}

#define MSG_ID_EXIT -1




















#endif //TINK_ERROR_CODE_H



