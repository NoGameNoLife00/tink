#include <gtest/gtest.h>

#include <message.h>
#include <datapack.h>
using tink::DataPack;
using tink::Message;
TEST(datapackTest, test1) {
    DataPack dp;
    Message msg;
    std::shared_ptr<byte> buf(new byte[5] {'t','i','n','k','\n'});
    msg.SetId(1);
    msg.SetDataLen(5);
    msg.SetData(buf);
    std::shared_ptr<Message> msg_ptr(&msg);
    dp.Pack(msg_ptr, &buf);


    std::shared_ptr<byte> new_buf;
    dp.Unpack(new_buf,msg_ptr);
    EXPECT_EQ(msg_ptr->GetId(), 1);
    EXPECT_EQ(msg_ptr->GetDataLen(), 5);
    EXPECT_STREQ(msg_ptr->GetData().get(), "tink");
}