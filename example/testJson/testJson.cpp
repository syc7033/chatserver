#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

// json序列表示例1
string func1(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";
    return js.dump();
}

string func2(){
    json js;
    js["id"] = {1,2,3,4,5};
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["li si"] = "no no no";
    // js["msg"] = {{"zhang san", "hello world"}, {"li si", "no no no"}};
    
    string sendBuf = js.dump();

    return sendBuf;
}
// json序列化STL容器
string func3(){
    json js;

    vector<int> vec = {5, 3, 1, 4, 2};
    js["list"] = vec;

    map<int, string> mp;
    mp.insert({1, "黄山"});
    mp.insert({2, "泰山"});
    mp.insert({3, "华山"});

    js["path"] = mp;
    string sendBuf = js.dump(); // json数据对象 --> 序列化成json字符串
    // cout << sendBuf << endl;
    return sendBuf;
}
int main(){
    // string recvBuf = func1(); // json字符串 --> 反序列化成json数据对象
    
    // json jsBuf = json::parse(recvBuf);
    // cout << jsBuf["msg_type"] << endl;
    // cout << jsBuf["from"] << endl;
    // cout << jsBuf["to"] << endl;
    // cout << jsBuf["msg"] << endl;
    // cout << 1 << endl;
    // string recvBuf = func2();
    // json jsBuf = json::parse(recvBuf);

    // auto arr = jsBuf["id"];
    // for(auto it : arr){
    //     cout << it << ' ';        
    // }cout << endl;
    // auto json1 = jsBuf["msg"];
    // cout << json1["zhang san"] << endl;
    // cout << json1["li si"] << endl;
    string recBuf = func3();
    auto jsBuf = json::parse(recBuf);
    auto list = jsBuf["list"];
    
    for(auto it : list){
        cout << it << ' ';
    }cout << endl;

    map<int, string> pathMap = jsBuf["path"];
    for(auto it : pathMap){
        cout << it.first << ' ' << it.second << ' '<< endl;
    }
    return 0;
}