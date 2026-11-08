#include "codexion.h"

int main(int argc, char *argv[])
{
    int check;
    t_config config;

    check = error_check(argc, argv);
    if (check == -1)
        return (1);
    input(&config, argv);
    simulation(&config);



    return (0);
}