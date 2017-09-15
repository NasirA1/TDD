#pragma once
#include "transport.h"
#include <memory>
#include <thread>
#include <chrono>
#include "Tokeniser.h"
#include <iostream>

#ifdef GetMessage
#undef GetMessage
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
	ChatChannel() : callbackHandler_(nullptr) {}
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

