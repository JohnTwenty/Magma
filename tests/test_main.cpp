#include <cstdio>
#include <cstring>

void test_String_endsWith();

struct TestCase {
    const char* name;
    void (*func)();
};

static TestCase tests[] = {
    {"String.endsWith", test_String_endsWith},
};

int main(int argc, char** argv)
{
    const char* filter = nullptr;
    bool list = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--list") == 0) {
            list = true;
        } else if (strncmp(argv[i], "--filter=", 9) == 0) {
            filter = argv[i] + 9;
        }
    }

    int count = sizeof(tests) / sizeof(TestCase);
    if (list) {
        for (int i = 0; i < count; ++i) {
            printf("%s\n", tests[i].name);
        }
        return 0;
    }

    int ran = 0;
    for (int i = 0; i < count; ++i) {
        if (!filter || strstr(tests[i].name, filter)) {
            printf("Running %s\n", tests[i].name);
            tests[i].func();
            ++ran;
        }
    }
    printf("Ran %d test(s)\n", ran);
    return 0;
}
