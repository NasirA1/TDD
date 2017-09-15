#ifndef WIN32
#include "wx/wx.h"
#include <wx/textctrl.h>
#include <wx/sizer.h>
#endif
#include "Chatter.h"
#ifdef WIN32
#include "wx/wx.h"
#include <wx/textctrl.h>
#include <wx/sizer.h>
#endif


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



struct MywxApp : public wxApp
{
	bool OnInit() override
	{
		//TODO Use program options, test first and validate!
		if (wxApp::argc < 3)
		{
			cerr << "Usage: chatter_app my_ip:port peer_ip:port" << endl;
			return false;
		}

		my_endpoint_ = wxApp::argv[1];
		peer_endpoint_ = wxApp::argv[2];

		//Model
		channel_ = make_unique<UdpChatChannel>(my_endpoint_, peer_endpoint_);

		//Presenter
		presenter_ = make_unique<ChatterPresenter>(*channel_);

		//View
		view_ = new wxChatterView(*presenter_);
		SetTopWindow(dynamic_cast<wxFrame*>(view_));

		presenter_->Initialise();
		return true;
	}


private:
	string my_endpoint_;
	string peer_endpoint_;
	unique_ptr<UdpChatChannel> channel_;
	ChatterView* view_;
	unique_ptr<ChatterPresenter> presenter_;
};

DECLARE_APP(MywxApp)
IMPLEMENT_APP(MywxApp)
