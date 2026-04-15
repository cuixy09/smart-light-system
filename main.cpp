#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <ctime>
#include <iomanip>

void get_cur_date(int& Y,int& M,int& D) {
	std::time_t t=std::time(nullptr);
    std::tm* now=std::localtime(&t);
    Y=now->tm_year+1900;
    M=now->tm_mon+1;
    D=now->tm_mday;
}
int main() {
	#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
	#endif
	int Y,M,D;
	get_cur_date(Y,M,D);
	std::cout<<"当前日期: "<<Y<<'-'<<std::setw(2)<<std::setfill('0')<<M<<'-'<<std::setw(2)<<std::setfill('0')<<D<<'\n';
	return 0;
}