#define _CRT_SECURE_NO_WARNINGS

#include "httplib.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <Python.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <utility>

const double pi = 3.1415926535;
const std::string arduino_port = "\\\\.\\COM6";

PyObject* g_py_func = nullptr;

struct ManualConfig {
    bool active = false;
    std::string date;
    int hour = 20;
    int minute = 0;
    int aqi = 85;
    std::string sunrise;
    std::string sunset;
};

std::string extract_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(":", pos) + 1;
    while (pos < json.size() && std::isspace(json[pos])) pos++;
    if (json[pos] == '\"') {
        pos++;
        size_t end = json.find("\"", pos);
        return json.substr(pos, end - pos);
    }
    return "";
}

int extract_int(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return -1;
    pos = json.find(":", pos) + 1;
    while (pos < json.size() && std::isspace(json[pos])) pos++;
    return std::stoi(json.substr(pos));
}

bool extract_bool(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;
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

bool init_python_api() {
    PyObject* pModule = PyImport_ImportModule("weather_api");
    if (!pModule) {
        PyErr_Print();
        return false;
    }
    g_py_func = PyObject_GetAttrString(pModule, "grab_data");
    Py_DECREF(pModule);
    if (!g_py_func || !PyCallable_Check(g_py_func)) {
        PyErr_Print();
        Py_XDECREF(g_py_func);
        g_py_func = nullptr;
        return false;
    }
    return true;
}

int day_of_year(int Y, int M, int D) {
    std::tm tm = {0};
    tm.tm_year = Y - 1900;
    tm.tm_mon  = M - 1;
    tm.tm_mday = D;
    std::mktime(&tm);
    return tm.tm_yday + 1;
}

std::pair<int, int> cal_times(int Y, int M, int D) {
    int doy = day_of_year(Y, M, D);
    double angle = 2.0 * pi * (doy - 80) / 365.0;
    double sin_val = std::sin(angle);
    int sr = 326 + static_cast<int>(94 * sin_val);
    int ss = 1046 - static_cast<int>(98 * sin_val);
    if (sr < 0) sr = 0;
    if (ss > 24 * 60) ss = 24 * 60;
    return {sr, ss};
}

void get_cur_date(int& Y, int& M, int& D) {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    Y = now->tm_year + 1900;
    M = now->tm_mon + 1;
    D = now->tm_mday;
}

int str_to_min(const std::string& t) {
    if (t.size() < 5 || t[2] != ':') return -1;
    int h = std::stoi(t.substr(0, 2));
    int m = std::stoi(t.substr(3, 2));
    return h * 60 + m;
}

std::string date_to_str(int Y, int M, int D) {
    std::ostringstream oss;
    oss << Y << '-' << std::setw(2) << std::setfill('0') << M << '-'
        << std::setw(2) << std::setfill('0') << D;
    return oss.str();
}

bool call_py(const std::string& date_str, int& sr, int& ss, int& aqi) {
    if (!g_py_func) return false;
    PyObject* pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(date_str.c_str()));
    PyObject* pResult = PyObject_CallObject(g_py_func, pArgs);
    Py_DECREF(pArgs);
    if (!pResult) {
        PyErr_Clear();
        return false;
    }
    bool ok = false;
    if (PyTuple_Check(pResult) && PyTuple_Size(pResult) == 3) {
        PyObject* py_sr = PyTuple_GetItem(pResult, 0);
        PyObject* py_ss = PyTuple_GetItem(pResult, 1);
        PyObject* py_aqi = PyTuple_GetItem(pResult, 2);
        if (py_sr != Py_None && py_ss != Py_None && py_aqi != Py_None && PyLong_Check(py_aqi)) {
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

int get_cur_min() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    return now->tm_hour * 60 + now->tm_min;
}

int cal_pwm(int cur_min, int sr, int ss, int aqi) {
    bool is_night = (cur_min >= ss || cur_min < sr);
    if (!is_night) return 0;
    int elapsed;
    if (cur_min >= ss) elapsed = cur_min - ss;
    else elapsed = (24 * 60 - ss) + cur_min;
    int night_time = (24 * 60 - ss) + sr;
    double lambda = elapsed * 1.0 / night_time;
    double base = 255.0 * (1.0 - 2.0 * std::abs(lambda - 0.5));
    double aqi_fac = 1.0 + (aqi - 50) / 200.0;
    if (aqi_fac < 0.5) aqi_fac = 0.5;
    if (aqi_fac > 2.0) aqi_fac = 2.0;
    int pwm = static_cast<int>(base * aqi_fac);
    if (pwm > 255) pwm = 255;
    if (pwm < 0) pwm = 0;
    return pwm;
}

HANDLE init_arduino_serial(const std::string& port) {
    HANDLE hSerial = CreateFileA(port.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hSerial, &dcb)) {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    if (!SetCommState(hSerial, &dcb)) {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }
    return hSerial;
}

void send_pwm_to_arduino(HANDLE hSerial, int pwm) {
    std::string data = std::to_string(pwm) + '\n';
    DWORD written;
    WriteFile(hSerial, data.c_str(), static_cast<DWORD>(data.size()), &written, NULL);
}

bool fetch_manual_config(ManualConfig& cfg) {
    httplib::Client cli("http://localhost:8080");
    cli.set_connection_timeout(0, 500000);
    cli.set_read_timeout(0, 500000);
    auto res = cli.Get("/get_manual");
    if (!res || res->status != 200) return false;
    std::string json_str = res->body;
    cfg.active = extract_bool(json_str, "active");
    if (!cfg.active) return false;
    cfg.date = extract_string(json_str, "date");
    cfg.hour = extract_int(json_str, "hour");
    cfg.minute = extract_int(json_str, "minute");
    cfg.aqi = extract_int(json_str, "aqi");
    cfg.sunrise = extract_string(json_str, "sunrise");
    cfg.sunset = extract_string(json_str, "sunset");
    if (cfg.date.empty() || cfg.hour < 0 || cfg.hour > 23 || cfg.minute < 0 || cfg.minute > 59) {
        cfg.active = false;
        return false;
    }
    return true;
}

int main() {
	#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
	#endif
    Py_Initialize();
    PyRun_SimpleString("import sys; sys.stdout.reconfigure(encoding='utf-8')");
    PyRun_SimpleString("sys.path.append('.')");

    if (!init_python_api()) {
        std::cerr << "Failed to init Python.\n";
        Py_Finalize();
        return 1;
    }
	
    HANDLE hSerial = init_arduino_serial(arduino_port);

    int pre_aqi = 50;

    while (true) {
        int Y, M, D;
        get_cur_date(Y, M, D);
        int cur_min = get_cur_min();
        std::string date_str = date_to_str(Y, M, D);

        int sr, ss, aqi;
        /*ManualConfig man;
        bool use_manual = fetch_manual_config(man);
        if (use_manual) {
            int year, month, day;
            if (sscanf(man.date.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                cur_min = man.hour * 60 + man.minute;
                if (!man.sunrise.empty() && !man.sunset.empty()) {
                    sr = str_to_min(man.sunrise);
                    ss = str_to_min(man.sunset);
                }
				else {
                    auto est = cal_times(year, month, day);
                    sr = est.first;
                    ss = est.second;
                }
                aqi = man.aqi;
            }
			else use_manual = false;
        }
		*/
        //if (!use_manual) {
            if (call_py(date_str, sr, ss, aqi)) pre_aqi = aqi;
			else {
                auto est = cal_times(Y, M, D);
                sr = est.first;
                ss = est.second;
                aqi = pre_aqi;
            }
        //}

        int pwm = cal_pwm(cur_min, sr, ss, aqi);
        std::cout << pwm << '\n';
		
        //send_pwm_to_arduino(hSerial, pwm);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Py_XDECREF(g_py_func);
    Py_Finalize();
	
    return 0;
}