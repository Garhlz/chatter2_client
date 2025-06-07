// src/utils/UserInfo.cpp
#include "UserInfo.h"

UserInfo& UserInfo::instance()
{
    static UserInfo instance;
    return instance;
}