#pragma once

#define LOG(str) \
	cout << __FILE__ << ":" << __LINE__ << " "<< \
	__TIMESTAMP__<<" : " << str << endl;  //文件、行号、时间、信息