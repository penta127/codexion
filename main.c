#include "codexion.h"

int main(int argc, char *argv[])
{
    bool valid_input;
    t_config config;

    valid_input = error_check(argc, argv);
    if (!valid_input)
        return (1);
    input(&config, argv);
    if (!simulation(&config))
        return (1);



    return (0);
}
