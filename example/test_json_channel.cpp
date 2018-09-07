#include <iguana/json.hpp>
#include <assert.h>
#include <string>
#include <vector>
typedef std::string String;

struct UserMsg
{
    int		 id;
    String	msg;

    bool operator == (const UserMsg& p) const
    {
        return id == p.id && msg == p.msg;
    }
};
REFLECTION(UserMsg, id, msg);

struct seat
{
    int id;
    String ip;
};
typedef std::vector<seat> seats;
REFLECTION(seat, id, ip);

struct channel
{
    String name;
    String seats;
};
typedef std::vector<channel> channels;
REFLECTION(channel, name, seats);

template<typename T>
String ToJsonString(const T& data)
{
    iguana::string_stream ss;
    iguana::json::to_json(ss, data);

    return ss.str();
}

template<typename T>
bool ToUserData(T& data, const String& str)
{
    return iguana::json::from_json(data, str.c_str());
}

void test_channel()
{
    seat st;
    seats sts;

    for (int i = 0; i < 3; ++i)
    {
        st.id = i;
        st.ip = "127.0.0.1:900" + std::to_string(i);

        sts.push_back(st);
    }

    channels datas;
    channel ch;

    for (int i = 0; i < 3; ++i)
    {
        ch.name = "测试" + std::to_string(i + 100);
        ch.seats = ToJsonString(sts);

        datas.push_back(ch);
    }

    auto jsonStr = ToJsonString(datas);

    channels datas2;
    bool bOK = ToUserData(datas2, jsonStr);

    assert(bOK);
    assert(datas2.size() == datas.size());
    assert(datas[0].seats.size() == datas2[0].seats.size());

    UserMsg msg;
    msg.id = 0;
    msg.msg = jsonStr;

    auto str = ToJsonString(msg);
    UserMsg msg2;
    bOK = ToUserData(msg2, str);
    // bug: 这里还原不出来了，打印了错误提示
    assert(bOK);
    assert(msg.id == msg2.id);
    assert(msg.msg == msg2.msg);

    datas2.clear();
    bOK = ToUserData(datas2, msg2.msg);
    assert(bOK);
    assert(datas2.size() == datas.size());
    assert(datas[0].seats.size() == datas2[0].seats.size());

}