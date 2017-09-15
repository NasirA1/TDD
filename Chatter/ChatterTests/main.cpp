#include "Chatter.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>


using namespace testing;


struct MockChannelCallbackHandler : ChannelCallbackHandler
{
	MOCK_METHOD1(OnMessageReceived, void(const string&));
};


struct MockChatterView : public ChatterView
{
	//NiceMock only works with const ref params
	//hence the hack below  :(
	MockChatterView(const ChatterPresenter& presenter)
	{
		const_cast<ChatterPresenter&>(presenter).SetView(this);
	}

	MOCK_METHOD0(Show, void());
	MOCK_METHOD1(SetStatus, void(const std::string&));
	MOCK_CONST_METHOD0(GetMessage, std::string());
	MOCK_CONST_METHOD0(GetMessageLength, size_t());
	MOCK_METHOD1(AppendToChatHistory, void(const std::string&));
	MOCK_METHOD1(SetMessage, void(const std::string&));
};


struct MockChatChannel : public ChatChannel
{
	MOCK_METHOD0(Initialise, bool());
	MOCK_CONST_METHOD0(IsOpen, bool());
	MOCK_METHOD1(SendMessage, void(const std::string&));
	MOCK_METHOD1(ReceiveMessage, bool(std::string&));
	MOCK_CONST_METHOD0(ToString, std::string());
};



TEST(UdpChatChannel, Constructor_InitialisesEndpoints)
{
	UdpChatChannel channel("127.0.0.1:2000", "127.0.0.1:2001");

	ASSERT_EQ("127.0.0.1", channel.GetMyIpAddress());
	ASSERT_EQ(2000u, channel.GetMyPort());
	ASSERT_EQ("127.0.0.1", channel.GetPeerIpAddress());
	ASSERT_EQ(2001u, channel.GetPeerPort());
}


TEST(UdpChatChannel, ToString_ReturnsEndpoints)
{
	UdpChatChannel channel("127.0.0.1:2000", "127.0.0.1:2001");

	ASSERT_EQ("127.0.0.1:2000 <---> 127.0.0.1:2001", channel.ToString());
}


TEST(UdpChatChannel, Initialise_InitialisesChannel)
{
	UdpChatChannel channel("127.0.0.1:2000", "127.0.0.1:2001");
	ASSERT_TRUE(channel.Initialise());
	ASSERT_TRUE(channel.IsOpen());
}


TEST(UdpChatChannel, Initialise_ReturnsFalseWhenChannelAlreadyInitialised)
{
	UdpChatChannel channel("127.0.0.1:2000", "127.0.0.1:2001");
	ASSERT_TRUE(channel.Initialise());
	ASSERT_TRUE(channel.IsOpen());
	ASSERT_FALSE(channel.Initialise());
}


TEST(UdpChatChannel, OnMessageReceivedIsCalledWhenMessageIsReceived)
{
	UdpChatChannel channel1("127.0.0.1:2000", "127.0.0.1:2001");
	UdpChatChannel channel2("127.0.0.1:2001", "127.0.0.1:2000");
	ASSERT_TRUE(channel1.Initialise());
	ASSERT_TRUE(channel2.Initialise());

	MockChannelCallbackHandler handler;
	channel2.SetCallbackHandler(&handler);
	EXPECT_CALL(handler, OnMessageReceived("hi, how are you?\n"));

	channel1.SendMessage("hi, how are you?\n");

	//busy wait until the message is received 
	while (!channel2.ReceivedMessageCount())
	{
		using namespace chrono;
		this_thread::sleep_for(100ms);
	}
}


TEST(ChatterPresenter, Initialise_ChannelIsInitialised)
{
	UdpChatChannel channel("127.0.0.1:2000", "127.0.0.1:2001");
	ChatterPresenter presenter(channel);
	NiceMock<MockChatterView> view(presenter);

	presenter.Initialise();

	EXPECT_TRUE(channel.IsOpen());
}


TEST(ChatterPresenter, Initialise_ChatterViewShown)
{
	UdpChatChannel channel("127.0.0.1:2000", "127.0.0.1:2001");
	ChatterPresenter presenter(channel);
	NiceMock<MockChatterView> view(presenter);

	EXPECT_CALL(view, Show());
	presenter.Initialise();
}


TEST(ChatterPresenter, Initialise_EndpointsShownInStatusBar)
{
	UdpChatChannel channel("127.0.0.1:2000", "127.0.0.1:2001");
	ChatterPresenter presenter(channel);
	NiceMock<MockChatterView> view(presenter);

	EXPECT_CALL(view, SetStatus("127.0.0.1:2000 <---> 127.0.0.1:2001"));
	presenter.Initialise();
}


TEST(ChatterPresenter, CanSend_ReturnsFalseWhenMessageIsEmpty)
{
	NiceMock<MockChatChannel> channel;
	ChatterPresenter presenter(channel);
	NiceMock<MockChatterView> view(presenter);

	EXPECT_CALL(view, GetMessageLength()).WillOnce(Return(0lu));

	ASSERT_FALSE(presenter.CanSend());
}


TEST(ChatterPresenter, CanSend_ReturnsTrueWhenMessageNotEmpty)
{
	NiceMock<MockChatChannel> channel;
	ChatterPresenter presenter(channel);
	NiceMock<MockChatterView> view(presenter);

	EXPECT_CALL(view, GetMessageLength()).WillOnce(Return(100lu));

	ASSERT_TRUE(presenter.CanSend());
}


TEST(ChatterPresenter, OnSendMessage_MessageIsAppendedToChatHistory)
{
	UdpChatChannel channel("127.0.0.1:2000", "127.0.0.1:2001");
	ChatterPresenter presenter(channel);
	NiceMock<MockChatterView> view(presenter);

	//mock call
	EXPECT_CALL(view, GetMessage()).WillOnce(Return("hi, how are you?"));

	//expectations
	EXPECT_CALL(view, AppendToChatHistory("hi, how are you?\n"));
	EXPECT_CALL(view, SetMessage(""));

	presenter.Initialise();
	presenter.OnSendCommand();
}


TEST(ChatterPresenter, OnSendMessage_MessageIsSent)
{
	NiceMock<MockChatChannel> channel;
	ChatterPresenter presenter(channel);
	NiceMock<MockChatterView> view(presenter);

	//mock call
	EXPECT_CALL(view, GetMessage()).WillOnce(Return("hi, how are you?"));

	//expectations
	EXPECT_CALL(view, AppendToChatHistory("hi, how are you?\n"));
	EXPECT_CALL(view, SetMessage(""));
	EXPECT_CALL(channel, SendMessage("hi, how are you?\n"));

	presenter.Initialise();
	presenter.OnSendCommand();
}


TEST(ChatterPresenter, ReceivedMessageIAppendedToChatHistory)
{
	UdpChatChannel channel1("127.0.0.1:2000", "127.0.0.1:2001");
	UdpChatChannel channel2("127.0.0.1:2001", "127.0.0.1:2000");	
	ChatterPresenter presenter2(channel2);
	NiceMock<MockChatterView> view2(presenter2);

	channel1.Initialise();
	presenter2.Initialise();

	//We send message from channel1 to channel2 through the presenter
	//we expect that the message should appear in view2's chat history
	//when it is received
	EXPECT_CALL(view2, AppendToChatHistory("hi from channel1!\n"));

	channel1.SendMessage("hi from channel1!");

	//busy wait until the message is received 
	while (!channel2.ReceivedMessageCount())
	{
		using namespace chrono;
		this_thread::sleep_for(100ms);
	}
}



int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
