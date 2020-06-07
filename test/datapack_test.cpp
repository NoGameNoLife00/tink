#include <gtest/gtest.h>

#include <message.h>
#include <datapack.h>
#include <global_mng.h>
#include <imsg_handler.h>
#include <msg_handler.h>
//using tink::DataPack;
//using tink::Message;
TEST(datapackTest, test1) {
    auto globalObj = tink::Singleton<tink::GlobalMng>::GetInstance();
    tink::DataPack dp;
    tink::Message msg;
    std::shared_ptr<byte> buf(new byte[5] {'t','i','n','k','\0'});
    std::shared_ptr<tink::IMsgHandler> msg_handler(new tink::MsgHandler());

    msg.SetId(1);
    msg.SetDataLen(5);
    msg.SetData(buf);

//    std::shared_ptr<tink::Message> msg_ptr(&msg);
    byte * out;
    uint out_len;
    dp.Pack(msg, &out, &out_len);


    tink::Message out_msg;
    dp.Unpack(out,out_msg);
    EXPECT_EQ(msg.GetId(), out_msg.GetId());
    EXPECT_EQ(msg.GetDataLen(), out_msg.GetDataLen());
    EXPECT_STREQ(msg.GetData().get(), "tink");
}