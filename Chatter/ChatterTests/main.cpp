#ifndef WIN32
#include "wx/wx.h"
#include <wx/textctrl.h>
#include <wx/sizer.h>
#endif
#include "transport.h"
#include <memory>
#include <thread>
#include <chrono>
#include "Tokeniser.h"
#ifdef WIN32
#include "wx/wx.h"
#include <wx/textctrl.h>
#include <wx/sizer.h>
#endif

using namespace std;
using namespace chrono;



void ExtractIpAndPort(const string& endpoint, string& ip, unsigned short& port)
{
	Tokeniser tokeniser(endpoint, ":");
	if (tokeniser.HasNext())
		ip = tokeniser.NextToken();
	if (tokeniser.HasNext())
		port = atoi(tokeniser.NextToken().c_str());
}



struct ChannelCallbackHandler
{
	virtual void OnMessageReceived(const string& message) = 0;
};


struct ChatChannel
{
	ChatChannel() : callbackHandler_(nullptr){}
	void SetCallbackHandler(ChannelCallbackHandler* callbackHandler) { callbackHandler_ = callbackHandler; }

	virtual bool Initialise() = 0;
	virtual bool IsOpen() const = 0;
	virtual void SendMessage(const std::string& message) = 0;
	virtual bool ReceiveMessage(std::string& message) = 0;
	virtual std::string ToString() const = 0;
	virtual ~ChatChannel() {}

protected:
	ChannelCallbackHandler* callbackHandler_;	
};


struct ChatterView
{
	virtual void Show() = 0;
	virtual void SetStatus(const std::string& text) = 0;
	virtual std::string GetMessage() const = 0;
	virtual size_t GetMessageLength() const = 0;
	virtual void SetMessage(const std::string& message) = 0;
	virtual void AppendToChatHistory(const std::string& message) = 0;
};



class UdpChatChannel : public ChatChannel
{
public:
	UdpChatChannel(const string& my_endpoint, const string& peer_endpoint)
		: my_ip_("")
		, my_port_(0u)
		, peer_ip_("")
		, peer_port_(0u)
		, send_socket_(nullptr)
		, recv_socket_(nullptr)
		, worker_(nullptr)
		, received_message_count_(0lu)
		, stopWorker_(false)
	{
		ExtractIpAndPort(my_endpoint, my_ip_, my_port_);
		ExtractIpAndPort(peer_endpoint, peer_ip_, peer_port_);
	}


	~UdpChatChannel()
	{
		stopWorker_ = true;
		if (worker_)
			worker_->join();
		cout << "Channel destroyed." << endl;
	}


	string GetMyIpAddress() const { return my_ip_; }
	unsigned short GetMyPort() const { return my_port_; }
	string GetPeerIpAddress() const { return peer_ip_; }
	unsigned short GetPeerPort() const { return peer_port_; }
	size_t ReceivedMessageCount() const { return received_message_count_; }


	string ToString() const override
	{
		return my_ip_ + ':' + to_string(my_port_)
			+ " <---> " + peer_ip_ + ':' + to_string(peer_port_);
	}


	bool Initialise() override
	{
		if (IsOpen())
		{
			cout << "Error: Already running!" << endl;
			return false;
		}

		send_socket_ = make_unique<UdpSocket>(peer_port_, peer_ip_.c_str());
		recv_socket_ = make_unique<UdpSocket>(my_port_, my_ip_.c_str());

		while (!recv_socket_->Bind())
			this_thread::sleep_for(500ms);

		worker_ = make_unique<thread>(&UdpChatChannel::ReceiveLoop, this);
		cout << "Channel initialised." << endl;
		return true;
	}


	bool IsOpen() const override
	{
		return send_socket_ && recv_socket_ &&
			send_socket_->IsOpen() && recv_socket_->IsOpen() && worker_;
	}


	void SendMessage(const std::string& message) override
	{
		send_socket_->SendTo(peer_ip_.c_str(), message);
	}


	bool ReceiveMessage(std::string& message) override
	{
		auto request = recv_socket_->RecvFrom<string>(150);
		//auto& req_ip = request.first; 
		if (request.second.size())
		{
			message = request.second;
			return true;
		}
		return false;
	}


private:
	void ReceiveLoop()
	{
		string buffer;
		while (!stopWorker_)
		{
			if (ReceiveMessage(buffer))
			{
				received_message_count_++;

				if (callbackHandler_)
					callbackHandler_->OnMessageReceived(buffer);
			}

			this_thread::sleep_for(1ms);
		}
	}



private:
	string my_ip_;
	unsigned short my_port_;
	string peer_ip_;
	unsigned short peer_port_;
	unique_ptr<UdpSocket> send_socket_;
	unique_ptr<UdpSocket> recv_socket_;
	unique_ptr<thread> worker_;
	size_t received_message_count_;
	bool stopWorker_;
};



class ChatterPresenter : public ChannelCallbackHandler
{
public:
	ChatterPresenter(ChatChannel& channel)
		: channel_(channel)
		, view_(nullptr)
	{
		channel_.SetCallbackHandler(this);
	}

	void SetView(ChatterView* view)
	{
		view_ = view;
	}

	void OnSendCommand()
	{
		string msg = view_->GetMessage();
		msg += '\n';
		view_->AppendToChatHistory(msg);
		channel_.SendMessage(msg);
		view_->SetMessage("");
	}

	bool CanSend() const
	{
		return view_->GetMessageLength() > 0;
	}

	void Initialise()
	{
		channel_.Initialise();
		view_->Show();
		view_->SetStatus(channel_.ToString());
	}

	void OnMessageReceived(const string& message) override
	{
		view_->AppendToChatHistory(message + "\n");
	}


private:
	ChatChannel& channel_;
	ChatterView* view_;
};



enum
{
	TEXT_CHATHISTORY = wxID_HIGHEST + 1,
	TEXT_MESSAGE,
	BUTTON_SEND = wxID_OK
};


class wxChatterView : public ChatterView, private wxFrame
{
public:
	wxChatterView(ChatterPresenter& presenter)
		: wxFrame(nullptr, -1, "Chatter", wxDefaultPosition, wxDefaultSize)
		, presenter_(presenter)
		, szr_content_(new wxBoxSizer(wxVERTICAL))
		, szr_bottom_(new wxBoxSizer(wxHORIZONTAL))
		, txt_chat_history_(new wxTextCtrl(this, TEXT_CHATHISTORY, "", wxDefaultPosition, wxSize(400, 200)
			, wxTE_MULTILINE | wxTE_RICH, wxDefaultValidator, wxTextCtrlNameStr))
		, txt_message_(new wxTextCtrl(this, TEXT_MESSAGE, "", wxDefaultPosition, wxDefaultSize))
		, btn_send_(new wxButton(this, BUTTON_SEND, "Send"))
	{
		CreateStatusBar();

		szr_bottom_->Add(txt_message_, wxSizerFlags(1).Border(wxALL, 10).Expand());
		szr_bottom_->Add(btn_send_, wxSizerFlags(0).Border(wxALL, 10));

		szr_content_->Add(txt_chat_history_, wxSizerFlags(1).Border(wxALL, 10).Expand());
		szr_content_->Add(szr_bottom_, wxSizerFlags(0).Expand());
		SetSizerAndFit(szr_content_);

		txt_chat_history_->SetEditable(false);
		txt_chat_history_->SetBackgroundColour(wxColor(200, 200, 200));
		txt_message_->SetFocus();

		presenter_.SetView(this);
	}


	void Show() override
	{
		wxFrame::Show(true);
	}

	void SetStatus(const std::string& text) override
	{
		SetStatusText(text);
	}

	std::string GetMessage() const override
	{
		return txt_message_->GetValue().ToStdString();
	}

	void AppendToChatHistory(const std::string& text) override
	{
		txt_chat_history_->AppendText(text.c_str());
	}

	void SetMessage(const std::string& message) override
	{
		txt_message_->SetValue(message.c_str());
	}

	size_t GetMessageLength() const override
	{
		return txt_message_->GetLineLength(0);
	}


private:
	void OnButtonSend(wxCommandEvent& event)
	{
		presenter_.OnSendCommand();
	}

	void OnButtonSendUpdateUI(wxUpdateUIEvent & event)
	{
		event.Enable(presenter_.CanSend());
	}


	DECLARE_EVENT_TABLE()

private:
	ChatterPresenter& presenter_;
	wxBoxSizer* szr_content_;
	wxBoxSizer* szr_bottom_;
	wxTextCtrl* txt_chat_history_;
	wxTextCtrl* txt_message_;
	wxButton* btn_send_;
};

BEGIN_EVENT_TABLE(wxChatterView, wxFrame)
EVT_BUTTON(BUTTON_SEND, wxChatterView::OnButtonSend)
EVT_UPDATE_UI(BUTTON_SEND, wxChatterView::OnButtonSendUpdateUI)
END_EVENT_TABLE();




#define TESTING

#ifndef TESTING

struct MywxApp : public wxApp
{
	bool OnInit() override
	{
		//Model
		channel_ = make_unique<UdpChatChannel>("127.0.0.1:2000", "127.0.0.1:2001");

		//Presenter
		presenter_ = make_unique<ChatterPresenter>(*channel_);

		//View
		view_ = new wxChatterView(*presenter_);
		SetTopWindow(dynamic_cast<wxFrame*>(view_));

		presenter_->Initialise();
		return true;
	}


private:
	unique_ptr<UdpChatChannel> channel_;
	ChatterView* view_;
	unique_ptr<ChatterPresenter> presenter_;
};

DECLARE_APP(MywxApp)
IMPLEMENT_APP(MywxApp)
#endif




/////////////////////////////////////////// TEST CODE //////////////////////////////////////////////


//#ifdef TESTING
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

#endif
