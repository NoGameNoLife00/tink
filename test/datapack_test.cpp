#include <gtest/gtest.h>

#include <message.h>
#include <datapack.h>
#include <config_mng.h>
#include <Message_handler.h>
#include <message_handler.h>
#include <memory>

TEST(datapackTest, test1) {
    auto globalObj = ConfigMngInstance;
    tink::DataPack dp;
    tink::NetMessage msg;
    UBytePtr buf(new byte[5] {'t','i','n','k','\0'});
    std::shared_ptr<tink::MessageHandler> msg_handler(new tink::MessageHandler);

    msg.SetId(1);
    msg.SetDataLen(5);
    msg.SetData(buf);

//    std::shared_ptr<tink::NetMessage> msg_ptr(&msg);
    UBytePtr out(new byte[1024]);
    uint out_len;
    dp.Pack(msg, out, out_len);


    tink::NetMessage out_msg;
    dp.Unpack(out,out_msg);
    EXPECT_EQ(msg.GetId(), out_msg.GetId());
    EXPECT_EQ(msg.GetDataLen(), out_msg.GetDataLen());
    EXPECT_STREQ(msg.GetData().get(), "tink");
}