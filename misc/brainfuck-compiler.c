/* This is an implementation of a 'compiler' for the language brainfuck.
 * Brainfuck only contains 8 symbols which can all be trivially translated into
 * C as this compiler demonstrates. This is basically a vehicle for me to
 * experiment with compiler optimisations so don't expect to see anything
 * astounding here.
 */

#include <stdio.h>
#include <string.h>

#define SYMBOL_BUFFER 20

int main(int argc, char** argv) {
    int c;
    char* indent = "    ";
    char symbol[SYMBOL_BUFFER];
    unsigned int indent_level = 1, i;

    /* Write prologue. */
    printf("#include <stdio.h>\n#include <stdlib.h>\n\nint main(int argc, char** argv) {\n%sunsigned char* p;\n%sp = (unsigned char*)malloc(sizeof(unsigned char) * 30000);\n", indent, indent);

    /* Translate the body of the code. */
    while ((c = getchar()) != EOF) {
        switch (c) {
            case '>':
                strcpy(symbol, "++p;");
                break;
            case '<':
                strcpy(symbol, "--p;");
                break;
            case '-':
                strcpy(symbol, "--*p;");
                break;
            case '+':
                strcpy(symbol, "++*p;");
                break;
            case ',':
                strcpy(symbol, "*p = getchar();");
                break;
            case '.':
                strcpy(symbol, "putchar(*p);");
                break;
            case '[':
                strcpy(symbol, "while (*p) {");
                break;
            case ']':
                --indent_level;
                strcpy(symbol, "}");
                break;
            default:
                continue;
        }

        for (i = 0; i < indent_level; ++i)
            printf("%s", indent);
        printf("%s\n", symbol);
        if (c == '[') ++indent_level;
    }

    /* Write epilogue. */
    printf("%sreturn 0;\n}\n", indent);

    return 0;
}
