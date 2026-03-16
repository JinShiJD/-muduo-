#include "CommonConnectionPool.h"
#include "public.h"
#include <iostream>

// 线程安全的懒汉单例模式
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool; //静态变量自动加锁
	return &pool;
}

//给外部提供接口，从连接池获取一个可用的空闲接口
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			if (_connectionQue.empty())
			{
				LOG("获取连接超时了...获取连接失败! ");
				return nullptr;
			}
		}
	}
		  
		//队列不为空
		/*
		shared_ptr智能指针析构是，会把connection资源直接delete掉，
		相当于调用connection的析构函数,connection就被析构掉了。
		这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue中
		*/
		shared_ptr<Connection> sp(_connectionQue.front(),
			[&](Connection *pcon)
			{
				unique_lock<mutex> lock(_queueMutex);
				pcon->refreshAliveTime();
				_connectionQue.push(pcon);
			});
		_connectionQue.pop();
		cv.notify_all();  // 消费完以后，通知生产者线程检查一下，如果队列空了赶紧生产

		return sp;

}

//从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}

	while (!feof(pf)) //文件没有到末尾 ==>> 文件结束返回 非0值 ：0
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0); //从0开始
		if (idx == -1) //没找到 ==>> 无效配置项
		{
			continue;
		}

		// pwd == xxxx\n
		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);
		
		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "userName")
		{
			_userName = value;
		}
		else if (key == "password")
		{
			_pwd = value;
		}
		else if (key == "dbName")
		{
			_dbName = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeout")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}
	return true;
}


//运行在独立线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			cv.wait(lock); // 队列不空，此处生产线程进入等待状态
		}

		// 连接数量没有到达上限，继续创建新连接 
		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _userName, _pwd, _dbName);
			p->refreshAliveTime();
			_connectionQue.push(p);
			_connectionCnt++;
		}

		// 通知消费者线程消费连接
		cv.notify_all();
	}
}

// 扫描超过maxIdleTime时间的空闲连接，进行对于连接的回收
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		//通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));
		
		//扫描整个队列，释放多余的连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p; // 调用~Connection() 释放连接
			}
			else
			{
				break;
			}
		}

	}
}


//连接池的构造
ConnectionPool::ConnectionPool()
{
	// 加载配置项了
	if (!loadConfigFile())
	{
		return;
	}
	//创建初始的连接数量
	for (int i = 0; i < _initSize; ++ i)
	{
		Connection *p = new Connection();
		p->connect(_ip, _port, _userName, _pwd, _dbName);
		p->refreshAliveTime(); //刷新开始空闲的起始时间
		_connectionQue.push(p);
		_connectionCnt ++;
	}

	//启动一个新的线程作为连接的生产者 Linux: thread==>>pthread create
	thread produceConnTask(std::bind(&ConnectionPool::produceConnectionTask, this));
	produceConnTask.detach();

	// 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行对于连接的回收
	thread scannerTask(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scannerTask.detach();

}