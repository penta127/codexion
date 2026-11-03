#include "codexion.h"

int number_check(char argv[], int yes_no_zero)
{
    int num;
    int i;
    i = 0;
    if (argv[0] == '\0')
        return (-1);
    while (argv[i] != '\0')
    {
        if (!('0' <= argv[i] && argv[i] <= '9'))
            return (-1);
        i++;
    }
    num = ft_atoi(argv);
    if (yes_no_zero == 0)
    {
        if (num <= 0)
            return (-1);
    }
    else
    {
        if (num < 0)
            return (-1);
    }
    
    return (0);
}

int error_check(int argc, char *argv[])
{
    int i;
    int check;
    int yes_no_zero;

    i = 1;
    if (argc != 9)
        return (-1);
    while(i <= 7)
    {
        if (i != 7)
            yes_no_zero = 0;
        else
            yes_no_zero = 1;
        check = number_check(argv[i], yes_no_zero); 
        if (check == -1)
            return (-1);
        i++;
    }
    if (strcmp(argv[i], "fifo") != 0 && strcmp(argv[i], "edf") != 0)
        return (-1);
    return(0);
}
