#ifndef KANON_LOG_TERMINALCOLOR_H
#define KANON_LOG_TERMINALCOLOR_H

// Use \033 to replace \e
// since the latter is useless in windows
#define NONE "\033[0m"
#define BLACK "\033[0;30m"
#define L_BLACK "\033[1;30m"
#define RED "\033[0;31m"
#define L_RED "\033[1;31m"
#define GREEN "\033[0;32m"
#define L_GREEN "\033[1;32m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[0;34m"
#define L_BLUE "\033[1;34m"
#define PURPLE "\033[0;35m"
#define L_PURPLE "\033[1;35m"
#define CYAN "\033[0;36m"
#define L_CYAN "\033[1;36m"
#define GRAY "\033[0;37m"
#define WHITE "\033[1;37m"

#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define BLINK "\033[5m"
#define REVERSE "\033[7m"
#define HIDE "\033[8m"
#define CLEAR "\033[2J"
// #define CLRLINE              "\e[1K\r"

#endif
