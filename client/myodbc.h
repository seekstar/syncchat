#ifndef MYODBC_H
#define MYODBC_H

#include <string>

bool login(void);
bool logout(void);
bool exec_sql(const std::string& stmt, bool critical);

#endif // MYODBC_H
