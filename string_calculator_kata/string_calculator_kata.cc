#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

using namespace std;
using namespace testing;



struct Tokeniser
{
	Tokeniser(const string& s, const string& d = ",\n")
		: str(s)
		, delims(d)
		, offset(0)
		, delim_pos(str.find_first_of(delims, offset))
		, has_next_(true)
	{}

	bool HasNext() const { return has_next_; }

	string NextToken()
	{
		auto token = str.substr(offset, delim_pos - offset);
		offset = delim_pos + 1;
		delim_pos = str.find_first_of(delims, offset);
		if (offset == 0)
			has_next_ = false;
		return token;
	}

private:
	const string& str;
	const string delims;
	size_t offset;
	size_t delim_pos;
	bool has_next_;
};



pair<string, string> Slice(const string& str)
{
	string delims = ",\n";
	string data = str;

	if (str.substr(0, 2) == "//")
	{
		auto newline_pos = str.find('\n');
		if (newline_pos != string::npos)
		{
			delims += str.substr(2, newline_pos - 2);
			data = str.substr(newline_pos + 1);
		}
	}

	return make_pair(delims, data);
}



int ToNumber(const std::string& s)
{
	auto n = atoi(s.c_str());
	if (n < 0)
		throw std::range_error("negatives not allowed");
	else if (n > 1000)
		return 0;
	return n;
}



int Add(const string& str)
{
	auto pair = Slice(str);
	Tokeniser tokeniser(pair.second, pair.first);

	auto sum = 0;
	while (tokeniser.HasNext())
	{
		sum += ToNumber(tokeniser.NextToken());
	}

	return sum;
}


struct StringCalculatorUi
{
	virtual string GetInput() const = 0;
	virtual void Display(const std::string& msg) = 0;
	virtual void NextInput() = 0;
};

struct StringCalculatorConsoleUi : public StringCalculatorUi
{
	StringCalculatorConsoleUi(const char* const in)
		: input_(in)
	{}

	virtual string GetInput() const override { return input_; }
	virtual void Display(const std::string& msg) override { cout << msg << endl; }
	virtual void NextInput() override { getline(cin, input_); }

private:
	string input_;
};


void scalc(StringCalculatorUi& ui)
{
	auto input = ui.GetInput();
	if (input == "")
		return;

	while (true)
	{
		auto result = Add(input);
		auto msg = std::string("The result is ") + to_string(result);
		ui.Display(msg);

		ui.Display("another input please");

		ui.NextInput();
		input = ui.GetInput();
		if (input == "")
			return;
	}
}


TEST(StringCalculator, Add_EmptyString_ReturnsZero)
{
	ASSERT_EQ(0, Add(""));
}

TEST(StringCalculator, Add_SingleNumber_ReturnsThatNumber)
{
	ASSERT_EQ(1, Add("1"));
}

TEST(StringCalculator, Add_SingleNumber_ReturnsThatNumber_2)
{
	ASSERT_EQ(2, Add("2"));
}

TEST(StringCalculator, Add_SingleNumber_ReturnsThatNumber_3)
{
	ASSERT_EQ(99, Add("99"));
}

TEST(StringCalculator, Add_TwoNumbers_ReturnsSum)
{
	ASSERT_EQ(3, Add("1,2"));
}

TEST(StringCalculator, Add_MultipleNumbers_ReturnsSum)
{
	ASSERT_EQ(7, Add("2,1,4"));
}

TEST(StringCalculator, Add_MultipleNumbers_ReturnsSum_2)
{
	ASSERT_EQ(200, Add("11,100,89"));
}

TEST(StringCalculator, Add_SupportsNewlineDelimiter)
{
	ASSERT_EQ(200, Add("11\n100,89"));
}

TEST(StringCalculator, Add_SupportsNewlineDelimiter_2)
{
	ASSERT_EQ(5, Add("1\n1\n3"));
}

TEST(StringCalculator, Add_SupportsDifferentDelimiters)
{
	ASSERT_EQ(3, Add("//;\n1;2"));
}

TEST(StringCalculator, Add_SupportsDifferentDelimiters_2)
{
	ASSERT_EQ(3, Add("//$\n1$2"));
}

TEST(StringCalculator, Add_NegativeNumbersThrowException)
{
	ASSERT_THROW(Add("-1,-2"), std::range_error);
}

TEST(StringCalculator, Add_NumbersBiggerThan1000AreIgnored)
{
	ASSERT_EQ(2, Add("2, 1001"));
}

TEST(StringCalculator, Add_DelimitersCanBeOfAnyLength)
{
	ASSERT_EQ(6, Add("//[***]\n1***2***3"));
}

TEST(StringCalculator, Add_AllowsMultipleDelimiters)
{
	ASSERT_EQ(6, Add("//[*][%]\n1*2%3"));
}



struct MockStringCalculatorUi : public StringCalculatorUi
{
	MOCK_CONST_METHOD0(GetInput, string());
	MOCK_METHOD1(Display, void(const string&));
	MOCK_METHOD0(NextInput, void());
};



TEST(StringCalculator, scalc_REPLLoopsTheStringCalculatorAndTerminatesOnEmptyString)
{
	NiceMock<MockStringCalculatorUi> ui;
	{
		InSequence seq;
		EXPECT_CALL(ui, GetInput()).WillOnce(Return("1,2"));
		EXPECT_CALL(ui, Display("The result is 3"));
		EXPECT_CALL(ui, Display("another input please"));
		EXPECT_CALL(ui, NextInput());

		EXPECT_CALL(ui, GetInput()).WillOnce(Return("3,4"));
		EXPECT_CALL(ui, Display("The result is 7"));
		EXPECT_CALL(ui, Display("another input please"));
		EXPECT_CALL(ui, NextInput());

		EXPECT_CALL(ui, GetInput()).WillOnce(Return(""));
		EXPECT_CALL(ui, Display(_)).Times(0);
	}

	scalc(ui);
}



#define TESTING

int main(int argc, char* argv[])
{
#ifdef TESTING
	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
#else
	StringCalculatorConsoleUi ui(argv[1]);
	scalc(ui);

#endif
}
