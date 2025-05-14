#ifndef _PRETTY_PRINT_H_
#define _PRETTY_PRINT_H_

#include <iostream>
#include <iomanip>
#include <string>

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define ORANGE "\033[38;5;208m"

#define TAG_OK GREEN "[+]" RESET " "
#define TAG_FAIL RED "[-]" RESET " "
#define TAG_IDK BLUE "[*]" RESET " "
#define TAG_PROGRESS YELLOW "[~]" RESET " "

#define TWIDTH 70
#define PBWIDTH TWIDTH - 9

static void printProgress(int percentage)
{
    int bar_width = TWIDTH - 9;
    int val = percentage;
    int lpad = (percentage * bar_width) / 100;
    int rpad = bar_width - lpad;
    std::cout << "\r";
    if (percentage == 100)
    {
        std::cout << TAG_OK;
    }
    else
    {
        std::cout << TAG_PROGRESS;
    }
    std::cout << ORANGE << std::setw(3) << std::setfill(' ') << std::right << val << "%" << RESET << " [";
    std::cout << GREEN << std::string(lpad, '|') << std::string(rpad, ' ') << RESET << "] ";
    if (percentage == 100)
    {
        std::cout << std::endl;
    }
    else
    {
        std::cout.flush();
    }
}

static void center_print(const std::string &text)
{
    int pad = (TWIDTH - text.length()) / 2;
    std::cout << std::string(pad, '=') << " " << text << " " << std::string(pad, '=');
    if (text.length() % 2 != 0)
    {
        std::cout << '=';
    }
    std::cout << std::endl;
}

#endif