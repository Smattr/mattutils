/* This is an implementation of a 'compiler' for the language brainfuck.
 * Brainfuck only contains 8 symbols which can all be trivially translated into
 * C as this compiler demonstrates. This is basically a vehicle for me to
 * experiment with compiler optimisations so don't expect to see anything
 * astounding here.
 */

#include <stdio.h>

int main(int argc, char** argv) {
    int c;

    /* Write prologue. */
    printf("#include <stdio.h>\n#include <stdlib.h>\n\nint main(int argc, char** argv) {\nunsigned char* p;\np = (unsigned char*)malloc(sizeof(unsigned char) * 30000);\n");

    /* Translate the body of the code. */
    while ((c = getchar()) != EOF) {
        switch (c) {
            case '>':
                printf("++p;\n");
                break;
            case '<':
                printf("--p;\n");
                break;
            case '-':
                printf("--*p;\n");
                break;
            case '+':
                printf("++*p;\n");
                break;
            case ',':
                printf("*p = getchar();\n");
                break;
            case '.':
                printf("putchar(*p);\n");
                break;
            case '[':
                printf("while (*p) {\n");
                break;
            case ']':
                printf("}\n");
                break;
        }
    }

    /* Write epilogue. */
    printf("return 0;\n}\n");

    return 0;
}
