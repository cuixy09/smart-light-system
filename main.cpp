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

const double pi=3.1415926535;
const std::string port="\\\\.\\COM6";

int day_of_year(int Y,int M, int D) {
    std::tm tm={0};
    tm.tm_year=Y-1900;
    tm.tm_mon=M-1;
    tm.tm_mday=D;
    std::mktime(&tm);
    return tm.tm_yday+1;
}

std::pair<int, int> cal_time(int Y, int M,int D) {
    int doy=day_of_year(Y,M,D);
    double angle=2.0*pi*(doy-80)/365.0;
    double sin_val=std::sin(angle);
    int sr=326+static_cast<int>(94*sin_val);
    int ss=1046-static_cast<int>(98*sin_val);
    if (sr<0) sr=0;
    if (ss>24*60) ss=24*60;
    return {sr,ss};
}

void get_cur_date(int& Y,int& M,int& D) {
	std::time_t t=std::time(nullptr);
    std::tm* now=std::localtime(&t);
    Y=now->tm_year+1900;
    M=now->tm_mon+1;
    D=now->tm_mday;
	return;
}

int str_to_min(const std::string& t) {
    if (t.size()<5 || t[2]!=':') return -1;
    int h=std::stoi(t.substr(0,2));
    int m=std::stoi(t.substr(3,2));
    return h*60+m;
}

std::string date_to_str(int Y,int M,int D) {
	std::ostringstream oss;
	oss<<Y<<'-'<<std::setw(2)<<std::setfill('0')<<M<<'-'<<std::setw(2)<<std::setfill('0')<<D;
	return oss.str();
}

bool call_py(const std::string& date_str,int& sr,int& ss,int& aqi) {
	std::cout<<"import\n";
	PyObject* pModule=PyImport_ImportModule("weather_api");
    if (!pModule) {
        PyErr_Print();
        std::cerr<<"Error.\n";
        return false;
    }
	std::cout<<"import ok\n";
	std::cout<<"func\n";
    PyObject* pFunc=PyObject_GetAttrString(pModule,"grab_data");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        std::cerr<<"Error.\n";
        return false;
    }
	std::cout<<"func ok\n";
	std::cout<<"run\n";
    PyObject* pArgs=PyTuple_New(1);
    PyTuple_SetItem(pArgs,0,PyUnicode_FromString(date_str.c_str()));
    PyObject* pResult=PyObject_CallObject(pFunc,pArgs);
    Py_DECREF(pArgs);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
    if (!pResult) {
        PyErr_Print();
        std::cerr<<"Error.\n";
        return false;
    }
	std::cout<<"run ok\n";
    bool ok=false;
    if (PyTuple_Check(pResult) && PyTuple_Size(pResult) == 3) {
        PyObject* py_sr=PyTuple_GetItem(pResult,0);
        PyObject* py_ss=PyTuple_GetItem(pResult,1);
        PyObject* py_aqi=PyTuple_GetItem(pResult,2);
        std::cout<<"tuple\n";
		std::cout<<(py_sr!=Py_None)<<' '<<(py_ss!=Py_None)<<' '<<(py_aqi!=Py_None)<<' '<<(PyLong_Check(py_aqi))<<'\n';
		if (py_sr!=Py_None && py_ss!=Py_None && py_aqi!=Py_None && PyLong_Check(py_aqi)) {
            std::string sr_str=PyUnicode_AsUTF8(py_sr);
            std::string ss_str=PyUnicode_AsUTF8(py_ss);
            aqi = static_cast<int>(PyLong_AsLong(py_aqi));
            std::cout<<sr_str<<ss_str<<aqi<<'\n';
			sr=str_to_min(sr_str);
            ss=str_to_min(ss_str);
            ok=true;
        }
		else std::cout<<"none\n";
    }
    Py_DECREF(pResult);
    return ok;
}

int get_cur_min() {
    std::time_t t=std::time(nullptr);
    std::tm* now=std::localtime(&t);
    return now->tm_hour*60+now->tm_min;
}

int cal_pwm(int cur_min,int sr,int ss,int aqi) {
	bool is_night=(cur_min>=ss || cur_min<sr);
	if (!is_night) return 0;
	int elapsed;
	if (cur_min>=ss) elapsed=cur_min-ss;
	else elapsed=(24*60-ss)+cur_min;
	int night_time=(24*60-ss)+sr;
	double lambda=elapsed*1.0/night_time;
	double base=255.0*(1.0-2.0*std::abs(lambda-0.5));
	double aqi_fac=1.0+(aqi-50)/200.0;
	if (aqi_fac<0.5) aqi_fac=0.5;
	if (aqi_fac>2.0) aqi_fac=2.0;
	int pwm=(int)(base*aqi_fac);
	if (pwm>255) pwm=255;
	if (pwm<0) pwm=0;
	return pwm;
}

void send_to_arduino(int pwm,const std::string& port) {
	HANDLE hSerial = CreateFileA(port.c_str(),GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hSerial==INVALID_HANDLE_VALUE) {
        std::cerr<<"Failed to open the port: "<<port<<'\n';
        return;
    }
    DCB dcb={0};
    dcb.DCBlength=sizeof(DCB);
    GetCommState(hSerial,&dcb);
    dcb.BaudRate=CBR_9600;
    dcb.ByteSize=8;
    dcb.StopBits=ONESTOPBIT;
    dcb.Parity=NOPARITY;
    SetCommState(hSerial,&dcb);
    std::string data=std::to_string(pwm)+'\n';
    DWORD written;
    WriteFile(hSerial,data.c_str(),data.size(),&written,NULL);
    CloseHandle(hSerial);
	return;
}

int main() {
    Py_Initialize();
	int pre_aqi=50;
    while (true) {
        int Y,M,D;
		get_cur_date(Y,M,D);
        int cur_min=get_cur_min();
        std::string date_str=date_to_str(Y,M,D);
        int sr,ss,aqi;
        if (call_py(date_str,sr,ss,aqi)) {
			std::cout<<"Success! Sunrise_time="<<sr/60<<':'<<sr%60<<" Sunset_time="<<ss/60<<':'<<ss%60<<" AQI="<<aqi<<'\n';
			pre_aqi=aqi;
		}
		else{
            std::cerr<<"Failed to call python.\n";
            auto tmp=cal_time(Y,M,D);
			sr=tmp.first;
			ss=tmp.second;
			aqi=pre_aqi;
        }
        int pwm=cal_pwm(cur_min,sr,ss,aqi);
        std::cout<<"Current time: "<<cur_min/60<<':'<<cur_min%60<<" PWM: "<< pwm<<'\n';
        send_to_arduino(pwm,port);
		std::this_thread::sleep_for(std::chrono::minutes(5));
    }
    Py_Finalize();
	return 0;
}