#include <gtest/gtest.h>

#include <message.h>
#include <datapack.h>
#include <global_mng.h>
//using tink::DataPack;
//using tink::Message;
TEST(datapackTest, test1) {
    auto globalObj = tink::Singleton<tink::GlobalMng>::GetInstance();
    tink::DataPack dp;
    tink::Message msg;
    std::shared_ptr<byte> buf(new byte[5] {'t','i','n','k','\0'});


    msg.SetId(1);
    msg.SetDataLen(5);
    msg.SetData(buf);

//    std::shared_ptr<tink::Message> msg_ptr(&msg);
    byte * buf_out = new byte[globalObj->GetMaxPackageSize()] {0};
    dp.Pack(msg, buf_out);


    tink::Message out_msg;
    dp.Unpack(buf_out,out_msg);
    EXPECT_EQ(msg.GetId(), out_msg.GetId());
    EXPECT_EQ(msg.GetDataLen(), out_msg.GetDataLen());
    EXPECT_STREQ(msg.GetData().get(), "tink");
}