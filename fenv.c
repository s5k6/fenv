#include <stdio.h>


int main(int argc, char **argv) {

    (void)argc; (void)argv;

    if (argc < 2) {
        printf(
            "\n"
#include "fenv.help"
            "\n"
        );
        return 0;
    }

    printf("gello %d\n", argc);

    return 0;
}
