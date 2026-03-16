#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群组用户， 多了一个role角色信息，从User类直接继承，复用User的其它信息
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    // void setId(int id) { this->id = id; }
    // void setName(string name) { this->name = name; }
    // void setState(string state) { this->state = state; }
    
    string getRole() { return this->role; }
    // int getId() { return this->id; }
    // string getName() { return this->name; }
    // string getState() { return this->state; }

private:
    string role;
    // int id;
    // string name;
    // string state;
    
};

#endif