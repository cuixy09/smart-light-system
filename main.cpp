#include "httplib.h" // HTTP 客户端库

#ifdef _WIN32
#include <windows.h> // 串口通信等
#endif

#include <iostream>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <Python.h> // 调用 Python API
#include <sstream>
#include <thread>
#include <chrono>
#include <utility>

const double pi = 3.1415926535; // 不要求精度，常记的 PI 值就够用
std::string arduino_port = "\\\\.\\"; // 输入端口号

PyObject* g_py_func = nullptr;
// 定义 JSON 结构体
struct ManualConfig {
    bool active = false; // 手动 or not
    std::string date; // 日期
    int hour = 20; // 小时
    int minute = 0; // 分钟
    int aqi = 85; // AQI
    std::string sunrise; // 指定日出
    std::string sunset; // 指定日落
};
// 简易 JSON 解析字符串值
std::string extract_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search); // 找 key
    pos = json.find(":", pos) + 1; // 找值
    while (pos < json.size() && std::isspace(json[pos])) pos++; // 跳过空格
    if (json[pos] == '\"') {
        pos++;
        size_t end = json.find("\"", pos);
        return json.substr(pos, end - pos); // 提取
    }
    return "";
}
// 简易 JSON 解析整数值，原理同上
int extract_int(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    pos = json.find(":", pos) + 1;
    while (pos < json.size() && std::isspace(json[pos])) pos++;
    return std::stoi(json.substr(pos));
}
// 简易 JSON 解析布尔值，原理同上
bool extract_bool(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    pos = json.find(":", pos) + 1;
    while (pos < json.size() && std::isspace(json[pos])) pos++;
    if (json.compare(pos, 4, "true") == 0) return true;
    if (json.compare(pos, 5, "false") == 0) return false;
    if (json[pos] == '\"') {
        pos++;
        size_t end = json.find("\"", pos);
        std::string val = json.substr(pos, end - pos);
        return val == "true";
    }
    return false;
}
// 初始化 Python API 调用
bool init_python_api() {
    PyObject* pModule = PyImport_ImportModule("weather_api");
    if (!pModule) {
        PyErr_Print(); // Python 本身有错误信息
        return false;
    }
    g_py_func = PyObject_GetAttrString(pModule, "grab_data"); // 查找指定函数
    Py_DECREF(pModule); // 避免内存泄漏
    if (!g_py_func || !PyCallable_Check(g_py_func)) {
        PyErr_Print();
        Py_XDECREF(g_py_func);
        g_py_func = nullptr;
        return false;
    }
    return true;
}
// 利用时间戳计算当前日期的天数
int day_of_year(int Y, int M, int D) {
    std::tm tm = {0};
    tm.tm_year = Y - 1900;
    tm.tm_mon  = M - 1;
    tm.tm_mday = D;
    std::mktime(&tm);
    return tm.tm_yday + 1;
}
// 数学模型计算日出日落时间，误差 ±5min
std::pair<int, int> cal_times(int Y, int M, int D) {
    int doy = day_of_year(Y, M, D);
    double angle = 2.0 * pi * (doy - 80) / 365.0;
    double sin_val = std::sin(angle);
    int sr = 326 + static_cast<int>(94 * sin_val);
    int ss = 1046 - static_cast<int>(98 * sin_val);
    return {sr, ss};
}
// 获取系统日期，引用修改
void get_cur_date(int& Y, int& M, int& D) {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    Y = now->tm_year + 1900;
    M = now->tm_mon + 1;
    D = now->tm_mday;
}
// 字符串格式转换为分钟数
int str_to_min(const std::string& t) {
    int h = std::stoi(t.substr(0, 2));
    int m = std::stoi(t.substr(3, 2));
    return h * 60 + m;
}
// 日期转换为字符串格式
std::string date_to_str(int Y, int M, int D) {
    std::ostringstream oss;
    oss << Y << '-' << std::setw(2) << std::setfill('0') << M << '-' << std::setw(2) << std::setfill('0') << D;
    return oss.str();
}
// 调用 Python API
bool call_py(const std::string& date_str, int& sr, int& ss, int& aqi) {
    if (!g_py_func) return false;
    // 构建元组
    PyObject* pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(date_str.c_str()));
    // 调用函数
    PyObject* pResult = PyObject_CallObject(g_py_func, pArgs);
    Py_DECREF(pArgs);
    if (!pResult) {
        PyErr_Clear();
        return false;
    }
    bool ok = false; // 是否获取到有效元组
    if (PyTuple_Check(pResult) && PyTuple_Size(pResult) == 3) {
        PyObject* py_sr = PyTuple_GetItem(pResult, 0);
        PyObject* py_ss = PyTuple_GetItem(pResult, 1);
        PyObject* py_aqi = PyTuple_GetItem(pResult, 2);
        if (py_sr != Py_None && py_ss != Py_None && py_aqi != Py_None) {
            std::string sr_str = PyUnicode_AsUTF8(py_sr);
            std::string ss_str = PyUnicode_AsUTF8(py_ss);
            aqi = static_cast<int>(PyLong_AsLong(py_aqi));
            sr = str_to_min(sr_str);
            ss = str_to_min(ss_str);
            ok = true;
        }
    }
    Py_DECREF(pResult);
    return ok;
}
// 计算分钟数
int get_cur_min() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    return now->tm_hour * 60 + now->tm_min;
}
// 核心部分：PWM 的计算
int cal_pwm(int cur_min, int sr, int ss, int aqi) {
    bool is_night = (cur_min >= ss || cur_min < sr); // 晚上？
    if (!is_night) return 0;
    int elapsed;
    if (cur_min >= ss) elapsed = cur_min - ss; // 已过时间
    else elapsed = (24 * 60 - ss) + cur_min; // 后半夜
    int night_time = (24 * 60 - ss) + sr; // 总时长
    double lambda = elapsed * 1.0 / night_time; // 各种计算
    double base = 200.0 * (1.0 - 2.0 * std::abs(lambda - 0.5));
    double aqi_fac = 1.0 + (aqi - 50) / 200.0;
    if (aqi_fac < 0.5) aqi_fac = 0.5; // 限定因子修正
    if (aqi_fac > 2.0) aqi_fac = 2.0;
    int pwm = static_cast<int>(base * aqi_fac);
    if (pwm > 200) pwm = 200; // 防止计算结果溢出
    if (pwm < 0) pwm = 0;
    return pwm;
}
// 打开串口
HANDLE init_arduino_serial(const std::string& port) {
    HANDLE hSerial = CreateFileA(port.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hSerial, &dcb)) {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }
    dcb.BaudRate = CBR_9600; // 比特率 9600
    dcb.ByteSize = 8; // 8 数据位
    dcb.StopBits = ONESTOPBIT; // 1 停止位
    dcb.Parity = NOPARITY; // 无校验
    if (!SetCommState(hSerial, &dcb)) {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }
    return hSerial;
}
// 通信
void send_pwm_to_arduino(HANDLE hSerial, int pwm) {
    std::string data = std::to_string(pwm) + '\n'; // 示例格式："255\n"
    DWORD written;
    WriteFile(hSerial, data.c_str(), static_cast<DWORD>(data.size()), &written, NULL);
}
// 网页交互
bool fetch_manual_config(ManualConfig& cfg) {
    httplib::Client cli("http://localhost:3000"); // 端口自行修改
    cli.set_connection_timeout(0, 500000);
    cli.set_read_timeout(0, 500000);
    auto res = cli.Get("/api/get_manual"); // 根据后端情况修改
    if (!res || res->status != 200) return false;
    std::string json_str = res->body; // 一堆提取
    cfg.active = extract_bool(json_str, "active");
    if (!cfg.active) return false;
    cfg.date = extract_string(json_str, "date");
    cfg.hour = extract_int(json_str, "hour");
    cfg.minute = extract_int(json_str, "minute");
    cfg.aqi = extract_int(json_str, "aqi");
    cfg.sunrise = extract_string(json_str, "sunrise");
    cfg.sunset = extract_string(json_str, "sunset");
    return true;
}

int main() {
    Py_Initialize();
    PyRun_SimpleString("import sys; sys.path.append('.')");

    if (!init_python_api()) {
        std::cerr << "Failed to init Python.\n";
        Py_Finalize();
        return 1;
    }
	
    std::string port = "COM3";
    std::cout << "Please Enter A Port(COMX):";
    std::cin >> port;
    arduino_port += port;

    HANDLE hSerial = init_arduino_serial(arduino_port);

    int pre_aqi = 50; // 给一个初始值，防止一直失败

    while (true) {
        int sr, ss, aqi, cur_min;
        ManualConfig man;
        bool use_manual = fetch_manual_config(man);
        // 手动？
        if (use_manual) {
            cur_min = man.hour * 60 + man.minute;
            sr = str_to_min(man.sunrise);
            ss = str_to_min(man.sunset);
            aqi = man.aqi;
        }
        // 自动
        else {
            int Y, M, D;
            get_cur_date(Y, M, D);
            cur_min = get_cur_min();
            std::string date_str = date_to_str(Y, M, D);

            if (call_py(date_str, sr, ss, aqi)) pre_aqi = aqi;
			else { // 一个失败就全部估算
                auto est = cal_times(Y, M, D);
                sr = est.first;
                ss = est.second;
                aqi = pre_aqi;
            }
        }
        // 计算
        int pwm = cal_pwm(cur_min, sr, ss, aqi);
        std::cout << pwm << '\n';
		// 通信
        send_pwm_to_arduino(hSerial, pwm);
        // 睡 1 秒 (bushi)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // 没写 while 出口，可以自己写
    Py_XDECREF(g_py_func);
    Py_Finalize();
	
    return 0;
}