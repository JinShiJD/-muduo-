#include <iostream>

#include "Connection.h"
#include "CommonConnectionPool.h"
using namespace std;

int main()
{
    /*
    Connection conn;
    char sql[1024] = { 0 };
    sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
        "Star", 23, "female");
    conn.connect("127.0.0.1", 3306, "root", "mgcngxsb", "chat");
    conn.update(sql);

    */

    //clock_t begin = clock();
    //for (int i = 0; i < 1000; ++i)
    //{
    //    Connection conn;
    //    char sql[1024] = { 0 };
    //    sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
    //        "Star", 23, "female");
    //    conn.connect("127.0.0.1", 3306, "root", "mgcngxsb", "chat");
    //    conn.update(sql);
    //}
    //clock_t end = clock();

    //cout << (end - begin) << "ms" << endl;

    //使用连接池
    //clock_t begin = clock();
    //ConnectionPool* cp = ConnectionPool::getConnectionPool();
    //for (int i = 0; i < 1000; ++i)
    //{
    //    shared_ptr<Connection> sp = cp->getConnection();
    //    char sql[1024] = { 0 };
    //    sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
    //    "Star", 23, "female"); 
    //    sp->update(sql);
    //}
    //clock_t end = clock();

    //cout << (end - begin) << "ms" << endl;

    //开线程
    clock_t begin = clock();
    thread t1([]() {
        ConnectionPool* cp = ConnectionPool::getConnectionPool();
        for (int i = 0; i < 1000; ++i)
        {
            shared_ptr<Connection> sp = cp->getConnection();
            char sql[1024] = { 0 };
            sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
            "Star", 23, "female"); 
            sp->update(sql);
        }

        });
    

    thread t2([]() {
        ConnectionPool* cp = ConnectionPool::getConnectionPool();
        for (int i = 0; i < 1000; ++i)
        {
            shared_ptr<Connection> sp = cp->getConnection();
            char sql[1024] = { 0 };
            sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
                "Star", 23, "female");
            sp->update(sql);
        }

        });

    t1.join();
    t2.join();
    clock_t end = clock();
    cout << (end - begin) << "ms" << endl;

    return 0;
}

